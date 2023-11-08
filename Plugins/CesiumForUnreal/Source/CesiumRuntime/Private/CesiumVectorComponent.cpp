#include "CesiumVectorComponent.h"
#include "Cesium3DTilesSelection/VectorOverlayTileProvider.h"
#include "GameFramework/Actor.h"
#include "CesiumVectorGeometryComponent.h"
#include "CesiumVectorRasterizerComponent.h"
#include "CesiumGeoreference.h"


UCesiumVectorComponent* UCesiumVectorComponent::CreateOnGameThread(
	const CesiumGltf::VectorTile* pModelData,
    Cesium3DTilesSelection::VectorOverlayTile& vectorTile,
	AActor* pOwner)
{
    auto provider = &vectorTile.getTileProvider();
    if (provider == nullptr)
    {
        return nullptr;
    }

    bool isRaster = true;

    UCesiumVectorComponent* mvtComponent = NewObject<UCesiumVectorComponent>(pOwner);
    mvtComponent->SetVisibility(true, true);
    mvtComponent->RegisterComponent();
    mvtComponent->AttachToComponent(pOwner->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
    mvtComponent->Level = pModelData->level;
    mvtComponent->Row = pModelData->row;
    mvtComponent->Col = pModelData->col;
    mvtComponent->SetUsingAbsoluteLocation(true);
    mvtComponent->SetMobility(EComponentMobility::Static);
    mvtComponent->SetFlags(RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
    if(pModelData->level == 3 && pModelData->row == 2 && pModelData->col == 13)
    {
        int a = 0;
    }
    if(isRaster)
    {
        mvtComponent->RenderRaster(pModelData);
    }
    else
    {
       
        mvtComponent->RenderGeometry(pModelData, vectorTile);
    }

   
    
	return mvtComponent;
}

// 设置默认值
UCesiumVectorComponent::UCesiumVectorComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
  
}

UCesiumVectorComponent::~UCesiumVectorComponent()
{

}


void UCesiumVectorComponent::BeginDestroy()
{
	 Super::BeginDestroy();
	 if(IsValid(RasterComponent))
     {
         RasterComponent->ConditionalBeginDestroy();
     }
     if(IsValid(GeometryComponent))
     {
         GeometryComponent->ConditionalBeginDestroy();
     }
}

void UCesiumVectorComponent::RenderRaster(const CesiumGltf::VectorTile* pModelData)
{
    //计算地图的变化范围
    auto Geo = ACesiumGeoreference::GetDefaultGeoreference(this->GetOwner());
    FVector uePos = Geo->TransformLongitudeLatitudeHeightToUnreal(FVector(pModelData->extentMin.x, pModelData->extentMin.y, 0));
    UCesiumVectorRasterizerComponent* rasterComponent = NewObject<UCesiumVectorRasterizerComponent>(this);
    rasterComponent->CreateDecal(pModelData);
    rasterComponent->SetWorldLocation(uePos);
}

void UCesiumVectorComponent::RenderGeometry(const CesiumGltf::VectorTile* pModelData,
        Cesium3DTilesSelection::VectorOverlayTile& vectorTile)
{
    UCesiumVectorGeometryComponent* geomComponent = NewObject<UCesiumVectorGeometryComponent>(this);
    geomComponent->CreateMesh(pModelData, vectorTile);
}
