// Fill out your copyright notice in the Description page of Project Settings.


#include "CesiumXYZVectorOverlay.h"
#include "Cesium3DTilesSelection/XYZVectorOverlay.h"

std::unique_ptr<Cesium3DTilesSelection::VectorOverlay> UCesiumXYZVectorOverlay::CreateOverlay(
	const Cesium3DTilesSelection::VectorOverlayOptions& options)
{
    Cesium3DTilesSelection::XYZVectorOverlayOptions xyzOptions;

    if (MaximumLevel > MinimumLevel && bSpecifyZoomLevels) {
        xyzOptions.minimumLevel = MinimumLevel;
        xyzOptions.maximumLevel = MaximumLevel;
    }
    xyzOptions.decode = bDecode;
    xyzOptions.sourceName = TCHAR_TO_UTF8(*SourceName);
    return std::make_unique<Cesium3DTilesSelection::XYZVectorOverlay>(
        TCHAR_TO_UTF8(*this->MaterialLayerKey),
        TCHAR_TO_UTF8(*this->Url),
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        xyzOptions,
        options);
}