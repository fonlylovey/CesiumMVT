#pragma once

#include "CoreMinimal.h"
#include "Cesium3DTilesSelection/IRendererResourcesWorker.h"
#include "Cesium3DTileset.h"

class GeoTransforms;

namespace Cesium3DTilesSelection {
class VectorOverlayTile;
} // namespace Cesium3DTilesSelection

/**
 * @brief ��native���ж�ȡ������ͨ��void*�ķ�ʽ���ݵ�UE��֮��������൱��ת��ΪUE������
 */

class CESIUMRUNTIME_API VectorResourceWorker : public Cesium3DTilesSelection::IRendererResourcesWorker
{
public:
	VectorResourceWorker(ACesium3DTileset* pActor) : _pActor(pActor) {}
	~VectorResourceWorker() = default;

	virtual void* prepareVectorInLoadThread(CesiumGltf::VectorModel* pModel, const std::any& rendererOptions) override;

	virtual void* prepareVectorInMainThread(Cesium3DTilesSelection::VectorOverlayTile& vectorTile,
		void* pLoadThreadResult) override;
	
	virtual void attachVectorInMainThread(const Cesium3DTilesSelection::Tile& tile, int32_t overlayTextureCoordinateID,
		const Cesium3DTilesSelection::VectorOverlayTile& VectorTile, void* pMainThreadRendererResources,
		const glm::dvec2& translation, const glm::dvec2& scale) override;

	virtual void detachVectorInMainThread(const Cesium3DTilesSelection::Tile& tile, int32_t overlayTextureCoordinateID,
		const Cesium3DTilesSelection::VectorOverlayTile& VectorTile,
		void* pMainThreadRendererResources) noexcept override;

	virtual void free(Cesium3DTilesSelection::Tile& tile,
      void* pLoadThreadResult,
      void* pMainThreadResult) noexcept override;

	virtual void freeVector(const Cesium3DTilesSelection::VectorOverlayTile& vectorTile, void* pLoadThreadResult,
		void* pMainThreadResult) noexcept override;

private:
	ACesium3DTileset* _pActor;
	GeoTransforms _geoTransforms;
};
