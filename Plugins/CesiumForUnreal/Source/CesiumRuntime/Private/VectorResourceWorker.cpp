#include "VectorResourceWorker.h"
#include "CesiumGltfComponent.h"
#include "CreateGltfOptions.h"
#include "CesiumVectorOverlay.h"

void* VectorResourceWorker::prepareInMainThread(Cesium3DTilesSelection::Tile& tile, void* pLoadThreadResult)
{
    const Cesium3DTilesSelection::TileContent& content = tile.getContent();
    if (content.isRenderContent()) {
        TUniquePtr<UCesiumGltfComponent::HalfConstructed> pHalf(
            reinterpret_cast<UCesiumGltfComponent::HalfConstructed*>(
                pLoadThreadResult));
        const Cesium3DTilesSelection::TileRenderContent& renderContent =
            *content.getRenderContent();
        return UCesiumGltfComponent::CreateOnGameThread(
            renderContent.getModel(),
            this->_pActor,
            std::move(pHalf),
            _pActor->GetCesiumTilesetToUnrealRelativeWorldTransform(),
            this->_pActor->GetMaterial(),
            this->_pActor->GetTranslucentMaterial(),
            this->_pActor->GetWaterMaterial(),
            this->_pActor->GetCustomDepthParameters(),
            tile.getContentBoundingVolume().value_or(tile.getBoundingVolume()),
            this->_pActor->GetCreateNavCollision());
    }
    // UE_LOG(LogCesium, VeryVerbose, TEXT("No content for tile"));
    return nullptr;
}

CesiumAsync::Future<Cesium3DTilesSelection::VectorTileLoadResultAndRenderResources> VectorResourceWorker::prepareInLoadThread(
	const CesiumAsync::AsyncSystem& asyncSystem, Cesium3DTilesSelection::VectorTileLoadResult&& tileLoadResult,
	const glm::dmat4& transform, const std::any& rendererOptions)
{
	CreateGltfOptions::CreateModelOptions options;
	TUniquePtr<UCesiumGltfComponent::HalfConstructed> pHalf =
		UCesiumGltfComponent::CreateOffGameThread(transform, options);
	return asyncSystem.createResolvedFuture(Cesium3DTilesSelection::VectorTileLoadResultAndRenderResources{
		std::move(tileLoadResult),
		pHalf.Release()});
}

void* VectorResourceWorker::prepareVectorInMainThread(Cesium3DTilesSelection::VectorOverlayTile& vectorTile,
    void* pLoadThreadResult)
{
    TUniquePtr<CesiumTextureUtility::LoadedTextureResult> pLoadedTexture{
         static_cast<CesiumTextureUtility::LoadedTextureResult*>(
             pLoadThreadResult) };

    if (!pLoadedTexture) {
        return nullptr;
    }
    return pLoadThreadResult;
}

void* VectorResourceWorker::prepareVectorInLoadThread(CesiumGltf::VectorModel& model, const std::any& rendererOptions)
{
    auto ppOptions =
        std::any_cast<FVectorOverlayRendererOptions*>(&rendererOptions);
    check(ppOptions != nullptr && *ppOptions != nullptr);
    if (ppOptions == nullptr || *ppOptions == nullptr) {
        return nullptr;
    }

    auto pOptions = *ppOptions;

    return nullptr;
}

void VectorResourceWorker::attachVectorInMainThread(const Cesium3DTilesSelection::Tile& tile,
	int32_t overlayTextureCoordinateID, const Cesium3DTilesSelection::VectorOverlayTile& VectorTile,
	void* pMainThreadRendererResources, const glm::dvec2& translation, const glm::dvec2& scale)
{
}

void VectorResourceWorker::detachVectorInMainThread(const Cesium3DTilesSelection::Tile& tile,
	int32_t overlayTextureCoordinateID, const Cesium3DTilesSelection::VectorOverlayTile& VectorTile,
	void* pMainThreadRendererResources) noexcept
{
}

void VectorResourceWorker::free(Cesium3DTilesSelection::Tile& tile, void* pLoadThreadResult,
	void* pMainThreadResult) noexcept
{
}

void VectorResourceWorker::freeVector(const Cesium3DTilesSelection::VectorOverlayTile& vectorTile,
	void* pLoadThreadResult, void* pMainThreadResult) noexcept
{
}
