// Copyright 2021-2023 MapMost for UE, Inc. and Contributors

#include "MMTiandiTuTilesRasterOverlay.h"
#include"Cesium3DTilesSelection/TiandiTuRasterOverlay.h"
#include "CesiumRuntime.h"

std::unique_ptr<Cesium3DTilesSelection::RasterOverlay>
UMapMostTiandiTuRasterOverlay::CreateOverlay(
    const Cesium3DTilesSelection::RasterOverlayOptions& options) {
  Cesium3DTilesSelection::TiandiTuRasterOverlayOptions TiandiTuOptions;
  if (MaximumLevel > MinimumLevel && bSpecifyZoomLevels) {
    TiandiTuOptions.minimumLevel = MinimumLevel;
    TiandiTuOptions.maximumLevel = MaximumLevel;
  }
  switch (this->MapStyle) {
  case ETiandiTuStyle::VectorStyle:
    TiandiTuOptions.MapStyle="vec";
    break;
  case ETiandiTuStyle::VectorAnnotationStyle:
    TiandiTuOptions.MapStyle="cva";
    break;
  case ETiandiTuStyle::AerialStyle:
    TiandiTuOptions.MapStyle="img";
    break;
  case ETiandiTuStyle::AerialAnnotationStyle:
    TiandiTuOptions.MapStyle="cia";
    break;
  case ETiandiTuStyle::TerrainShadingStyle:
    TiandiTuOptions.MapStyle="ter";
    break;
  case ETiandiTuStyle::TerrainAnnotationStyle:
    TiandiTuOptions.MapStyle="cta";
    break;
  case ETiandiTuStyle::GlobalNationalBoundariesStyle:
    TiandiTuOptions.MapStyle="ibo";
    break;
  case ETiandiTuStyle::EN_VectorAnnotationStyle:
    TiandiTuOptions.MapStyle="eva";
    break;
  case ETiandiTuStyle::EN_AerialAnnotationStyle:
    TiandiTuOptions.MapStyle="eia";
    break;
  }

  TiandiTuOptions.token=TCHAR_TO_UTF8(*Token);
  return std::make_unique<Cesium3DTilesSelection::TiandiTuRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      std::vector<CesiumAsync::IAssetAccessor::THeader>(),
     TiandiTuOptions,
      options);
	return 0;
}

