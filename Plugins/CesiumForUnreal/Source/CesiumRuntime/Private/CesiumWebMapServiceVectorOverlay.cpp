// Fill out your copyright notice in the Description page of Project Settings.


#include "CesiumWebMapServiceVectorOverlay.h"
#include "Cesium3DTilesSelection/WebMapServiceVectorOverlay.h"
#include "CesiumRuntime.h"


std::unique_ptr<Cesium3DTilesSelection::VectorOverlay> UCesiumWebMapServiceVectorOverlay::CreateOverlay(
	const Cesium3DTilesSelection::VectorOverlayOptions& options)
{
    Cesium3DTilesSelection::WebMapServiceVectorOverlayOptions wmsOptions;

    if (MaximumLevel > MinimumLevel && bSpecifyZoomLevels) {
        wmsOptions.minimumLevel = MinimumLevel;
        wmsOptions.maximumLevel = MaximumLevel;
    }
    return std::make_unique<Cesium3DTilesSelection::WebMapServiceVectorOverlay>(
        TCHAR_TO_UTF8(*this->MaterialLayerKey),
        TCHAR_TO_UTF8(*this->Url),
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        wmsOptions,
        options);
}
