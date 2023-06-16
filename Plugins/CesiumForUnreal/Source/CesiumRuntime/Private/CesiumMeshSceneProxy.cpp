// Copyright Peter Leontev

#include "CesiumMeshSceneProxy.h"
#include "CesiumVectorComponent.h"
#include "CesiumMeshSection.h"

/** Class representing a single section of the proc mesh */
class FMeshProxySection
{
public:
    virtual ~FMeshProxySection()
    {
        check (IsInRenderingThread());

        VertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
        VertexBuffers.PositionVertexBuffer.ReleaseResource();
        VertexBuffers.ColorVertexBuffer.ReleaseResource();
        IndexBuffer.ReleaseResource();
        VertexFactory.ReleaseResource();
    }

public:
    /** Material applied to this section */
    class UMaterialInterface* Material;
    /** Vertex buffer for this section */
    FStaticMeshVertexBuffers VertexBuffers;
    /** Index buffer for this section */
    FRawStaticIndexBuffer IndexBuffer;
    /** Vertex factory for this section */
    FLocalVertexFactory VertexFactory;
    /** Whether this section is currently visible */
    bool bSectionVisible;
    /** Section bounding box */
    FBox3f SectionLocalBox;
    /** Whether this section is initialized i.e. render resources created */
    bool bInitialized;

    FMeshProxySection(ERHIFeatureLevel::Type InFeatureLevel)
        : Material(NULL)
        , VertexFactory(InFeatureLevel, "FMeshProxySection")
        , bSectionVisible(true)
        , bInitialized(false)
    {}
};


FLineMeshSceneProxy::FLineMeshSceneProxy(UCesiumVectorComponent* InComponent)
	: FPrimitiveSceneProxy(InComponent),
	Component(InComponent),
	MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
{
    
}

FLineMeshSceneProxy::~FLineMeshSceneProxy()
{
    check (IsInRenderingThread());
    
    for (TTuple<int32, TSharedPtr<FMeshProxySection>> KeyValueIter : Sections)
    {
        TSharedPtr<FMeshProxySection> Section = KeyValueIter.Value;
        if (Section.IsValid())
        {
            Section->VertexBuffers.PositionVertexBuffer.ReleaseResource();
            Section->VertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
            Section->VertexBuffers.ColorVertexBuffer.ReleaseResource();
            Section->IndexBuffer.ReleaseResource();
            Section->VertexFactory.ReleaseResource();
        }
    }
}

SIZE_T FLineMeshSceneProxy::GetTypeHash() const
{
    static size_t UniquePointer;
    return reinterpret_cast<size_t>(&UniquePointer);
}

void FLineMeshSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
    // SCOPE_CYCLE_COUNTER(STAT_ProcMesh_GetMeshElements);

    // Iterate over sections
    for (const TTuple<int32, TSharedPtr<FMeshProxySection>>& KeyValueIter : Sections)
    {
        TSharedPtr<FMeshProxySection> Section = KeyValueIter.Value;

        if (Section.IsValid() && Section->bInitialized && Section->bSectionVisible)
        {
            FMaterialRenderProxy* MaterialProxy = Section->Material->GetRenderProxy();

            // For each view..
            for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
            {
                if (VisibilityMap & (1 << ViewIndex))
                {
                    const FSceneView* View = Views[ViewIndex];
                    // Draw the mesh.
                    FMeshBatch& Mesh = Collector.AllocateMesh();
                    Mesh.VertexFactory = &Section->VertexFactory;
                    Mesh.MaterialRenderProxy = MaterialProxy;
                    Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
                    Mesh.Type = PT_LineList;
                    Mesh.DepthPriorityGroup = SDPG_Foreground;
                    Mesh.bCanApplyViewModeOverrides = false;

                    FMeshBatchElement& BatchElement = Mesh.Elements[0];
                    BatchElement.IndexBuffer = &Section->IndexBuffer;

                    bool bHasPrecomputedVolumetricLightmap;
                    FMatrix PreviousLocalToWorld;
                    int32 SingleCaptureIndex;
                    bool bOutputVelocity;
                    GetScene().GetPrimitiveUniformShaderParameters_RenderThread(
					GetPrimitiveSceneInfo(), bHasPrecomputedVolumetricLightmap, PreviousLocalToWorld, SingleCaptureIndex, bOutputVelocity);

                    FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer = Collector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
                    DynamicPrimitiveUniformBuffer.Set(GetLocalToWorld(), PreviousLocalToWorld, GetBounds(), GetLocalBounds(), true, bHasPrecomputedVolumetricLightmap, DrawsVelocity(), bOutputVelocity);
                    BatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;

                    BatchElement.FirstIndex = 0;
                    BatchElement.NumPrimitives = Section->IndexBuffer.GetNumIndices() / 2;
                    BatchElement.MinVertexIndex = 0;
                    BatchElement.MaxVertexIndex = Section->VertexBuffers.PositionVertexBuffer.GetNumVertices() - 1;
                   
                    Collector.AddMesh(ViewIndex, Mesh);
                }
            }
        }
    }
}

