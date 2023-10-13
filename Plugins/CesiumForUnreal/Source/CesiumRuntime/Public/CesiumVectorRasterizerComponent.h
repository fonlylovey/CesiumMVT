// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include <vector>
#include <glm/glm.hpp>
#include "CesiumVectorRasterizerComponent.generated.h"

namespace CesiumGltf {
struct VectorModel;
}

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CESIUMRUNTIME_API UCesiumVectorRasterizerComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCesiumVectorRasterizerComponent();

    void CreateDecal(const CesiumGltf::VectorModel* pModelData);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
    UMaterialInterface* createMaterial(UTexture2D* texture);

    void BuildLines(std::vector<std::vector<glm::ivec2>>& buffers);

private:
    UPROPERTY(EditAnywhere, Category = "Cesium")
	UMaterialInterface* BaseMaterial = nullptr;

    UPROPERTY(EditAnywhere, Category = "Cesium")
    class UDecalComponent* decalComponent;
};
