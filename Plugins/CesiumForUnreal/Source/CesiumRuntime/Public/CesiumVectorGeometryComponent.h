// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include <Cesium3DTilesSelection/VectorOverlayTileProvider.h>
#include "Materials/MaterialInterface.h"
#include "CesiumVectorGeometryComponent.generated.h"

namespace CesiumGltf {
struct VectorTile;
}
namespace Cesium3DTilesSelection {
class Tile;
class VectorOverlayTile;
struct BoxExtent;
} // namespace Cesium3DTilesSelection


struct FTileModel;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CESIUMRUNTIME_API UCesiumVectorGeometryComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCesiumVectorGeometryComponent();

     void CreateMesh(const CesiumGltf::VectorTile* pModelData, 
                     Cesium3DTilesSelection::VectorOverlayTile& vectorTile);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
    virtual void BeginDestroy() override;

private:
    UMaterialInterface* createMaterial(const FLinearColor& color);

    void BuildMesh(const FTileModel* tileModel, FString strName);

    void buildPoints(const FTileModel* tileModel, FString strName);

    void buildLines(const FTileModel* tileModel, FString strName);

    void buildPolygons(const FTileModel* tileModel, FString strName);

    glm::dvec3 PixelToWGS84(glm::ivec3& pbfPos,
							int Row,
							int Col,
                            Cesium3DTilesSelection::BoxExtent tileExtent,
							Cesium3DTilesSelection::VectorOverlayTileProvider* provider);

    FTileModel* CreateModel(const CesiumGltf::VectorTile* pModelData,
                             Cesium3DTilesSelection::VectorOverlayTileProvider* provider);
private:
	UPROPERTY(EditAnywhere, Category = "Cesium")
	UStaticMeshComponent* outlineMeshComponent;

	UPROPERTY(EditAnywhere, Category = "Cesium")
	UStaticMeshComponent* polygonMeshComponent;

    UPROPERTY(EditAnywhere, Category = "Cesium")
    UStaticMeshComponent* lineMeshComponent;

    UPROPERTY(EditAnywhere, Category = "Cesium")
	UMaterialInterface* BaseMaterial = nullptr;

    class ACesiumGeoreference* Georeference;
	
};
