// Fill out your copyright notice in the Description page of Project Settings.


#include "MapmostLayer.h"
#include "CesiumWebMapTileServiceVectorOverlay.h"

AMapmostLayer::AMapmostLayer()
{
}

AMapmostLayer::~AMapmostLayer()
{
}

void AMapmostLayer::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    this->Visible = Visible;
}

void AMapmostLayer::CreateLayer(const CesiumGltf::MapSourceData& sourceData, const CesiumGltf::MapLayerData& layerData)
{
   _layerData = layerData;
    
    FString strSourceLayerName = FString(layerData.sourceLayer.c_str());
    LayerName = FString(layerData.id.c_str());
}

void AMapmostLayer::CreateLayer(UCesiumVectorOverlay* pOverlay, const CesiumGltf::MapLayerData& layerData)
{
    _layerData = layerData;
    _pMapSource = pOverlay;
    //_pMapSource->SetActive(true);
}
