// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CesiumVectorOverlay.h"

#include "Cesium3DTilesSelection/MapmostVectorMap.h"
#include "MapmostLayer.h"
#include "MapmostMap.generated.h"

namespace Cesium3DTilesSelection
{
    MapmostExternals;
}

class ACesium3DTileset;

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

    virtual void Activate(bool bReset) override;

    virtual void Deactivate() override;


    ACesium3DTileset* GetCesiumActor();

    void LoadMapMetaStyle();

    void DestorMapStyle();

    void Refresh();

    UFUNCTION(BlueprintCallable, Category = "Mapmost")
    void AddSource(const FString& SourceName, const FString& SourceType, const FString& SourceURL);

    void AddSource(const CesiumGltf::MapSourceData& sourceData);

    UFUNCTION(BlueprintCallable, Category = "Mapmost")
    void AddLayer(const FString& LayerID, const FString& LayerType);

    void AddLayer(const CesiumGltf::MapLayerData& layerData);

	/**
   * Map Style url
   */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mapmost")
	FString MapStyleUrl;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mapmost")
    TArray<AMapmostLayer*> MapLayers;

private:
    ACesium3DTileset* _pCesiumActor;
    TUniquePtr<Cesium3DTilesSelection::MapmostVectorMap> _pMapmostMap;
    TMap<FString, UCesiumVectorOverlay*> SourceOverlayDict;
    TMap<FString, CesiumGltf::MapSourceData> SourceDict;
    TMap<FString, CesiumGltf::MapLayerData> LayerDict;
};
