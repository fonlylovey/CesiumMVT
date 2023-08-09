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

	void BuildMesh(const FTileModel* tileModel, FString strName);

	virtual void BeginDestroy() override;

	UMaterialInterface* createMaterial(const FLinearColor& color);

	UPROPERTY(EditAnywhere, Category = "Cesium")
	UMaterialInterface* BaseMaterial = nullptr;

	UPROPERTY(EditAnywhere, Category = "Cesium")
	UMaterialInterface* VectorMaterial = nullptr;

	UPROPERTY(EditAnywhere, Category = "Cesium")
	UStaticMeshComponent* lineMeshComponent;

	UPROPERTY(EditAnywhere, Category = "Cesium")
	UStaticMeshComponent* polygonMeshComponent;
	bool isAttach = false;

    int Level = 0;
    int Row = 0;
    int Col = 0;
};
