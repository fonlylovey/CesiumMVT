// Fill out your copyright notice in the Description page of Project Settings.


#include "CesiumWebMapTileServiceVectorOverlay.h"
#include "Cesium3DTilesSelection/WebMapTileServiceVectorOverlay.h"
#include "CesiumRuntime.h"


std::unique_ptr<Cesium3DTilesSelection::VectorOverlay> UCesiumWebMapTileServiceVectorOverlay::CreateOverlay(
	const Cesium3DTilesSelection::VectorOverlayOptions& options)
{
    Cesium3DTilesSelection::WebMapTileServiceVectorOverlayOptions wmtsOptions;

    if (MaximumLevel > MinimumLevel && bSpecifyZoomLevels) {
        wmtsOptions.minimumLevel = MinimumLevel;
        wmtsOptions.maximumLevel = MaximumLevel;
    }
    wmtsOptions.isFill = Fill;
    wmtsOptions.fillColor = glm::dvec4(FillColor.R, FillColor.G, FillColor.B, FillColor.A);
    wmtsOptions.isOutline = Outline;
    wmtsOptions.lineWidth = LineWidth;
    wmtsOptions.outlineColor = glm::dvec4(OutlineColor.R, OutlineColor.G, OutlineColor.B, OutlineColor.A);
    wmtsOptions.layers = TCHAR_TO_UTF8(*LayerName);
    wmtsOptions.sourceName = TCHAR_TO_UTF8(*SourceName);
    return std::make_unique<Cesium3DTilesSelection::WebMapTileServiceVectorOverlay>(
        TCHAR_TO_UTF8(*this->MaterialLayerKey),
        TCHAR_TO_UTF8(*this->Url),
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        wmtsOptions,
        options);
}
