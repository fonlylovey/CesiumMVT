// Fill out your copyright notice in the Description page of Project Settings.


#include "MapmostMap.h"
#include "CesiumRuntime.h"
#include "Cesium3DTilesSelection/MapmostVectorMap.h"
#include "Cesium3DTilesSelection/IMapmostListens.h"
#include "Cesium3DTileset.h"
#include "CesiumWebMapTileServiceVectorOverlay.h"

using namespace CesiumGltf;

//地图图层，所有添加到场景的内容都是一个Layer，类似UE关卡中的Actor
class CESIUMRUNTIME_API FMapmostMapLayer
{
public:
	FMapmostMapLayer()
    {
    }
	~FMapmostMapLayer() = default;

private:
    //图层ID，一个标识
	FString LayerID = TEXT("");
    //数据源名称
    FString Source = "";
    //数据源图层名称（例如一个瓦片有三个图层：Road， Water，Building）
    FString SourceLayer = "";

};


/**
 * @brief 在native当中读取的数据通过void*的方式传递到UE层之后，在这个类当中转换为UE的类型
 */
class CESIUMRUNTIME_API FMapmostListener : public Cesium3DTilesSelection::IMapmostListens
{
public:
	FMapmostListener(ACesium3DTileset* pActor) : _pActor(pActor) {}
	~FMapmostListener() = default;

	virtual void finishLoadStyle(CesiumGltf::MapMetaData* styleData) override
    {
        for (const MapSourceData& source : styleData->sources)
        {
            FString strName = FString(source.sourceName.c_str());
            UCesiumWebMapTileServiceVectorOverlay* wmtsOverlay = NewObject<UCesiumWebMapTileServiceVectorOverlay>(_pActor, FName(*strName));
            wmtsOverlay->bSpecifyZoomLevels = true;
            wmtsOverlay->MinimumLevel = 0;
            wmtsOverlay->MaximumLevel = 20;
            wmtsOverlay->Fill = true;
            wmtsOverlay->FillColor = FLinearColor::Yellow;
            wmtsOverlay->Outline = false;
            wmtsOverlay->LineWidth = 5;
            wmtsOverlay->OutlineColor = FLinearColor::Yellow;
            wmtsOverlay->Url = FString(source.url.c_str());
        }
        
        
        /*
        UCesiumWebMapTileServiceRasterOverlay* wmtsLayer = NewObject<UCesiumWebMapTileServiceRasterOverlay>(actor, FName(layerId + "-" + baseUrl + layer));
        wmtsLayer->Url = baseUrl;
        wmtsLayer->Layer = layer;
        wmtsLayer->MinimumLevel = minimumLevel;
        wmtsLayer->MaximumLevel = maximumLevel;
        wmtsLayer->TileMatrixSet = tileMatrixSet;
        wmtsLayer->Style = style;
        wmtsLayer->Format = format;
        wmtsLayer->MaterialLayerKey = layerId;
        wmtsLayer->SetActive(true);
        wmtsLayer->bSpecifyZoomLevels = true;
        actor->AddInstanceComponent(wmtsLayer);
        wmtsLayer->RegisterComponent();
    */
    }

private:
	ACesium3DTileset* _pActor;
};


UMapmostMap::UMapmostMap(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
    _pMapmostMap = nullptr;
    this->bAutoActivate = true;
}

UMapmostMap::~UMapmostMap()
{
   
}

void UMapmostMap::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    //Refresh();
}

void UMapmostMap::OnAttachmentChanged()
{

}

void UMapmostMap::BeginDestroy()
{
    Super::BeginDestroy();
}

void UMapmostMap::LoadMap()
{
    UE_LOG(LogCesium, Warning, TEXT("Create Mapmost Map..."));
    if(_pMapmostMap == nullptr)
    {
        auto cesiumActor = Cast<ACesium3DTileset>(this->GetOwner());
        assert(cesiumActor != nullptr);
        cesiumActor->GetTileset()->getExternals().pPrepareMapResources;
        Cesium3DTilesSelection::MapmostExternals externals
        {
            std::make_shared<FMapmostListener>(cesiumActor),
            getAsyncSystem(),
            getAssetAccessor()
        };
        _pMapmostMap = MakeUnique<Cesium3DTilesSelection::MapmostVectorMap>(externals);
    }
    if (!MapStyleUrl.IsEmpty())
    {
        _pMapmostMap->CreateMap(TCHAR_TO_UTF8(*MapStyleUrl));
    }
}

void UMapmostMap::Refresh()
{
    this->LoadMap();
}

void UMapmostMap::Activate(bool bReset)
{
    Super::Activate(bReset);
    this->LoadMap();
}

void UMapmostMap::Deactivate()
{
    Super::Deactivate();
}

