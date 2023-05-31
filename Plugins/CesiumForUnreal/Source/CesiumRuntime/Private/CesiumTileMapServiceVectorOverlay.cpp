// Fill out your copyright notice in the Description page of Project Settings.


#include "CesiumTileMapServiceVectorOverlay.h"
#include "Cesium3DTilesSelection/TileMapServiceVectorOverlay.h"
#include "CesiumRuntime.h"


std::unique_ptr<Cesium3DTilesSelection::VectorOverlay> UCesiumTileMapServiceVectorOverlay::CreateOverlay(
	const Cesium3DTilesSelection::VectorOverlayOptions& options)
{
    Cesium3DTilesSelection::TileMapServiceVectorOverlayOptions tmsOptions;

    if (MaximumLevel > MinimumLevel && bSpecifyZoomLevels) {
        tmsOptions.minimumLevel = MinimumLevel;
        tmsOptions.maximumLevel = MaximumLevel;
    }
    return std::make_unique<Cesium3DTilesSelection::TileMapServiceVectorOverlay>(
        TCHAR_TO_UTF8(*this->MaterialLayerKey),
        TCHAR_TO_UTF8(*this->Url),
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        tmsOptions,
        options);
}
