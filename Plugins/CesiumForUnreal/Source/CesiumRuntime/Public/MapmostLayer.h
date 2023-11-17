// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CesiumVectorOverlay.h"
#include "CesiumGlobeAnchorComponent.h"
#include <CesiumGltf/VectorModel.h>
#include "MapmostLayer.generated.h"


namespace Cesium3DTilesSelection
{
    
}

namespace CesiumGltf
{
    struct MapLayerData;
    struct MapSourceData;
}

/**
 * 
 */
UCLASS()
class CESIUMRUNTIME_API AMapmostLayer  : public AActor 
{
    GENERATED_BODY()
public:
	AMapmostLayer();
	~AMapmostLayer();

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    void CreateLayer(const CesiumGltf::MapSourceData& sourceData, const CesiumGltf::MapLayerData& layerData);

    void CreateLayer(UCesiumVectorOverlay* pOverlay, const CesiumGltf::MapLayerData& layerData);

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mapmost"  )
    FString LayerName = TEXT("");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mapmost")
    bool Visible = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cesium")
    UCesiumGlobeAnchorComponent* GlobeAnchor;

private:
    CesiumGltf::MapLayerData _layerData;
    CesiumGltf::MapSourceData _sourceData;
    UCesiumVectorOverlay* _pMapSource;
};
