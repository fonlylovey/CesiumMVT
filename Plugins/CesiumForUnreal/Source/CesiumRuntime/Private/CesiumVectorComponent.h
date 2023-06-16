#pragma once

#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
#include "Components/LineBatchComponent.h"
#include <glm/mat4x4.hpp>
#include "Cesium3DTilesSelection/BoundingVolume.h"
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

UCLASS()
class UCesiumVectorComponent : public UMeshComponent
{
	GENERATED_BODY()

public:
	static UCesiumVectorComponent* CreateOnGameThread(
		const CesiumGltf::VectorModel& model, 
		AActor* pParentActor,
		const glm::dmat4x4& CesiumToUnrealTransform, 
		UMaterialInterface* BaseMaterial,
		const Cesium3DTilesSelection::BoundingVolume& boundingVolume,
		bool createNavCollision);
	static ULineBatchComponent* CreateTest(const CesiumGltf::VectorModel& model,
										   AActor* pParentActor, 
										   const glm::dmat4x4& CesiumToUnrealTransform);

	UCesiumVectorComponent();
	virtual ~UCesiumVectorComponent();

	void BuildMesh(const CesiumGltf::VectorModel& model);

	void UpdateTransformFromCesium(const glm::dmat4& CesiumToUnrealTransform);

public:
	virtual void BeginDestroy() override;

	//UPrimitiveComponent Interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials,
								  bool bGetDebugMaterials = false) const override;

	//UMeshComponent Interface.
	virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const override;

	//USceneComponent Interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

	void UpdateLocalBounds();

	virtual void SetCollisionEnabled(ECollisionEnabled::Type NewType);

	UPROPERTY(EditAnywhere, Category = "Cesium")
	UMaterialInterface* BaseMaterial = nullptr;
};
