#include "Cesium3DTilesSelection/VectorOverlay.h"

#include "Cesium3DTilesSelection/VectorOverlayCollection.h"
#include "Cesium3DTilesSelection/RasterOverlayLoadFailureDetails.h"
#include "Cesium3DTilesSelection/VectorOverlayTileProvider.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"

using namespace CesiumAsync;
using namespace Cesium3DTilesSelection;
using namespace CesiumUtility;

namespace {
class PlaceholderTileProvider : public VectorOverlayTileProvider {
public:
  PlaceholderTileProvider(
      const IntrusivePointer<const VectorOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor) noexcept
      : VectorOverlayTileProvider(pOwner, asyncSystem, pAssetAccessor) {}

  virtual CesiumAsync::Future<LoadedVectorOverlayData>
  loadTileData(VectorOverlayTile& /* overlayTile */) override {
    return this->getAsyncSystem()
        .createResolvedFuture<LoadedVectorOverlayData>({});
  }
};
} // namespace

VectorOverlay::VectorOverlay(
    const std::string& name,
    const VectorOverlayOptions& options)
    : _name(name), _options(options), _destructionCompleteDetails{} {}

VectorOverlay::~VectorOverlay() noexcept {
  if (this->_destructionCompleteDetails.has_value()) {
    this->_destructionCompleteDetails->promise.resolve();
  }
}

CesiumAsync::SharedFuture<void>&
VectorOverlay::getAsyncDestructionCompleteEvent(
    const CesiumAsync::AsyncSystem& asyncSystem) {
  if (!this->_destructionCompleteDetails.has_value()) {
    auto promise = asyncSystem.createPromise<void>();
    auto sharedFuture = promise.getFuture().share();
    this->_destructionCompleteDetails.emplace(DestructionCompleteDetails{
        asyncSystem,
        std::move(promise),
        std::move(sharedFuture)});
  } else {
    // All invocations of getAsyncDestructionCompleteEvent on a particular
    // VectorOverlay must pass equivalent AsyncSystems.
    assert(this->_destructionCompleteDetails->asyncSystem == asyncSystem);
  }

  return this->_destructionCompleteDetails->future;
}

CesiumUtility::IntrusivePointer<VectorOverlayTileProvider>
VectorOverlay::createPlaceholder(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor) const {
  return new PlaceholderTileProvider(this, asyncSystem, pAssetAccessor);
}
