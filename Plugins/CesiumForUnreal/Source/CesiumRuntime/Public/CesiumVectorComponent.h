#pragma once

#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
#include "Components/LineBatchComponent.h"
#include <glm/mat4x4.hpp>
#include "Cesium3DTilesSelection/BoundingVolume.h"
#include "LineMeshComponent.h"
#include "CesiumVectorComponent.generated.h"

class UMaterialInterface;

namespace CesiumGltf {
struct VectorModel;
}

namespace Cesium3DTilesSelection {
class Tile;
class VectorOverlayTile;
} // namespace Cesium3DTilesSelection

class FPrimitiveSceneProxy;
class FLineMeshSceneProxy;
struct FCesiumMeshSection;
struct FTileModel;

UCLASS(BlueprintType, Blueprintable)
class CESIUMRUNTIME_API UCesiumVectorComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	static UCesiumVectorComponent* CreateOnGameThread(
        const CesiumGltf::VectorModel* pModelData,
        Cesium3DTilesSelection::VectorOverlayTile& vectorTile,
        AActor* pOwner);

	UCesiumVectorComponent(const FObjectInitializer& ObjectInitializer);
	virtual ~UCesiumVectorComponent();

	virtual void BeginDestroy() override;

private:
	void BuildMesh(const FTileModel* tileModel, FString strName);

	UMaterialInterface* createMaterial(const FLinearColor& color);

    void buildPoints(const FTileModel* tileModel, FString strName);

    void buildLines(const FTileModel* tileModel, FString strName);

    void buildPolygons(const FTileModel* tileModel, FString strName);

public:
	UPROPERTY(EditAnywhere, Category = "Cesium")
	UMaterialInterface* BaseMaterial = nullptr;

	UPROPERTY(EditAnywhere, Category = "Cesium")
	UMaterialInterface* VectorMaterial = nullptr;

	UPROPERTY(EditAnywhere, Category = "Cesium")
	UStaticMeshComponent* outlineMeshComponent;

	UPROPERTY(EditAnywhere, Category = "Cesium")
	UStaticMeshComponent* polygonMeshComponent;

    UPROPERTY(EditAnywhere, Category = "Cesium")
    ULineMeshComponent* lineMeshComponent;

	bool isAttach = false;

    int Level = 0;
    int Row = 0;
    int Col = 0;
};
