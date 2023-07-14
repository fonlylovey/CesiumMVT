// Copyright Peter Leontev

#include "LineMeshComponent.h"
#include "LineMeshSceneProxy.h"
#include "LineMeshSection.h"
#include "Async/Async.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Runtime/Core/Public/HAL/ThreadingBase.h"


DEFINE_LOG_CATEGORY_STATIC(LogLineRendererComponent, Log, All);

ULineMeshComponent::ULineMeshComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	Mutex = new FCriticalSection;
}

void ULineMeshComponent::CreateLine(int32 SectionIndex, const TArray<FVector>& InVertices, const FLinearColor& Color)
{
    // SCOPE_CYCLE_COUNTER(STAT_ProcMesh_CreateMeshSection);

    TArray<FVector3f> Vertices(InVertices);

    FLineMeshSection* NewSection = new FLineMeshSection;

    // Copy data to vertex buffer 10;//
    const int32 NumVerts = Vertices.Num();
    NewSection->ProcVertexBuffer.Reset();
    NewSection->ProcVertexBuffer.Reserve(NumVerts);

    for (int32 VertIdx = 0; VertIdx < NumVerts; VertIdx++)
    {
		NewSection->ProcVertexBuffer.Add(Vertices[VertIdx]);
		NewSection->SectionLocalBox += Vertices[VertIdx];
    }

	if (Vertices.Num() - 1 > 0)
	{
		NewSection->ProcIndexBuffer.Reset();
		NewSection->ProcIndexBuffer.SetNumZeroed(2 * (NumVerts - 1));
	}

	const int32 NumTris = NumVerts - 1;

	for (int32 TriInd = 0; TriInd < NumTris; ++TriInd)
	{
		NewSection->ProcIndexBuffer[2 * TriInd] = TriInd;
		NewSection->ProcIndexBuffer[2 * TriInd + 1] = TriInd + 1;
	}

	NewSection->SectionIndex = SectionIndex;
    NewSection->SectionLocalBox = FBox3f(Vertices);
	BoundingBox.Origin = FVector(NewSection->SectionLocalBox.GetCenter());
	BoundingBox.BoxExtent = FVector(NewSection->SectionLocalBox.GetExtent());
	BoundingBox.SphereRadius = BoundingBox.BoxExtent.Size();
    NewSection->Material = CreateOrUpdateMaterial(SectionIndex, Color);
	Sections.Add(NewSection);

	UpdateLocalBounds();
	MarkRenderStateDirty();
}

void ULineMeshComponent::RemoveLine(int32 SectionIndex)
{
	Sections.RemoveAt(SectionIndex);
	FLineMeshSceneProxy* LineMeshSceneProxy = (FLineMeshSceneProxy*)SceneProxy;
	LineMeshSceneProxy->ClearMeshSection(SectionIndex);

    SectionMaterials.Remove(SectionIndex);
}

void ULineMeshComponent::RemoveAllLines()
{
	FLineMeshSceneProxy* LineMeshSceneProxy = (FLineMeshSceneProxy*)SceneProxy;
	LineMeshSceneProxy->ClearAllMeshSections();

    SectionMaterials.Empty();
}

void ULineMeshComponent::SetLineVisible(int32 SectionIndex, bool bNewVisibility)
{
	FLineMeshSceneProxy* LineMeshSceneProxy = (FLineMeshSceneProxy*)SceneProxy;
	LineMeshSceneProxy->SetMeshSectionVisible(SectionIndex, bNewVisibility);
}

bool ULineMeshComponent::IsLineVisible(int32 SectionIndex) const
{
    FLineMeshSceneProxy* LineMeshSceneProxy = (FLineMeshSceneProxy*)SceneProxy;
    return LineMeshSceneProxy->IsMeshSectionVisible(SectionIndex);
}

int32 ULineMeshComponent::GetNumLines() const
{
	FLineMeshSceneProxy* LineMeshSceneProxy = (FLineMeshSceneProxy*)SceneProxy;
	return LineMeshSceneProxy->GetNumSections();
}

void ULineMeshComponent::UpdateLocalBounds()
{
	// Update global bounds
	UpdateBounds();
	// Need to send to render thread
	MarkRenderTransformDirty();
}

FPrimitiveSceneProxy* ULineMeshComponent::CreateSceneProxy()
{
	// SCOPE_CYCLE_COUNTER(STAT_ProcMesh_CreateSceneProxy);

	return new FLineMeshSceneProxy(this);
}

UMaterialInterface* ULineMeshComponent::GetMaterial(int32 ElementIndex) const
{
    if (SectionMaterials.Contains(ElementIndex))
    {
        return SectionMaterials[ElementIndex];
    }
    
    return nullptr;
}
UMaterialInterface* ULineMeshComponent::CreateOrUpdateMaterial(int32 SectionIndex, const FLinearColor& Color)
{
    if (!SectionMaterials.Contains(SectionIndex))
    {
		FScopeLock ScopeLock(Mutex);
        UMaterialInstanceDynamic* MI = UMaterialInstanceDynamic::Create(Material, Material);
        SectionMaterials.Add(SectionIndex, MI);
        OverrideMaterials.Add(MI);
    }

    UMaterialInstanceDynamic* MI = SectionMaterials[SectionIndex];
    MI->SetVectorParameterValue(TEXT("FillColor"), Color);

    return MI;
}

FBoxSphereBounds ULineMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return BoundingBox;
}

void ULineMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials /*= false*/) const
{
    Super::GetUsedMaterials(OutMaterials, false);

	OutMaterials.Add(Material);

	FScopeLock ScopeLock(Mutex);
    for (TTuple<int32, UMaterialInstanceDynamic*> KeyValuePair : SectionMaterials)
    {
        OutMaterials.Add(KeyValuePair.Value);
    }
}