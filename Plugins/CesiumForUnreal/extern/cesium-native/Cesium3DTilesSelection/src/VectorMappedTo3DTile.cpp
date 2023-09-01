#include "TileUtilities.h"

#include <Cesium3DTilesSelection/IPrepareRendererResources.h>
#include <Cesium3DTilesSelection/VectorMappedTo3DTile.h>
#include <Cesium3DTilesSelection/VectorOverlayCollection.h>
#include <Cesium3DTilesSelection/VectorOverlayTileProvider.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/Tileset.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/MVTUtilities.h>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace Cesium3DTilesSelection;
using namespace CesiumUtility;

namespace {

// Find the given overlay in the given tile.
VectorOverlayTile* findTileOverlay(Tile& tile, const VectorOverlay& overlay) {
  std::vector<VectorMappedTo3DTile>& tiles = tile.getMappedVectorTiles();
  auto parentTileIt = std::find_if(
      tiles.begin(),
      tiles.end(),
      [&overlay](VectorMappedTo3DTile& Vector) noexcept {
        VectorOverlayTile* pReady = Vector.getReadyTile();
        if (pReady == nullptr)
          return false;

        return &pReady->getTileProvider().getOwner() == &overlay;
      });
  if (parentTileIt != tiles.end()) {
    VectorMappedTo3DTile& mapped = *parentTileIt;

    // Prefer the loading tile if there is one.
    if (mapped.getLoadingTile()) {
      return mapped.getLoadingTile();
    } else {
      return mapped.getReadyTile();
    }
  }

  return nullptr;
}

} // namespace

