#include "CesiumVectorComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Cesium3DTilesSelection/VectorOverlayTile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "CesiumMeshSection.h"
#include "CustomMeshComponent.h"

UCesiumVectorComponent* UCesiumVectorComponent::CreateOnGameThread(
	const FTileModel* tileModel,
	USceneComponent* pParentComponent)
{
	UCesiumVectorComponent* mvtComponent = NewObject<UCesiumVectorComponent>(pParentComponent, *tileModel->TileName);
	//mvtComponent->AttachToComponent(pParentComponent, FAttachmentTransformRules::KeepRelativeTransform);
	mvtComponent->ReregisterComponent();
	mvtComponent->SetUsingAbsoluteLocation(true);
	mvtComponent->SetMobility(pParentComponent->Mobility);
	mvtComponent->SetFlags(RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
	mvtComponent->BuildMesh(tileModel, TEXT("Mesh_") + tileModel->TileName);
	mvtComponent->SetVisibility(false, true);
	return mvtComponent;
}

// 设置默认值
UCesiumVectorComponent::UCesiumVectorComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
  FString str = TEXT("/CesiumForUnreal/Materials/MVT/M_VectorBase.M_VectorBase");
  BaseMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, *str));
}

UCesiumVectorComponent::~UCesiumVectorComponent()
{

}

