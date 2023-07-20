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
	mvtComponent->SetUsingAbsoluteLocation(true);
	mvtComponent->SetMobility(pParentComponent->Mobility);
	mvtComponent->SetFlags(RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
	mvtComponent->BuildMesh(tileModel, TEXT("Mesh_") + tileModel->TileName);
	mvtComponent->RegisterComponent();
	mvtComponent->AttachToComponent(pParentComponent, FAttachmentTransformRules::KeepWorldTransform);
	//mvtComponent->SetVisibility(false, true);
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

	
	//* 测试代码->GetOwner()->GetRootComponent()
	/*
	lineComponent = NewObject<ULineMeshComponent>(this, *strName);
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
		index = index > 9 ? 1 : index;
		lineComponent->CreateLine(sectionIndex, worldPosArray, colors[index]);
		sectionIndex++;
		++index;
	}
	lineComponent->RegisterComponent();
	lineComponent->AttachToComponent(this, FAttachmentTransformRules::KeepWorldTransform);*/
	/*************************/

	
	VectorMesh = NewObject<UStaticMeshComponent>(this, *strName);
	VectorMesh->RegisterComponent();
	VectorMesh->AttachToComponent(this, FAttachmentTransformRules::KeepWorldTransform);

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
	
	int sectionCount = 0;
	FBox3d box;
	box.Init();
	
	for (const FCesiumMeshSection& section : tileModel->Sections)
	{
		int32 count = section.IndexBuffer.Num();
		FStaticMeshSection& staticSection = Sections.AddDefaulted_GetRef();
		staticSection.bEnableCollision = false;
		staticSection.MaterialIndex = 0;
		staticSection.NumTriangles =  count / 3;
		staticSection.FirstIndex = sectionCount;
		staticSection.MinVertexIndex = 0;
		staticSection.MaxVertexIndex = sectionCount + count - 1;

		for (int32 i = 0; i < section.VertexBuffer.Num(); i++)
		{
			
			FStaticMeshBuildVertex vertex;
			vertex.Position = section.VertexBuffer[i];
			box += FVector(vertex.Position);
			vertex.Color = FColor::Red;
			vertex.UVs[0] = FVector2f(0, 0);
			vertex.TangentZ = vertex.Position.GetSafeNormal();
			vertexData.Add(vertex);
		}

		for (int i = 0; i < count; i++)
		{
			int32 theIndex = section.IndexBuffer[i];
			indexs.Add(sectionCount + theIndex);
		}
		sectionCount = sectionCount + count;
	}

	//设置包围盒
	
	FBoxSphereBounds BoundingBoxAndSphere;
	BoundingBoxAndSphere.Origin = FVector(box.GetCenter());
	BoundingBoxAndSphere.BoxExtent = FVector(box.GetExtent());
	BoundingBoxAndSphere.SphereRadius = BoundingBoxAndSphere.BoxExtent.Size();
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
		 //lineComponent->BeginDestroy();
		 //lineComponent = nullptr;
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


namespace
{
	
	FBoxSphereBounds calcBox(const TArray<FVector3f>& vertex)
	{
		float xMin, yMin, xMax, yMax;
		for (int32 i = 0; i < vertex.Num(); i++)
		{
			FVector3f pos = vertex[i];
			xMin = pos.X > xMin ? xMin : pos.X;
			yMin = pos.Y > yMin ? yMin : pos.Y;

			xMax = pos.X <  xMax ? xMax : pos.X;
			yMax = pos.Y < yMax ? yMax : pos.Y;
		}
		FBoxSphereBounds box;
		return box;
	}

}