namespace Cesium3DTilesSelection {

VectorMappedTo3DTile::VectorMappedTo3DTile(
    const CesiumUtility::IntrusivePointer<VectorOverlayTile>& pVectorTile,
    int32_t textureCoordinateIndex)
    : _pLoadingTile(pVectorTile),
      _pReadyTile(nullptr),
      _textureCoordinateID(textureCoordinateIndex),
      _translation(0.0, 0.0),
      _scale(1.0, 1.0),
      _state(AttachmentState::Unattached),
      _originalFailed(false) {
  assert(this->_pLoadingTile != nullptr);
}

/*
* 每一帧都会执行update函数
* VectorOverlayTile在第一次创建时_pLoadingTile是一个占位符，还没有调用请求
* 当在Provider中调用完成请求之后，再次执行update函数时，_pLoadingTile状态发生改变，
* 将_pLoadingTile赋值给_pReadyTile，_pLoadingTile置空
*/
bool VectorMappedTo3DTile::update(IPrepareRendererResources& pPrepareRendererResources, Tile& tile) 
{
    assert(this->_pLoadingTile != nullptr || this->_pReadyTile != nullptr);

    if (this->getState() == AttachmentState::Attached) 
    {
        return !this->_originalFailed && this->_pReadyTile;
    }

    // If the loading tile has failed, try its parent's loading tile.
    
    Tile* pTile = &tile;
/*
    while (this->_pLoadingTile &&
            this->_pLoadingTile->getState() ==
                VectorOverlayTile::LoadState::Failed && pTile) 
    {
        // Note when our original tile fails to load so that we don't report more
        // data available. This means - by design - we won't refine past a failed
        // tile.
        this->_originalFailed = true;

        pTile = pTile->getParent();
        if (pTile) 
        {
            VectorOverlayTile* pOverlayTile = findTileOverlay(*pTile, this->_pLoadingTile->getTileProvider().getOwner());
            if (pOverlayTile) 
            {
                this->_pLoadingTile = pOverlayTile;
            }
        }
    }*/

    // If the loading tile is now ready, make it the ready tile.
    if (this->_pLoadingTile &&
        this->_pLoadingTile->getState() >= VectorOverlayTile::LoadState::Loaded) 
    {
        // Unattach the old tile
        if (this->_pReadyTile && this->getState() != AttachmentState::Unattached) 
        {
            pPrepareRendererResources.detachVectorInMainThread(
                tile,
                this->getTextureCoordinateID(),
                *this->_pReadyTile,
                this->_pReadyTile->getRendererResources());
            this->_state = AttachmentState::Unattached;
        }


        // Mark the loading tile ready.
        this->_pReadyTile = this->_pLoadingTile;
        this->_pLoadingTile = nullptr;

        // Compute the translation and scale for the new tile.
        this->computeTranslationAndScale(tile);
    }

    // Find the closest ready ancestor tile.
    
    if (this->_pLoadingTile) 
    {
        CesiumUtility::IntrusivePointer<VectorOverlayTile> pCandidate;

        pTile = tile.getParent();
        while (pTile) 
        {
            pCandidate = findTileOverlay(
                *pTile,
                this->_pLoadingTile->getTileProvider().getOwner());
            if (pCandidate &&
                pCandidate->getState() >= VectorOverlayTile::LoadState::Loaded) 
            {
                break;
            }
            pTile = pTile->getParent();
        }

        if (pCandidate &&
            pCandidate->getState() >= VectorOverlayTile::LoadState::Loaded &&
            this->_pReadyTile != pCandidate) 
        {
            if (this->getState() != AttachmentState::Unattached) 
            {
                pPrepareRendererResources.detachVectorInMainThread(
                    tile,
                    this->getTextureCoordinateID(),
                    *this->_pReadyTile,
                    this->_pReadyTile->getRendererResources());
                this->_state = AttachmentState::Unattached;
            }

            this->_pReadyTile = pCandidate;

            // Compute the translation and scale for the new tile.
            this->computeTranslationAndScale(tile);
        }
    }

    // Attach the ready tile if it's not already attached.
    if (this->_pReadyTile &&
        this->getState() == VectorMappedTo3DTile::AttachmentState::Unattached) 
    {
        this->_pReadyTile->loadInMainThread();
        pPrepareRendererResources.attachVectorInMainThread(
            tile,
            this->getTextureCoordinateID(),
            *this->_pReadyTile,
            this->_pReadyTile->getRendererResources(),
            this->getTranslation(),
            this->getScale());

        this->_state = this->_pLoadingTile ? AttachmentState::TemporarilyAttached
                                            : AttachmentState::Attached;
    }

    assert(this->_pLoadingTile != nullptr || this->_pReadyTile != nullptr);

    // TODO: check more precise Vector overlay tile availability, rather than just
    // max level?
    return false;
}

void VectorMappedTo3DTile::detachFromTile(
    IPrepareRendererResources& prepareRendererResources,
    Tile& tile) noexcept {
  if (this->getState() == AttachmentState::Unattached) {
    return;
  }

  if (!this->_pReadyTile) {
    return;
  }

  prepareRendererResources.detachVectorInMainThread(
      tile,
      this->getTextureCoordinateID(),
      *this->_pReadyTile,
      this->_pReadyTile->getRendererResources());

  this->_state = AttachmentState::Unattached;
}

bool VectorMappedTo3DTile::loadThrottled() noexcept {
  CESIUM_TRACE("VectorMappedTo3DTile::loadThrottled");
  VectorOverlayTile* pLoading = this->getLoadingTile();
  if (!pLoading) {
    return true;
  }

  VectorOverlayTileProvider& provider = pLoading->getTileProvider();
  return provider.loadTileThrottled(*pLoading);
}

namespace {

IntrusivePointer<VectorOverlayTile>
getPlaceholderTile(VectorOverlayTileProvider& tileProvider) {
  // Rectangle and geometric error don't matter for a placeholder.
  return tileProvider.getTile(Rectangle(), glm::dvec2(0.0));
}

std::optional<Rectangle> getPreciseRectangleFromBoundingVolume(
    const Projection& projection,
    const BoundingVolume& boundingVolume) {
  const BoundingRegion* pRegion =
      getBoundingRegionFromBoundingVolume(boundingVolume);
  if (!pRegion) {
    return std::nullopt;
  }

  // Currently _all_ supported projections can have a rectangle precisely
  // determined from a bounding region. This may not be true, however, for
  // projections we add in the future where X is not purely a function of
  // longitude or Y is not purely a function of latitude.
  return projectRectangleSimple(projection, pRegion->getRectangle());
}

int32_t addProjectionToList(
    std::vector<Projection>& projections,
    const Projection& projection) {
  auto it = std::find(projections.begin(), projections.end(), projection);
  if (it == projections.end()) {
    projections.emplace_back(projection);
    return int32_t(projections.size()) - 1;
  } else {
    return int32_t(it - projections.begin());
  }
}

glm::dvec2 computeDesiredScreenPixels(
    const Tile& tile,
    const Projection& projection,
    const Rectangle& rectangle,
    double maxHeight,
    double maximumScreenSpaceError,
    const Ellipsoid& ellipsoid = Ellipsoid::WGS84) {
  // We're aiming to estimate the maximum number of pixels (in each projected
  // direction) the tile will occupy on the screen. The will be determined by
  // the tile's geometric error, because when less error is needed (i.e. the
  // viewer moved closer), the LOD will switch to show the tile's children
  // instead of this tile.
  //
  // It works like this:
  // * Estimate the size of the projected rectangle in world coordinates.
  // * Compute the distance at which tile will switch to its children, based on
  // its geometric error and the tileset SSE.
  // * Compute the on-screen size of the projected rectangle at that distance.
  //
  // For the two compute steps, we use the usual perspective projection SSE
  // equation:
  // screenSize = (realSize * viewportHeight) / (distance * 2 * tan(0.5 * fovY))
  //
  // Conveniently a bunch of terms cancel out, so the screen pixel size at the
  // switch distance is not actually dependent on the screen dimensions or
  // field-of-view angle.
  double geometryError = tile.getNonZeroGeometricError();
  glm::dvec2 diameters = computeProjectedRectangleSize(
      projection,
      rectangle,
      maxHeight,
      ellipsoid);
  return diameters * maximumScreenSpaceError / geometryError;
}

VectorMappedTo3DTile* addRealTile(
    Tile& tile,
    VectorOverlayTileProvider& provider,
    const Rectangle& rectangle,
    const glm::dvec2& screenPixels,
    int32_t textureCoordinateIndex) 
    {
        IntrusivePointer<VectorOverlayTile> pTile = provider.getTile(rectangle, screenPixels);
        if (!pTile) 
        {
            return nullptr;
        } 
        else 
        {
            auto tileID = MVTUtilities::GetTileID(tile.getTileID());
            int wmtsY = static_cast<int>(glm::pow(2, tileID.level)) - 1 - tileID.y;
            pTile->setTileID(tileID.level, wmtsY, tileID.x);
            return &tile.getMappedVectorTiles().emplace_back(VectorMappedTo3DTile(pTile, textureCoordinateIndex));
        }
    }

} // namespace

/*static*/ VectorMappedTo3DTile* VectorMappedTo3DTile::mapOverlayToTile(
    double maximumScreenSpaceError,
    VectorOverlayTileProvider& tileProvider,
    VectorOverlayTileProvider& placeholder,
    Tile& tile,
    std::vector<Projection>& missingProjections)
{
    IntrusivePointer<VectorOverlayTile> pTile = getPlaceholderTile(placeholder);
    auto tileID = MVTUtilities::GetTileID(tile.getTileID());
            int wmtsY = static_cast<int>(glm::pow(2, tileID.level)) - 1 - tileID.y;
            pTile->setTileID(tileID.level, wmtsY, tileID.x);

  if (tileProvider.isPlaceholder())
  {
    // Provider not created yet, so add a placeholder tile.
    return &tile.getMappedVectorTiles().emplace_back(VectorMappedTo3DTile(pTile, -1));
  }

  // We can get a more accurate estimate of the real-world size of the projected
  // rectangle if we consider the rectangle at the true height of the geometry
  // rather than assuming it's on the ellipsoid. This will make basically no
  // difference for small tiles (because surface normals on opposite ends of
  // tiles are effectively identical), and only a small difference for large
  // ones (because heights will be small compared to the total size of a large
  // tile). So we're skipping this complexity for now and estimating geometry
  // width/height as if it's on the ellipsoid surface.
  const double heightForSizeEstimation = 0.0;

  const Projection& projection = tileProvider.getProjection();

  // If the tile is loaded, use the precise rectangle computed from the content.
  const TileContent& content = tile.getContent();
  const TileRenderContent* pRenderContent = content.getRenderContent();
  if (pRenderContent)
  {
    const RasterOverlayDetails& overlayDetails =
        pRenderContent->getRasterOverlayDetails();
    const Rectangle* pRectangle =
        overlayDetails.findRectangleForOverlayProjection(projection);
    if (pRectangle)
    {
      // We have a rectangle and texture coordinates for this projection.
      int32_t index =int32_t(pRectangle - &overlayDetails.rasterOverlayRectangles[0]);
      const glm::dvec2 screenPixels = computeDesiredScreenPixels(
          tile,
          projection,
          *pRectangle,
          heightForSizeEstimation,
          maximumScreenSpaceError,
          Ellipsoid::WGS84);
      return addRealTile(tile, tileProvider, *pRectangle, screenPixels, index);
    }
    else
    {
      // We don't have a precise rectangle for this projection, which means the
      // tile was loaded before we knew we needed this projection. We'll need to
      // reload the tile (later).
      int32_t existingIndex =
          int32_t(overlayDetails.rasterOverlayProjections.size());
      int32_t textureCoordinateIndex =
          existingIndex + addProjectionToList(missingProjections, projection);
      return &tile.getMappedVectorTiles().emplace_back(VectorMappedTo3DTile(pTile, textureCoordinateIndex));
    }
  }

  // Maybe we can derive a precise rectangle from the bounding volume.
  int32_t textureCoordinateIndex =
      addProjectionToList(missingProjections, projection);
  std::optional<Rectangle> maybeRectangle =
      getPreciseRectangleFromBoundingVolume(
          tileProvider.getProjection(),
          tile.getBoundingVolume());
  if (maybeRectangle)
  {
    const glm::dvec2 screenPixels = computeDesiredScreenPixels(
        tile,
        projection,
        *maybeRectangle,
        heightForSizeEstimation,
        maximumScreenSpaceError,
        Ellipsoid::WGS84);
    return addRealTile(
        tile,
        tileProvider,
        *maybeRectangle,
        screenPixels,
        textureCoordinateIndex);
  } else {
    // No precise rectangle yet, so return a placeholder for now.
    return &tile.getMappedVectorTiles().emplace_back(VectorMappedTo3DTile(pTile, textureCoordinateIndex));
  }
}

void VectorMappedTo3DTile::computeTranslationAndScale(const Tile& tile) {
  if (!this->_pReadyTile) {
    // This shouldn't happen
    assert(false);
    return;
  }

  const TileRenderContent* pRenderContent =
      tile.getContent().getRenderContent();
  if (!pRenderContent) {
    return;
  }

  const RasterOverlayDetails& overlayDetails =
      pRenderContent->getRasterOverlayDetails();
  const VectorOverlayTileProvider& tileProvider =
      this->_pReadyTile->getTileProvider();

  const Projection& projection = tileProvider.getProjection();
  const std::vector<Projection>& projections =
      overlayDetails.rasterOverlayProjections;
  const std::vector<Rectangle>& rectangles =
      overlayDetails.rasterOverlayRectangles;

  auto projectionIt =
      std::find(projections.begin(), projections.end(), projection);
  if (projectionIt == projections.end()) {
    return;
  }

  int32_t projectionIndex = int32_t(projectionIt - projections.begin());
  if (projectionIndex < 0 || size_t(projectionIndex) >= rectangles.size()) {
    return;
  }

  const Rectangle& geometryRectangle = rectangles[size_t(projectionIndex)];

  const CesiumGeometry::Rectangle imageryRectangle =
      this->_pReadyTile->getRectangle();

  const double terrainWidth = geometryRectangle.computeWidth();
  const double terrainHeight = geometryRectangle.computeHeight();

  const double scaleX = terrainWidth / imageryRectangle.computeWidth();
  const double scaleY = terrainHeight / imageryRectangle.computeHeight();
  this->_translation = glm::dvec2(
      (scaleX * (geometryRectangle.minimumX - imageryRectangle.minimumX)) /
          terrainWidth,
      (scaleY * (geometryRectangle.minimumY - imageryRectangle.minimumY)) /
          terrainHeight);
  this->_scale = glm::dvec2(scaleX, scaleY);
}

} // namespace Cesium3DTilesSelection
