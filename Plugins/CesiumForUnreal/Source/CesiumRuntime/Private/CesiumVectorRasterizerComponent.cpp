// Fill out your copyright notice in the Description page of Project Settings.


#include "CesiumVectorRasterizerComponent.h"
#include "Materials/MaterialInterface.h"
#include <Cesium3DTilesSelection/VectorOverlayTileProvider.h>
#include "Cesium3DTilesSelection/MVTUtilities.h"
#include <glm/glm.hpp>
#include "Engine/Texture2D.h"
#include "VectorRasterizer.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
    double Clamp(double value, double min, double max)
    {
        value = value > max ? max : value;
        value = value < min ? min : value;
        return value;
    }
}

// Sets default values for this component's properties
UCesiumVectorRasterizerComponent::UCesiumVectorRasterizerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
    FString str = TEXT("/CesiumForUnreal/Materials/MVT/M_VectorDecalBase.M_VectorDecalBase");
    BaseMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, *str));
}

void UCesiumVectorRasterizerComponent::CreateDecal(const CesiumGltf::VectorTile* pModelData)
{
    int tileWidth = 4096;
}

// Called when the game starts
void UCesiumVectorRasterizerComponent::BeginPlay()
{
	Super::BeginPlay();

}

UMaterialInterface* UCesiumVectorRasterizerComponent::createMaterial(UTexture2D* texture)
{
    UMaterialInstanceDynamic* mat = UMaterialInstanceDynamic::Create(BaseMaterial, this);
    mat->SetTextureParameterValue(TEXT("DecalTexture"), texture);
    mat->TwoSided = true;
	return mat;
}

void UCesiumVectorRasterizerComponent::BuildLines(std::vector<std::vector<glm::ivec2>>& buffers)
{
    int width = 256;
    UTexture2D* texture = UTexture2D::CreateTransient(width, width, EPixelFormat::PF_B8G8R8A8);
    texture->AddToRoot();

    UMaterialInterface* matDecal = createMaterial(texture);
}

