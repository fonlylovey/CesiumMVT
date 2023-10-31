// Copyright 2021-2023 MapMost for UE, Inc. and Contributors

#pragma once

#include "CesiumRasterOverlay.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "MMTiandiTuTilesRasterOverlay.generated.h"

UENUM(BlueprintType)
enum class ETiandiTuStyle : uint8 {
  VectorStyle UMETA(DisplayName = "矢量底图"),
  VectorAnnotationStyle UMETA(DisplayName = "矢量注记"),
  AerialStyle UMETA(DisplayName = "影像底图"),
  AerialAnnotationStyle UMETA(DisplayName = "影像注记"),
  TerrainShadingStyle UMETA(DisplayName = "地形晕渲"),
  TerrainAnnotationStyle UMETA(DisplayName = "地形注记"),
  GlobalNationalBoundariesStyle UMETA(DisplayName = "全球境界"),
  EN_VectorAnnotationStyle UMETA(DisplayName = "矢量英文注记"),
  EN_AerialAnnotationStyle UMETA(DisplayName = "影像英文注记"),
};
/**
 * A raster overlay that directly accesses Amap Tiles server.
 */
UCLASS(ClassGroup = (Cesium), meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UMapMostTiandiTuRasterOverlay
    : public UCesiumRasterOverlay {
  GENERATED_BODY()

public:

  /**
 * The MapStyle of TiandiTu.
 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MapMost")
  ETiandiTuStyle MapStyle = ETiandiTuStyle::AerialStyle;


  /**
   * The token of TiandiTu.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MapMost")
  FString Token ="";
  /**
   * True to directly specify minum and maximum zoom levels available from the
   * server, or false to automatically determine the minimum and maximum zoom
   * levels from the default setting.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MapMost")
  bool bSpecifyZoomLevels = false;

  /**
   * Minimum zoom level.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "MapMost",
      meta = (EditCondition = "bSpecifyZoomLevels", ClampMin = 0))
  int32 MinimumLevel = 0;

  /**
   * Maximum zoom level.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "MapMost",
      meta = (EditCondition = "bSpecifyZoomLevels", ClampMin = 0))
  int32 MaximumLevel = 18;

protected:
  virtual std::unique_ptr<Cesium3DTilesSelection::RasterOverlay> CreateOverlay(
      const Cesium3DTilesSelection::RasterOverlayOptions& options = {})
      override;
};
