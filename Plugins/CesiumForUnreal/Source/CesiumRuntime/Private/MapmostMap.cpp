// Fill out your copyright notice in the Description page of Project Settings.


#include "MapmostMap.h"
#include "CesiumRuntime.h"
#include "Cesium3DTilesSelection/MapmostVectorMap.h"
#include "Cesium3DTilesSelection/IMapmostListens.h"
#include "Cesium3DTileset.h"
#include "CesiumWebMapTileServiceVectorOverlay.h"
#include "MapmostLayer.h"

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
	FMapmostListener(UMapmostMap* pMapmost) : _pMapmost(pMapmost) {}
	~FMapmostListener() = default;

	virtual void finishLoadStyle(CesiumGltf::MapMetaData* styleData) override
    {
        for (const MapSourceData& source : styleData->sources)
        {
            _pMapmost->AddSource(source);
        }

        _pMapmost->GetCesiumActor()->GetTileset()->getExternals().pPrepareMapResources->setLayers(styleData->laysers);
        for (const MapLayerData& layer : styleData->laysers)
        {
            _pMapmost->AddLayer(layer);
        }
    }

private:
	UMapmostMap* _pMapmost;
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
    _pCesiumActor = Cast<ACesium3DTileset>(this->GetOwner());
    assert(_pCesiumActor != nullptr);
}

void UMapmostMap::BeginDestroy()
{
    Super::BeginDestroy();
}

ACesium3DTileset* UMapmostMap::GetCesiumActor()
{
    return _pCesiumActor;
}

void UMapmostMap::LoadMapMetaStyle()
{
    UE_LOG(LogCesium, Warning, TEXT("Create Mapmost Map..."));
    if(_pMapmostMap == nullptr)
    {
        Cesium3DTilesSelection::MapmostExternals externals
        {
            std::make_shared<FMapmostListener>(this),
            getAsyncSystem(),
            getAssetAccessor()
        };
        _pMapmostMap = MakeUnique<Cesium3DTilesSelection::MapmostVectorMap>(externals);
    }
    if (!MapStyleUrl.IsEmpty())
    {
        _pMapmostMap->loadMetaStyle(TCHAR_TO_UTF8(*MapStyleUrl));
    }
}

void UMapmostMap::DestorMapStyle()
{
    SourceDict.Empty();
    LayerDict.Empty();
    
}

void UMapmostMap::Refresh()
{
    //this->LoadMapMetaStyle();
}

void UMapmostMap::AddSource(const CesiumGltf::MapSourceData& sourceData)
{
    FString strName = FString(sourceData.sourceName.c_str());
    SourceDict.Add(strName, sourceData);

     //会修改成xyz的方式
    UCesiumWebMapTileServiceVectorOverlay* pOverlay = NewObject<UCesiumWebMapTileServiceVectorOverlay>(_pCesiumActor, FName(*strName));
    pOverlay->bSpecifyZoomLevels = true;
    pOverlay->MinimumLevel = 0;
    pOverlay->MaximumLevel = 20;
    pOverlay->Fill = true;
    pOverlay->FillColor = FLinearColor::Yellow;
    pOverlay->Outline = false;
    pOverlay->LineWidth = 5;
    pOverlay->OutlineColor = FLinearColor::Yellow;
    pOverlay->Url = FString(sourceData.url.c_str());
    pOverlay->SourceName = FString(sourceData.sourceName.c_str());
    pOverlay->LayerName = TEXT("YQMap");
    pOverlay->SetActive(false);
    pOverlay->RegisterComponent();
    _pCesiumActor->AddInstanceComponent(pOverlay);
    SourceOverlayDict.Add(strName, pOverlay);
}

void UMapmostMap::AddSource(const FString& SourceName, const FString& SourceType, const FString& SourceURL)
{

}

void UMapmostMap::AddLayer(const CesiumGltf::MapLayerData& layerData)
{
    FString strSourceName = FString(layerData.source.c_str());
    FString strLayerName = FString(layerData.id.c_str());

    LayerDict.Add(strLayerName, layerData);

    auto sourceData = SourceDict.FindRef(strSourceName);
    auto pOverlay = SourceOverlayDict.FindRef(strSourceName);
    FActorSpawnParameters info;
    info.Name = FName(strLayerName);
    info.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    //AMapmostLayer* layer = GetWorld()->SpawnActor<AMapmostLayer>(info);
    //layer->CreateLayer(pOverlay, layerData);

}

void UMapmostMap::AddLayer(const FString& LayerID, const FString& LayerType)
{

}

void UMapmostMap::Activate(bool bReset)
{
    Super::Activate(bReset);
    this->Refresh();
}

void UMapmostMap::Deactivate()
{
    Super::Deactivate();
}

