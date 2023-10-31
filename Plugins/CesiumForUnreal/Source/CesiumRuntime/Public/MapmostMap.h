// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CesiumVectorOverlay.h"
#include "CoreMinimal.h"

#include "Cesium3DTilesSelection/MapmostVectorMap.h"
#include "MapmostMap.generated.h"

namespace Cesium3DTilesSelection {
    
} 

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UMapmostMap : public USceneComponent
{
	GENERATED_BODY()

public:
    UMapmostMap(const FObjectInitializer& ObjectInitializer);
    virtual ~UMapmostMap();

#if WITH_EDITOR
  // Called when properties are changed in the editor
  virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    virtual void OnAttachmentChanged() override;

    virtual void BeginDestroy() override;

    void LoadMap();

      UFUNCTION(BlueprintCallable, Category = "Mapmost")
    void Refresh();

    virtual void Activate(bool bReset) override;

    virtual void Deactivate() override;
	/**
   * Map Style url
   */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mapmost")
	FString MapStyleUrl;
private:
    TUniquePtr<Cesium3DTilesSelection::MapmostVectorMap> _pMapmostMap;
};
