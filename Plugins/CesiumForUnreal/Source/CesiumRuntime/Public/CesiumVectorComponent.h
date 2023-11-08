#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "CesiumVectorComponent.generated.h"

class UMaterialInterface;

namespace CesiumGltf {
struct VectorTile;
}

namespace Cesium3DTilesSelection {
class VectorOverlayTile;
} // namespace Cesium3DTilesSelection

struct FTileModel;

UCLASS(BlueprintType, Blueprintable)
class CESIUMRUNTIME_API UCesiumVectorComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	static UCesiumVectorComponent* CreateOnGameThread(
        const CesiumGltf::VectorTile* pModelData,
        Cesium3DTilesSelection::VectorOverlayTile& vectorTile,
        AActor* pOwner);

	UCesiumVectorComponent(const FObjectInitializer& ObjectInitializer);
	virtual ~UCesiumVectorComponent();

	virtual void BeginDestroy() override;

private:
    // using Raster way render
    void RenderRaster(const CesiumGltf::VectorTile* pModelData);

    // using geometry way render
    void RenderGeometry(const CesiumGltf::VectorTile* pModelData,
                      Cesium3DTilesSelection::VectorOverlayTile& vectorTile);

    // using Tencil Volume Shadow way render
    void RenderSVS() { 
    //none
    };

    class UCesiumVectorRasterizerComponent* RasterComponent;
    class UCesiumVectorRasterizerComponent* GeometryComponent;
public:
    int Level = 0;
    int Row = 0;
    int Col = 0;

};
