#include "Cesium3DTilesSelection/VectorOverlay.h"

#include "Cesium3DTilesSelection/IPrepareRendererResources.h"
#include "Cesium3DTilesSelection/VectorOverlay.h"
#include "Cesium3DTilesSelection/VectorOverlayTileProvider.h"
#include "Cesium3DTilesSelection/TilesetExternals.h"

#include <CesiumAsync/IAssetResponse.h>
#include <CesiumAsync/ITaskProcessor.h>
#include <CesiumUtility/joinToString.h>

using namespace CesiumAsync;

namespace Cesium3DTilesSelection {

VectorOverlayTile::VectorOverlayTile(
    VectorOverlayTileProvider& tileProvider) noexcept 
    {
        _pTileProvider = &tileProvider;
        _targetScreenPixels = glm::dvec2(0, 0);
        _rectangle = CesiumGeometry::Rectangle(0.0, 0.0, 0.0, 0.0);
        _tileCredits = {};
        _state = LoadState::Placeholder;
        _vectorModel = nullptr;
        _pRendererResources = nullptr;
        
    }

VectorOverlayTile::VectorOverlayTile(
    VectorOverlayTileProvider& tileProvider,
    const glm::dvec2& targetScreenPixels,
    const CesiumGeometry::Rectangle& rectangle) noexcept 
    {
        _pTileProvider = &tileProvider;
        _targetScreenPixels = targetScreenPixels;
        _rectangle = rectangle;
        _tileCredits = {};
        _state = LoadState::Unloaded;
        _vectorModel = nullptr;
        _pRendererResources = nullptr;
       
    }

VectorOverlayTile::~VectorOverlayTile()
{
  VectorOverlayTileProvider& tileProvider = *this->_pTileProvider;

  tileProvider.removeTile(this);

  const std::shared_ptr<IPrepareRendererResources>& pPrepareRendererResources =
      tileProvider.getPrepareRendererResources();

  if (pPrepareRendererResources) {
    void* pLoadThreadResult =
        this->getState() == VectorOverlayTile::LoadState::Done
            ? nullptr
            : this->_pRendererResources;
    void* pMainThreadResult =
        this->getState() == VectorOverlayTile::LoadState::Done
            ? this->_pRendererResources
            : nullptr;

    pPrepareRendererResources->freeVector(
        *this,
        pLoadThreadResult,
        pMainThreadResult);
  }
}

VectorOverlay& VectorOverlayTile::getOverlay() noexcept
{
  return this->_pTileProvider->getOwner();
}

/**
 * @brief Returns the {@link VectorOverlay} that created this instance.
 */
const VectorOverlay& VectorOverlayTile::getOverlay() const noexcept
{
  return this->_pTileProvider->getOwner();
}

void VectorOverlayTile::loadInMainThread()
{
  if (this->getState() != VectorOverlayTile::LoadState::Loaded) {
    return;
  }

  // Do the final main thread Vector loading
  VectorOverlayTileProvider& tileProvider = *this->_pTileProvider;
  this->_pRendererResources =
      tileProvider.getPrepareRendererResources()->prepareVectorInMainThread(
          *this,
          this->_pRendererResources);
  this->setState(LoadState::Done);
}

void VectorOverlayTile::setState(LoadState newState) noexcept
{
  this->_state = newState;
}

} // namespace Cesium3DTilesSelection
