// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CesiumVectorOverlay.h"
#include "CesiumXYZVectorOverlay.generated.h"

/**
 * 
 */
UCLASS(ClassGroup=(Cesium), meta=(BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumXYZVectorOverlay : public UCesiumVectorOverlay
{
	GENERATED_BODY()

protected:
    virtual std::unique_ptr<Cesium3DTilesSelection::VectorOverlay> CreateOverlay(
              const Cesium3DTilesSelection::VectorOverlayOptions& options = {}) override;

public:
	/**
   * ��Ƭ�����url��ַ
   */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
	FString Url;

	/*
	 * ���Ϊ true����ֱ��ָ���������ṩ����С��������ż���
	 * ���Ϊ false����ӷ������� url �ĵ����Զ�ȷ����С��������ż���
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
	bool bSpecifyZoomLevels = false;

    /*
	 * �Ƿ���Ҫ����
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
	bool bDecode = false;

	/**
   * ��Ƭ��ͼ��С����
   */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Cesium",
		meta = (EditCondition = "bSpecifyZoomLevels", ClampMin = 0))
	int32 MinimumLevel = 0;

	/**
	 * ��Ƭ��ͼ��󼶱�
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Cesium",
		meta = (EditCondition = "bSpecifyZoomLevels", ClampMin = 0))
	int32 MaximumLevel = 20;

    UPROPERTY()
    FString SourceName;
	
};
