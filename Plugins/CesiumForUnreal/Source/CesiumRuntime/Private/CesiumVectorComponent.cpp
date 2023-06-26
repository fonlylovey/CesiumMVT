#include "CesiumVectorComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Cesium3DTilesSelection/VectorOverlayTile.h"
#include "CesiumMeshSceneProxy.h"

UCesiumVectorComponent* UCesiumVectorComponent::CreateOnGameThread(
	const CesiumGltf::VectorModel& model,
	AActor* pParentActor,
	const glm::dmat4x4& cesiumToUnrealTransform,
	UMaterialInterface* pBaseMaterial,
	const Cesium3DTilesSelection::BoundingVolume& boundingVolume,
	bool createNavCollision)
{
	UCesiumVectorComponent* mvtComponent = NewObject<UCesiumVectorComponent>(pParentActor);
	mvtComponent->SetUsingAbsoluteLocation(true);
	mvtComponent->SetMobility(pParentActor->GetRootComponent()->Mobility);
	mvtComponent->SetFlags(RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
	mvtComponent->SetVisibility(false, true);
	mvtComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if(IsValid(pBaseMaterial))
	{
		mvtComponent->BaseMaterial = pBaseMaterial;
	}
	mvtComponent->BuildMesh(model);

	return mvtComponent;
}

// 设置默认值
UCesiumVectorComponent::UCesiumVectorComponent()
{
	
}

UCesiumVectorComponent::~UCesiumVectorComponent()
{

}

void UCesiumVectorComponent::BuildMesh(const CesiumGltf::VectorModel& model)
{
	for (const CesiumGltf::VectorLayer& layer : model.layers)
	{

	}
}

void UCesiumVectorComponent::UpdateTransformFromCesium(const glm::dmat4& CesiumToUnrealTransform)
{

}

void UCesiumVectorComponent::BeginDestroy()
{

}

FPrimitiveSceneProxy* UCesiumVectorComponent::CreateSceneProxy()
{
	return new FLineMeshSceneProxy(this);
}

void UCesiumVectorComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials /*= false*/) const
{
	
}

UMaterialInterface* UCesiumVectorComponent::GetMaterial(int32 ElementIndex) const
{
    return BaseMaterial;
}

FBoxSphereBounds UCesiumVectorComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FLineMeshSceneProxy* LineMeshSceneProxy = (FLineMeshSceneProxy*)SceneProxy;

    FBoxSphereBounds LocalBounds(FBoxSphereBounds3f(FVector3f(0, 0, 0), FVector3f(0, 0, 0), 0));
    if (LineMeshSceneProxy != nullptr)
    {
        LocalBounds = LineMeshSceneProxy->GetLocalBounds();
    }

    FBoxSphereBounds Ret(FBoxSphereBounds(LocalBounds).TransformBy(LocalToWorld));

    Ret.BoxExtent *= BoundsScale;
    Ret.SphereRadius *= BoundsScale;

    return Ret;
}

void UCesiumVectorComponent::UpdateLocalBounds()
{

}

void UCesiumVectorComponent::SetCollisionEnabled(ECollisionEnabled::Type NewType)
{

}