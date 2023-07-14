// Copyright Peter Leontev

#include "LineMeshSceneProxy.h"
#include "LineMeshComponent.h"
#include "LineMeshSection.h"
#include "Async/Async.h"
#include "Stats/Stats.h"
#include "Runtime/Core/Public/HAL/ThreadingBase.h"

/** Class representing a single section of the proc mesh */
class FLineMeshProxySection
{
public:
    virtual ~FLineMeshProxySection()
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

    FLineMeshProxySection(ERHIFeatureLevel::Type InFeatureLevel)
        : Material(NULL)
        , VertexFactory(InFeatureLevel, "FLineMeshProxySection")
        , bSectionVisible(true)
        , bInitialized(false)
    {}
};


inline void InitOrUpdateResource(FRenderResource* Resource)
{
	if (!Resource->IsInitialized())
	{
		Resource->InitResource();
	}
	else
	{
		Resource->UpdateRHI();
	}
}

FLineMeshSceneProxy::FLineMeshSceneProxy(ULineMeshComponent* InComponent)
: FPrimitiveSceneProxy(InComponent), Component(InComponent), MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
{
    Mutex = new FCriticalSection;
	for (auto aaa :InComponent->Sections)
	{
		AddNewSection_GameThread(aaa);
	}
	
}

FLineMeshSceneProxy::~FLineMeshSceneProxy()
{
    check (IsInRenderingThread());
    
    for (TTuple<int32, TSharedPtr<FLineMeshProxySection>> KeyValueIter : Sections)
    {
        TSharedPtr<FLineMeshProxySection> Section = KeyValueIter.Value;
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
    // Iterate over sections
    for (const TTuple<int32, TSharedPtr<FLineMeshProxySection>>& KeyValueIter : Sections)
    {
        TSharedPtr<FLineMeshProxySection> Section = KeyValueIter.Value;

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
                    Mesh.DepthPriorityGroup = SDPG_World;
                    Mesh.bCanApplyViewModeOverrides = false;

                    FMeshBatchElement& BatchElement = Mesh.Elements[0];
                    BatchElement.IndexBuffer = &Section->IndexBuffer;

                    bool bHasPrecomputedVolumetricLightmap;
                    FMatrix PreviousLocalToWorld;
                    int32 SingleCaptureIndex;
                    bool bOutputVelocity;
                    GetScene().GetPrimitiveUniformShaderParameters_RenderThread(GetPrimitiveSceneInfo(), bHasPrecomputedVolumetricLightmap, PreviousLocalToWorld, SingleCaptureIndex, bOutputVelocity);

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

void FLineMeshSceneProxy::AddNewSection_GameThread(TSharedPtr<FLineMeshSection> SrcSection)
{
    check(IsInGameThread());

    const int32 SrcSectionIndex = SrcSection->SectionIndex;

    // Copy data from vertex buffer
    const int32 NumVerts = SrcSection->ProcVertexBuffer.Num();

    TSharedPtr<FLineMeshProxySection> NewSection(MakeShareable(new FLineMeshProxySection(GetScene().GetFeatureLevel())));
    {
        NewSection->VertexBuffers.StaticMeshVertexBuffer.Init(NumVerts, 2, true);

        TArray<FVector3f> InVertexBuffer(MoveTemp(SrcSection->ProcVertexBuffer));
        NewSection->VertexBuffers.PositionVertexBuffer.Init(InVertexBuffer, true);
        NewSection->IndexBuffer.SetIndices(SrcSection->ProcIndexBuffer, EIndexBufferStride::Force16Bit);

        // Grab material
        NewSection->Material = SrcSection->Material;

        // Copy visibility info
        NewSection->bSectionVisible = SrcSection->bSectionVisible;
        NewSection->SectionLocalBox = SrcSection->SectionLocalBox;
    }
	auto aaa = &NewSection->VertexFactory;
    ENQUEUE_RENDER_COMMAND(LineMeshVertexBuffersInit)(
        [this, SrcSectionIndex, NewSection](FRHICommandListImmediate& RHICmdList)
        {
			InitOrUpdateResource(&NewSection->VertexBuffers.PositionVertexBuffer);
			//InitOrUpdateResource(&NewSection->VertexBuffers.StaticMeshVertexBuffer);

            FLocalVertexFactory::FDataType Data;
            NewSection->VertexBuffers.PositionVertexBuffer.BindPositionVertexBuffer(&NewSection->VertexFactory, Data);
            NewSection->VertexBuffers.StaticMeshVertexBuffer.BindTangentVertexBuffer(&NewSection->VertexFactory, Data);
            NewSection->VertexBuffers.StaticMeshVertexBuffer.BindPackedTexCoordVertexBuffer(&NewSection->VertexFactory, Data);
            NewSection->VertexBuffers.StaticMeshVertexBuffer.BindLightMapVertexBuffer(&NewSection->VertexFactory, Data, 1);

            Data.LODLightmapDataIndex = 0;

            NewSection->VertexFactory.SetData(Data);
			InitOrUpdateResource(&NewSection->VertexFactory);
            

			FScopeLock ScopeLock(Mutex);
            Sections.Add(SrcSectionIndex, NewSection);

            NewSection->bInitialized = true;
        }
    );

	 // Enqueue initialization of render resource
        BeginInitResource(&NewSection->VertexBuffers.PositionVertexBuffer);
        BeginInitResource(&NewSection->VertexBuffers.StaticMeshVertexBuffer);
        BeginInitResource(&NewSection->IndexBuffer);
		BeginInitResource(&NewSection->VertexFactory);
/*
#if WITH_EDITOR
		TArray<UMaterialInterface*> UsedMaterials;
		Component->GetUsedMaterials(UsedMaterials);

		SetUsedMaterialForVerification(UsedMaterials);
#endif
*/
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

int32 FLineMeshSceneProxy::GetNumSections() const
{
    return Sections.Num();
}

int32 FLineMeshSceneProxy::GetNumPointsInSection(int32 SectionIndex) const
{
    if (Sections.Contains(SectionIndex))
    {
        return Sections[SectionIndex]->VertexBuffers.PositionVertexBuffer.GetNumVertices();
    }

    return 0;
}

void FLineMeshSceneProxy::ClearMeshSection(int32 SectionIndex)
{
    ENQUEUE_RENDER_COMMAND(ReleaseSectionResources)(
        [this, SectionIndex](FRHICommandListImmediate&)
        {
            TSharedPtr<FLineMeshProxySection> Section = Sections[SectionIndex];

            Section->VertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
            Section->VertexBuffers.PositionVertexBuffer.ReleaseResource();
            Section->VertexBuffers.ColorVertexBuffer.ReleaseResource();
            Section->IndexBuffer.ReleaseResource();
            Section->VertexFactory.ReleaseResource();

            Sections.Remove(SectionIndex);
        }
    );
    
}

void FLineMeshSceneProxy::ClearAllMeshSections()
{
    TArray<int32> SectionIndices;
    Sections.GetKeys(SectionIndices);

    for (int32 SectionIndex : SectionIndices)
    {
        ClearMeshSection(SectionIndex);
    }
}

void FLineMeshSceneProxy::SetMeshSectionVisible(int32 SectionIndex, bool bNewVisibility)
{
    ENQUEUE_RENDER_COMMAND(SetMeshSectionVisibility)(

	[this, SectionIndex, bNewVisibility](FRHICommandListImmediate&)
        {
            if (Sections.Contains(SectionIndex))
            {
                Sections[SectionIndex]->bSectionVisible = bNewVisibility;
            }
        }
    );
}

void FLineMeshSceneProxy::SetAllSectionVisible( bool bNewVisibility)
{
	if (Sections.Num() < 1)
	{
		return;
	}
	TArray<int32> SectionIndices;
    Sections.GetKeys(SectionIndices);
    for (int32 SectionIndex : SectionIndices)
    {
         ENQUEUE_RENDER_COMMAND(SetMeshSectionVisibility)
		 (
			 [this, SectionIndex, bNewVisibility](FRHICommandListImmediate&)
				{
					if (Sections.Contains(SectionIndex))
					{
						Sections[SectionIndex]->bSectionVisible = bNewVisibility;
					}
				}
		);
    }
}

bool FLineMeshSceneProxy::IsMeshSectionVisible(int32 SectionIndex) const
{
    return Sections.Contains(SectionIndex) && Sections[SectionIndex]->bSectionVisible;
}
