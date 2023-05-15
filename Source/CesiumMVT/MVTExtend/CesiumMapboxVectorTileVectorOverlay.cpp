// Fill out your copyright notice in the Description page of Project Settings.


#include "CesiumMapboxVectorTileVectorOverlay.h"
#include "Cesium3DTilesSelection/MapboxVectorTileVectorOverlay.h"
#include "CesiumRuntime.h"


std::unique_ptr<Cesium3DTilesSelection::VectorOverlay> UCesiumMapboxVectorTileVectorOverlay::CreateOverlay(
	const Cesium3DTilesSelection::VectorOverlayOptions& options)
{
    Cesium3DTilesSelection::MapboxVectorTileVectorOverlayOptions mvtOptions;

    if (MaximumLevel > MinimumLevel && bSpecifyZoomLevels) {
        mvtOptions.minimumLevel = MinimumLevel;
        mvtOptions.maximumLevel = MaximumLevel;
    }
    return std::make_unique<Cesium3DTilesSelection::MapboxVectorTileVectorOverlay>(
        TCHAR_TO_UTF8(*this->MaterialLayerKey),
        TCHAR_TO_UTF8(*this->Url),
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        mvtOptions,
        options);
}
