#include "CesiumVectorComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Cesium3DTilesSelection/VectorOverlayTile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "CustomMeshComponent.h"

UCesiumVectorComponent* UCesiumVectorComponent::CreateOnGameThread(
	const FTileModel* tileModel,
	USceneComponent* pParentComponent)
{
	UCesiumVectorComponent* mvtComponent = NewObject<UCesiumVectorComponent>(pParentComponent, *tileModel->TileName);
	mvtComponent->RegisterComponent();
	mvtComponent->SetUsingAbsoluteLocation(true);
	mvtComponent->SetMobility(pParentComponent->Mobility);
	mvtComponent->SetFlags(RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
	mvtComponent->SetVisibility(false, true);
	mvtComponent->BuildMesh(tileModel, TEXT("Mesh_") + tileModel->TileName);
	return mvtComponent;
}

// 设置默认值
UCesiumVectorComponent::UCesiumVectorComponent()
{
	// Structure to hold one-time initialization
  struct FConstructorStatics 
  {
    ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMaterial;
    ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseTranslucency;

    FConstructorStatics() : 
		BaseMaterial(TEXT("/CesiumForUnreal/Materials/MVT/M_VectorBase.M_VectorBase")),
		BaseTranslucency(TEXT("/CesiumForUnreal/Materials/MVT/M_VectorBase.M_VectorBase"))
	{
    }
  };
  static FConstructorStatics ConstructorStatics;
  FString str = TEXT("/CesiumForUnreal/Materials/MVT/M_VectorBase.M_VectorBase");
  UMaterialInterface* aaa = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, *str));
  this->BaseMaterial = aaa;
  this->BaseTranslucencyMaterial = aaa;
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
	*/
	lineComponent = NewObject<ULineMeshComponent>(this, *strName);
	lineComponent->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
	lineComponent->RegisterComponent();
	lineComponent->Material = BaseMaterial;
	//lineComponent->SetVisibility(false);

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
	/*************************/

	/*
	VectorComponent = NewObject<UStaticMeshComponent>(this, *strName);
	VectorComponent->AttachToComponent(this->GetOwner()->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	VectorComponent->RegisterComponent();

	UStaticMesh* pStaticMesh = NewObject<UStaticMesh>(VectorComponent);
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
	VectorComponent->SetStaticMesh(pStaticMesh);
	*/
}

void UCesiumVectorComponent::BeginDestroy()
{
	 Super::BeginDestroy();
	 if(lineComponent != nullptr)
	 {
		 lineComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		 lineComponent->DestroyComponent();
	 }
}


void UCesiumVectorComponent::OnVisibilityChanged()
{
	if (lineComponent != nullptr)
	{
		//bool isBisble = lineComponent->IsVisible();
		//lineComponent->SetVisibility(isBisble);
	}

	UE_LOG(LogTemp, Error, TEXT("%s"), *GetName());
	FString strMsg = IsVisible() ? TEXT("显示 : ") : TEXT("隐藏：");
	strMsg += TEXT("Name : ") + GetName();
	GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, strMsg);
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
