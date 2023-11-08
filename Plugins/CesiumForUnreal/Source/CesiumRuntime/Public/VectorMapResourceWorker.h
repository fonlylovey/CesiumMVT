#pragma once

#include "CoreMinimal.h"
#include "Cesium3DTilesSelection/IPrepareVectorMapResources.h"
#include "Cesium3DTileset.h"
#include <string>

/**
 * @brief 在native当中读取的数据通过void*的方式传递到UE层之后，在这个类当中转换为UE的类型
 */
class CESIUMRUNTIME_API VectorMapResourceWorker : public Cesium3DTilesSelection::IPrepareVectorMapResources
{
public:
	VectorMapResourceWorker(ACesium3DTileset* pActor) : _pActor(pActor) {}
	~VectorMapResourceWorker();

	virtual void* prepareVectorInLoadThread(CesiumGltf::VectorTile* pModel, const std::any& rendererOptions) override;

	virtual void* prepareVectorInMainThread(Cesium3DTilesSelection::VectorOverlayTile& vectorTile, void* pLoadThreadResult) override;

	virtual void attachVectorInMainThread(const Cesium3DTilesSelection::Tile& tile, int32_t overlayTextureCoordinateID,
		const Cesium3DTilesSelection::VectorOverlayTile& VectorTile, void* pMainThreadRendererResources,
		const glm::dvec2& translation, const glm::dvec2& scale) override;

	virtual void detachVectorInMainThread(const Cesium3DTilesSelection::Tile& tile, int32_t overlayTextureCoordinateID,
		const Cesium3DTilesSelection::VectorOverlayTile& VectorTile,
		void* pMainThreadRendererResources) noexcept override;

	virtual void freeVector(const Cesium3DTilesSelection::VectorOverlayTile& vectorTile, void* pLoadThreadResult,
		void* pMainThreadResult) noexcept override;

    virtual void setLayers(const std::vector<CesiumGltf::MapLayerData>& laysers);

private:
	ACesium3DTileset* _pActor;
    TMap<FString, CesiumGltf::MapLayerData> _layers;
};