FPrimitiveViewRelevance FLineMeshSceneProxy::GetViewRelevance(const FSceneView* View) const
{
    FPrimitiveViewRelevance Result;
    Result.bDrawRelevance = IsShown(View);
    Result.bShadowRelevance = IsShadowCast(View);
    Result.bDynamicRelevance = true;
    Result.bRenderInMainPass = ShouldRenderInMainPass();
    Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
    Result.bRenderCustomDepth = ShouldRenderCustomDepth();
    Result.bTranslucentSelfShadow = bCastVolumetricTranslucentShadow;
    MaterialRelevance.SetPrimitiveViewRelevance(Result);
    Result.bVelocityRelevance = IsMovable() && Result.bOpaque && Result.bRenderInMainPass;
    return Result;
}

void FLineMeshSceneProxy::SetSection_GameThread(TSharedPtr<FCesiumMeshSection> SrcSection)
{
    check(IsInGameThread());

    const int32 SrcSectionIndex = SrcSection->SectionIndex;

    // Copy data from vertex buffer
    const int32 NumVerts = SrcSection->ProcVertexBuffer.Num();

    TSharedPtr<FMeshProxySection> NewSection(MakeShareable(new FMeshProxySection(GetScene().GetFeatureLevel())));
    {
        NewSection->VertexBuffers.StaticMeshVertexBuffer.Init(NumVerts, 2, true);

        TArray<FVector3f> InVertexBuffer(MoveTemp(SrcSection->ProcVertexBuffer));
        NewSection->VertexBuffers.PositionVertexBuffer.Init(InVertexBuffer, true);
        NewSection->IndexBuffer.SetIndices(SrcSection->ProcIndexBuffer, EIndexBufferStride::Force16Bit);

        // Enqueue initialization of render resource
        BeginInitResource(&NewSection->VertexBuffers.PositionVertexBuffer);
        BeginInitResource(&NewSection->VertexBuffers.StaticMeshVertexBuffer);
        BeginInitResource(&NewSection->IndexBuffer);

        // Grab material
        NewSection->Material = SrcSection->Material;

        // Copy visibility info
        NewSection->bSectionVisible = SrcSection->bSectionVisible;
        NewSection->SectionLocalBox = SrcSection->SectionLocalBox;
    }

	//通过这个渲染指令更新顶点缓存
    ENQUEUE_RENDER_COMMAND(FMeshProxySection)(
        [this, SrcSectionIndex, NewSection](FRHICommandListImmediate& RHICmdList)
        {
            FLocalVertexFactory::FDataType Data;

            NewSection->VertexBuffers.PositionVertexBuffer.BindPositionVertexBuffer(&NewSection->VertexFactory, Data);
            NewSection->VertexBuffers.StaticMeshVertexBuffer.BindTangentVertexBuffer(&NewSection->VertexFactory, Data);
            NewSection->VertexBuffers.StaticMeshVertexBuffer.BindPackedTexCoordVertexBuffer(&NewSection->VertexFactory, Data);
            NewSection->VertexBuffers.StaticMeshVertexBuffer.BindLightMapVertexBuffer(&NewSection->VertexFactory, Data, 1);

            Data.LODLightmapDataIndex = 0;

            NewSection->VertexFactory.SetData(Data);
            NewSection->VertexFactory.InitResource();

#if WITH_EDITOR
            TArray<UMaterialInterface*> UsedMaterials;
            Component->GetUsedMaterials(UsedMaterials);

            SetUsedMaterialForVerification(UsedMaterials);
#endif

            Sections.Add(SrcSectionIndex, NewSection);
			/*
            AsyncTask(
                ENamedThreads::GameThread, 
                [this]()
                {
                    Component->UpdateLocalBounds();
                }
            );
			*/
            NewSection->bInitialized = true;
        }
    );
}

bool FLineMeshSceneProxy::CanBeOccluded() const
{
    return !MaterialRelevance.bDisableDepthTest;
}

uint32 FLineMeshSceneProxy::GetMemoryFootprint() const
{
    return sizeof(*this) + GetAllocatedSize();
}

uint32 FLineMeshSceneProxy::GetAllocatedSize() const
{
    return FPrimitiveSceneProxy::GetAllocatedSize();
}