void UCesiumVectorComponent::BuildMesh(const FTileModel* tileModel, FString strName)
{
	TArray<FVector>worldPosArray;
	TArray<FLinearColor> colors = {
	FLinearColor::White, FLinearColor::Black, FLinearColor::Red, FLinearColor::Green, FLinearColor::Blue,
	FLinearColor::Yellow, FLinearColor(0,255,255), FLinearColor(255,0,255), FLinearColor(43, 156, 18), FLinearColor(169, 7, 228)};

	/*
	* 测试代码->GetOwner()->GetRootComponent()
	
	lineComponent = NewObject<ULineMeshComponent>(this, *strName);
	lineComponent->RegisterComponent();
	lineComponent->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
	lineComponent->Material = BaseMaterial;

	int index = 0;
	int sectionIndex = 0;
	for (const FCesiumMeshSection& section : tileModel->Sections)
	{
		int32 count = section.IndexBuffer.Num();
		worldPosArray.Empty();
		for (int32 i = 0; i < section.VertexBuffer.Num(); i++)
		{
			//int32 theIndex = section.IndexBuffer[i];
			FVector3f pos = section.VertexBuffer[i];
			worldPosArray.Add(FVector(pos));
		}
		lineComponent->CreateLine(sectionIndex, worldPosArray, colors[index]);
		sectionIndex++;
		++index;
	}
	*/
	/*************************/

	
	VectorMesh = NewObject<UStaticMeshComponent>(this, *strName);
	VectorMesh->AttachToComponent(this->GetOwner()->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	VectorMesh->RegisterComponent();

	UStaticMesh* pStaticMesh = NewObject<UStaticMesh>(VectorMesh);
	pStaticMesh->NeverStream = true;
	pStaticMesh->SetIsBuiltAtRuntime(true);
	
	pStaticMesh->SetFlags(RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
	TUniquePtr<FStaticMeshRenderData> RenderData = MakeUnique<FStaticMeshRenderData>();
	RenderData->AllocateLODResources(1);
	FStaticMeshLODResources& LODResources = RenderData->LODResources[0];
	LODResources.bHasColorVertexData = false;
	LODResources.bHasDepthOnlyIndices = false;
	LODResources.bHasReversedIndices = false;
	LODResources.bHasReversedDepthOnlyIndices = false;

	FStaticMeshSectionArray& Sections = LODResources.Sections;
	TArray<FStaticMeshBuildVertex> vertexData;
	TArray<uint32> indexs;
	int index = 0;
	FVector3f minPos = FVector3f::Zero(), maxPos = FVector3f::Zero();
	for (const FCesiumMeshSection& section : tileModel->Sections)
	{
		int32 count = section.IndexBuffer.Num();
		FStaticMeshSection& staticSection = Sections.AddDefaulted_GetRef();
		staticSection.bEnableCollision = false;
		staticSection.MaterialIndex = 0;
		staticSection.NumTriangles =  count / 3;
		staticSection.FirstIndex = index;
		staticSection.MinVertexIndex = index;
		staticSection.MaxVertexIndex = index + count - 1;
		minPos = minPos.IsZero() ? section.VertexBuffer[0] : minPos;
		maxPos = maxPos.IsZero() ? section.VertexBuffer[0] : maxPos;
		
		for (int32 i = 0; i < section.IndexBuffer.Num(); i++)
		{
			int32 theIndex = section.IndexBuffer[i];
			FStaticMeshBuildVertex vertex;
			vertex.Position = section.VertexBuffer[theIndex];
			vertex.Position.X < minPos.X ? minPos.X = vertex.Position.X: minPos.X;
			vertex.Position.X > maxPos.X ? maxPos.X = vertex.Position.X: maxPos.X;

			vertex.Position.Y < minPos.Y ? minPos.Y = vertex.Position.Y : minPos.Y;
			vertex.Position.Y > maxPos.Y ? maxPos.Y = vertex.Position.Y : maxPos.Y;

			vertex.Color = FColor::Red;
			vertex.UVs[0] = FVector2f(0, 0);
			vertex.TangentZ = FVector3f(0, 0, 1);
			vertexData.Add(vertex);
			indexs.Add(index + theIndex);
			index++;
		}
		
	}

	//设置包围盒
	FBoxSphereBounds BoundingBoxAndSphere;
	BoundingBoxAndSphere.Origin = FVector(0, 0, 0);
	BoundingBoxAndSphere.BoxExtent = FVector(maxPos.X - minPos.X, maxPos.Y - minPos.Y, 1000);
	BoundingBoxAndSphere.SphereRadius = FVector3f::Distance(maxPos, minPos);
	RenderData->Bounds = BoundingBoxAndSphere;
	LODResources.IndexBuffer.SetIndices(indexs, EIndexBufferStride::AutoDetect);
	LODResources.VertexBuffers.PositionVertexBuffer.Init(vertexData);
	LODResources.VertexBuffers.StaticMeshVertexBuffer.Init(vertexData, 1);
	
	
	pStaticMesh->SetRenderData(MoveTemp(RenderData));
	UMaterialInterface* pMaterial = createMaterial();
	pStaticMesh->AddMaterial(pMaterial);
	pStaticMesh->InitResources();
	VectorMesh->SetStaticMesh(pStaticMesh);
	
}

void UCesiumVectorComponent::BeginDestroy()
{
	 Super::BeginDestroy();
	 if(lineComponent != nullptr)
	 {
		 FString strMsg = TEXT("Destroy: ") + lineComponent->GetName();
		 UE_LOG(LogTemp, Error, TEXT("%s"), *strMsg);
		 lineComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		 lineComponent->UnregisterComponent();
		 lineComponent->DestroyComponent();
		 lineComponent = nullptr;
	 }
}


void UCesiumVectorComponent::OnVisibilityChanged()
{
	if (lineComponent != nullptr)
	{
		bool isBisble = lineComponent->IsVisible();
		lineComponent->SetVisibility(isBisble);
	}
}

UMaterialInterface* UCesiumVectorComponent::createMaterial()
{
	//if (!SectionMaterials.Contains(SectionIndex))
    {
    }
    UMaterialInstanceDynamic* mat = UMaterialInstanceDynamic::Create(BaseMaterial, this);
    mat->SetVectorParameterValue(TEXT("FillColor"), FLinearColor::Blue);
	mat->TwoSided = true;
	VectorMaterial = mat;
	return mat;
}
