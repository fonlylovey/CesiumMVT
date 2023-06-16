#include "VectorResourceWorker.h"
#include "CesiumGltfComponent.h"
#include "CesiumVectorComponent.h"
#include "CreateGltfOptions.h"
#include "CesiumVectorOverlay.h"
#include "Cesium3DTilesSelection/VectorOverlayTile.h"


void* VectorResourceWorker::prepareVectorInLoadThread(CesiumGltf::VectorModel& model, const std::any& rendererOptions)
{
    auto ppOptions =
        std::any_cast<FVectorOverlayRendererOptions*>(&rendererOptions);
    check(ppOptions != nullptr && *ppOptions != nullptr);
    if (ppOptions == nullptr || *ppOptions == nullptr) {
        return nullptr;
    }
    auto pOptions = *ppOptions;
	CesiumGltf::VectorModel* theModel = new CesiumGltf::VectorModel;
	if(model.layers.size() > 0)
	{
		theModel->layers = model.layers;
	}
	
    return theModel;
}

void* VectorResourceWorker::prepareVectorInMainThread(Cesium3DTilesSelection::VectorOverlayTile& vectorTile,
    void* pLoadThreadResult)
{
	CesiumGltf::VectorModel* pModelData = static_cast<CesiumGltf::VectorModel*>(pLoadThreadResult);
	std::cout << "MainThread" << pLoadThreadResult << std::endl;
	if(pModelData->layers.size() > 0)
	{
		glm::dmat4x4 transform;
		ULineBatchComponent* lineBatch = UCesiumVectorComponent::CreateTest(*pModelData, _pActor, transform);
		return pLoadThreadResult;
	}
	//TUniquePtr<Cesium3DTilesSelection::LoadedVectorOverlayData> pLoadedOverlayData{
	//		static_cast<Cesium3DTilesSelection::LoadedVectorOverlayData*>(
	//			pLoadThreadResult)};
	return nullptr;
}

void VectorResourceWorker::attachVectorInMainThread(const Cesium3DTilesSelection::Tile& tile,
	int32_t overlayTextureCoordinateID, const Cesium3DTilesSelection::VectorOverlayTile& VectorTile,
	void* pMainThreadRendererResources, const glm::dvec2& translation, const glm::dvec2& scale)
{
	const Cesium3DTilesSelection::TileContent& content = tile.getContent();
    const Cesium3DTilesSelection::TileRenderContent* pRenderContent = content.getRenderContent();
	if(pRenderContent != nullptr)
	{
		 UCesiumGltfComponent* pGltfContent = reinterpret_cast<UCesiumGltfComponent*>(
              pRenderContent->getRenderResources());
		CesiumGltf::VectorModel vectorModel = VectorTile.getVectorModel();
		CesiumGltf::VectorModel* ptrModel = static_cast<CesiumGltf::VectorModel*>(pMainThreadRendererResources);
		
		if(pMainThreadRendererResources != nullptr && ptrModel->layers.size() > 0)
		{
			glm::dmat4x4 transform;
			transform;
			//ULineBatchComponent* lineBatch = UCesiumVectorComponent::CreateTest(vectorModel, _pActor, transform);
		}
		/*
		 CesiumGltf::VectorModel* vectorModel = static_cast<CesiumGltf::VectorModel*>(pMainThreadRendererResources);
		 auto ptrLoaded = static_cast<Cesium3DTilesSelection::LoadedVectorOverlayData*>(pMainThreadRendererResources);
		 TUniquePtr<Cesium3DTilesSelection::LoadedVectorOverlayData> pLoadedVector;
		 pLoadedVector.Reset(ptrLoaded);
		 if(vectorModel->layers.size() > 0)
		 {
			 glm::dmat4x4 transform;
			 //ULineBatchComponent* lineBatch = UCesiumVectorComponent::CreateTest(*vectorModel, _pActor, transform);
		 }
		 */
	}

}

void VectorResourceWorker::detachVectorInMainThread(const Cesium3DTilesSelection::Tile& tile,
	int32_t overlayTextureCoordinateID, const Cesium3DTilesSelection::VectorOverlayTile& VectorTile,
	void* pMainThreadRendererResources) noexcept
{
}

void VectorResourceWorker::freeVector(const Cesium3DTilesSelection::VectorOverlayTile& vectorTile,
	void* pLoadThreadResult, void* pMainThreadResult) noexcept
{
}
