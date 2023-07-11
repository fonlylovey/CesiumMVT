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

UCLASS()
class UCesiumVectorComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	static UCesiumVectorComponent* CreateOnGameThread(
	const FTileModel* tileModel,
	USceneComponent* pParentComponent);

	UCesiumVectorComponent();
	virtual ~UCesiumVectorComponent();

	void BuildMesh(const FTileModel* tileModel, FString strName);

	virtual void BeginDestroy() override;

	void OnVisibilityChanged() override;

	UMaterialInterface* createMaterial();

	UPROPERTY(EditAnywhere, Category = "Cesium")
	UMaterialInterface* BaseMaterial = nullptr;

	UPROPERTY(EditAnywhere, Category = "Cesium")
	UMaterialInterface* VectorMaterial = nullptr;

	UPROPERTY(EditAnywhere, Category = "Cesium")
	ULineMeshComponent* lineComponent;
};
