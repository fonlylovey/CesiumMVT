#pragma once

#include "CoreMinimal.h"
#include "Cesium3DTilesSelection/IRendererResourcesWorker.h"
#include "Cesium3DTileset.h"

/**
 * @brief 在native当中读取的数据通过void*的方式传递到UE层之后，在这个类当中转换为UE的类型
 */
class CESIUMRUNTIME_API VectorResourceWorker : public Cesium3DTilesSelection::IRendererResourcesWorker
{
public:
	VectorResourceWorker(ACesium3DTileset* pActor) : _pActor(pActor) {}
	~VectorResourceWorker() = default;

	virtual void* prepareInMainThread(Cesium3DTilesSelection::Tile& tile, void* pLoadThreadResult) override;

	virtual CesiumAsync::Future<Cesium3DTilesSelection::VectorTileLoadResultAndRenderResources>
				prepareInLoadThread(const CesiumAsync::AsyncSystem& asyncSystem, 
					Cesium3DTilesSelection::VectorTileLoadResult&& tileLoadResult,
					const glm::dmat4& transform, const std::any& rendererOptions) override;

	virtual void* prepareVectorInMainThread(Cesium3DTilesSelection::VectorOverlayTile& vectorTile,
		void* pLoadThreadResult) override;

	virtual void* prepareVectorInLoadThread(CesiumGltf::VectorModel& model, const std::any& rendererOptions) override;

	virtual void attachVectorInMainThread(const Cesium3DTilesSelection::Tile& tile, int32_t overlayTextureCoordinateID,
		const Cesium3DTilesSelection::VectorOverlayTile& VectorTile, void* pMainThreadRendererResources,
		const glm::dvec2& translation, const glm::dvec2& scale) override;

	virtual void detachVectorInMainThread(const Cesium3DTilesSelection::Tile& tile, int32_t overlayTextureCoordinateID,
		const Cesium3DTilesSelection::VectorOverlayTile& VectorTile,
		void* pMainThreadRendererResources) noexcept override;

	virtual void free(Cesium3DTilesSelection::Tile& tile, void* pLoadThreadResult,
		void* pMainThreadResult) noexcept override;

	virtual void freeVector(const Cesium3DTilesSelection::VectorOverlayTile& vectorTile, void* pLoadThreadResult,
		void* pMainThreadResult) noexcept override;

private:
	ACesium3DTileset* _pActor;
};
