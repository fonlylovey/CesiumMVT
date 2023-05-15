// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CesiumVectorOverlay.h"
#include "CoreMinimal.h"
#include "CesiumMapboxVectorTileVectorOverlay.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RIGELGIS_API UCesiumMapboxVectorTileVectorOverlay : public UCesiumVectorOverlay
{
	GENERATED_BODY()

protected:
      virtual std::unique_ptr<Cesium3DTilesSelection::VectorOverlay> CreateOverlay(
          const Cesium3DTilesSelection::VectorOverlayOptions& options = {}) override;

public:
	/**
   * 瓦片服务的url地址
   */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
	FString Url;

	/*
	 * 如果为 true，则直接指定服务器提供的最小和最大缩放级别，
	 * 如果为 false，则从服务器的 url 文档中自动确定最小和最大缩放级别。
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
	bool bSpecifyZoomLevels = false;

	/**
   * 瓦片地图最小级别
   */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Cesium",
		meta = (EditCondition = "bSpecifyZoomLevels", ClampMin = 0))
	int32 MinimumLevel = 0;

	/**
	 * 瓦片地图最大级别
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Cesium",
		meta = (EditCondition = "bSpecifyZoomLevels", ClampMin = 0))
	int32 MaximumLevel = 10;
};
