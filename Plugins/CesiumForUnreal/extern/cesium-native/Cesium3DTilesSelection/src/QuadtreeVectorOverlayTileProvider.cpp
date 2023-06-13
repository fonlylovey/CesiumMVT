#include "Cesium3DTilesSelection/QuadtreeVectorOverlayTileProvider.h"

#include "Cesium3DTilesSelection/VectorOverlay.h"

#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumUtility/Math.h>
#include <CesiumGltfReader/ImageManipulation.h>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumGltf;
using namespace CesiumGltfReader;
using namespace CesiumUtility;

namespace {

// One hundredth of a pixel. If we compute a rectangle that goes less than "this
// much" into the next pixel, we'll ignore the extra.
constexpr double pixelTolerance = 0.01;

} // namespace

namespace Cesium3DTilesSelection {

QuadtreeVectorOverlayTileProvider::QuadtreeVectorOverlayTileProvider(
    const IntrusivePointer<const VectorOverlay>& pOwner,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    std::optional<Credit> credit,
    const std::shared_ptr<IRendererResourcesWorker>& pRendererResourcesWorker,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const CesiumGeospatial::Projection& projection,
    const CesiumGeometry::QuadtreeTilingScheme& tilingScheme,
    const CesiumGeometry::Rectangle& coverageRectangle,
    uint32_t minimumLevel,
    uint32_t maximumLevel,
    uint32_t imageWidth,
    uint32_t imageHeight) noexcept
    : VectorOverlayTileProvider(
          pOwner,
          asyncSystem,
          pAssetAccessor,
          credit,
          pRendererResourcesWorker,
          pLogger,
          projection,
          coverageRectangle),
      _minimumLevel(minimumLevel),
      _maximumLevel(maximumLevel),
      _tileWidth(imageWidth),
      _tileHeight(imageHeight),
      _tilingScheme(tilingScheme),
      _tilesOldToRecent(),
      _tileLookup(),
      _cachedBytes(0) {}

uint32_t QuadtreeVectorOverlayTileProvider::computeLevelFromTargetScreenPixels(
    const CesiumGeometry::Rectangle& rectangle,
    const glm::dvec2& screenPixels) {
  const double rasterScreenSpaceError =
      this->getOwner().getOptions().maximumScreenSpaceError;

  const glm::dvec2 rasterPixels = screenPixels / rasterScreenSpaceError;
  const glm::dvec2 rasterTiles =
      rasterPixels / glm::dvec2(this->getWidth(), this->getHeight());
  const glm::dvec2 targetTileDimensions =
      glm::dvec2(rectangle.computeWidth(), rectangle.computeHeight()) /
      rasterTiles;
  const glm::dvec2 totalDimensions = glm::dvec2(
      this->getTilingScheme().getRectangle().computeWidth(),
      this->getTilingScheme().getRectangle().computeHeight());
  const glm::dvec2 totalTileDimensions =
      totalDimensions / glm::dvec2(
                            this->getTilingScheme().getRootTilesX(),
                            this->getTilingScheme().getRootTilesY());
  const glm::dvec2 twoToTheLevelPower =
      totalTileDimensions / targetTileDimensions;
  const glm::dvec2 level = glm::log2(twoToTheLevelPower);
  const glm::dvec2 rounded = glm::max(glm::round(level), 0.0);

  uint32_t imageryLevel = uint32_t(glm::max(rounded.x, rounded.y));

  const uint32_t maximumLevel = this->getMaximumLevel();
  if (imageryLevel > maximumLevel) {
    imageryLevel = maximumLevel;
  }

  const uint32_t minimumLevel = this->getMinimumLevel();
  if (imageryLevel < minimumLevel) {
    imageryLevel = minimumLevel;
  }

  return imageryLevel;
}

std::vector<CesiumAsync::SharedFuture<
    QuadtreeVectorOverlayTileProvider::LoadedQuadtreeData>>
QuadtreeVectorOverlayTileProvider::mapVectorTilesToGeometryTile(
    const CesiumGeometry::Rectangle& geometryRectangle,
    const glm::dvec2 targetScreenPixels)
{
  std::vector<CesiumAsync::SharedFuture<LoadedQuadtreeData>> result;

  const QuadtreeTilingScheme& tilingScheme = this->getTilingScheme();

  //如果此影像图层使用该投影并且地形切片完全位于投影的有效范围内，
  //则使用 Web 墨卡托进行纹理坐标计算。
  //bool useWebMercatorT = pWebMercatorProjection 
  //&& tileRectangle.getNorth（）<= WebMercatorProjection：：MAXIMUM_LATITUDE 
  //&& tileRectangle.getSouth（） >= -WebMercatorProjection：：MAXIMUM_LATITUDE;

  const CesiumGeometry::Rectangle& providerRectangle =
      this->getCoverageRectangle();
  const CesiumGeometry::Rectangle& tilingSchemeRectangle =
      tilingScheme.getRectangle();

  // Compute the rectangle of the imagery from this raster tile provider that
  // overlaps the geometry tile.  The VectorOverlayTileProvider and its tiling
  // scheme both have the opportunity to constrain the rectangle.
  CesiumGeometry::Rectangle rectangle =
      tilingSchemeRectangle.computeIntersection(providerRectangle)
          .value_or(tilingSchemeRectangle);

  CesiumGeometry::Rectangle intersection(0.0, 0.0, 0.0, 0.0);
  std::optional<CesiumGeometry::Rectangle> maybeIntersection =
      geometryRectangle.computeIntersection(rectangle);
  if (maybeIntersection)
  {
    intersection = maybeIntersection.value();
  }
  else
  {
    // There is no overlap between this terrain tile and this imagery
    // provider.  Unless this is the base layer, no skeletons need to be
    // created. We stretch texels at the edge of the base layer over the entire
    // globe.

    // TODO: base layers
    // if (!this.isBaseLayer()) {
    //     return false;
    // }
    if (geometryRectangle.minimumY >= rectangle.maximumY)
    {
      intersection.minimumY = intersection.maximumY = rectangle.maximumY;
    }
    else if (geometryRectangle.maximumY <= rectangle.minimumY)
    {
      intersection.minimumY = intersection.maximumY = rectangle.minimumY;
    } else
    {
      intersection.minimumY =
          glm::max(geometryRectangle.minimumY, rectangle.minimumY);

      intersection.maximumY =
          glm::min(geometryRectangle.maximumY, rectangle.maximumY);
    }

    if (geometryRectangle.minimumX >= rectangle.maximumX)
    {
      intersection.minimumX = intersection.maximumX = rectangle.maximumX;
    }
    else if (geometryRectangle.maximumX <= rectangle.minimumX)
    {
      intersection.minimumX = intersection.maximumX = rectangle.minimumX;
    }
    else
    {
      intersection.minimumX =
          glm::max(geometryRectangle.minimumX, rectangle.minimumX);

      intersection.maximumX =
          glm::min(geometryRectangle.maximumX, rectangle.maximumX);
    }
  }

  // Compute the required level in the imagery tiling scheme.
  uint32_t level = this->computeLevelFromTargetScreenPixels(
      geometryRectangle,
      targetScreenPixels);

  std::optional<QuadtreeTileID> southwestTileCoordinatesOpt =
      tilingScheme.positionToTile(intersection.getLowerLeft(), level);
  std::optional<QuadtreeTileID> northeastTileCoordinatesOpt =
      tilingScheme.positionToTile(intersection.getUpperRight(), level);

  // Because of the intersection, we should always have valid tile coordinates.
  // But give up if we don't.
  if (!southwestTileCoordinatesOpt || !northeastTileCoordinatesOpt)
  {
    return result;
  }

  QuadtreeTileID southwestTileCoordinates = southwestTileCoordinatesOpt.value();
  QuadtreeTileID northeastTileCoordinates = northeastTileCoordinatesOpt.value();

  // If the northeast corner of the rectangle lies very close to the south or
  // west side of the northeast tile, we don't actually need the northernmost or
  // easternmost tiles. Similarly, if the southwest corner of the rectangle lies
  // very close to the north or east side of the southwest tile, we don't
  // actually need the southernmost or westernmost tiles.

  // We define "very close" as being within 1/512 of the width of the tile.
  const double veryCloseX = geometryRectangle.computeWidth() / 512.0;
  const double veryCloseY = geometryRectangle.computeHeight() / 512.0;

  const CesiumGeometry::Rectangle southwestTileRectangle =
      tilingScheme.tileToRectangle(southwestTileCoordinates);

  if (glm::abs(southwestTileRectangle.maximumY - geometryRectangle.minimumY) <
          veryCloseY &&
      southwestTileCoordinates.y < northeastTileCoordinates.y)
  {
    ++southwestTileCoordinates.y;
  }

  if (glm::abs(southwestTileRectangle.maximumX - geometryRectangle.minimumX) <
          veryCloseX &&
      southwestTileCoordinates.x < northeastTileCoordinates.x)
  {
    ++southwestTileCoordinates.x;
  }

  const CesiumGeometry::Rectangle northeastTileRectangle =
      tilingScheme.tileToRectangle(northeastTileCoordinates);

  if (glm::abs(northeastTileRectangle.maximumY - geometryRectangle.minimumY) <
          veryCloseY &&
      northeastTileCoordinates.y > southwestTileCoordinates.y)
  {
    --northeastTileCoordinates.y;
  }

  if (glm::abs(northeastTileRectangle.minimumX - geometryRectangle.maximumX) <
          veryCloseX &&
      northeastTileCoordinates.x > southwestTileCoordinates.x)
  {
    --northeastTileCoordinates.x;
  }

  // If we're mapping too many tiles, reduce the level until it's sane.
  uint32_t maxTextureSize =
      uint32_t(this->getOwner().getOptions().maximumTextureSize);

  uint32_t tilesX = northeastTileCoordinates.x - southwestTileCoordinates.x + 1;
  uint32_t tilesY = northeastTileCoordinates.y - southwestTileCoordinates.y + 1;

  while (level > 0U && (tilesX * this->getWidth() > maxTextureSize ||
                        tilesY * this->getHeight() > maxTextureSize))
  {
    --level;
    northeastTileCoordinates = northeastTileCoordinates.getParent();
    southwestTileCoordinates = southwestTileCoordinates.getParent();
    tilesX = northeastTileCoordinates.x - southwestTileCoordinates.x + 1;
    tilesY = northeastTileCoordinates.y - southwestTileCoordinates.y + 1;
  }

  // Create TileImagery instances for each imagery tile overlapping this terrain
  // tile. We need to do all texture coordinate computations in the imagery
  // provider's projection.
  const CesiumGeometry::Rectangle imageryBounds = intersection;
  std::optional<CesiumGeometry::Rectangle> clippedImageryRectangle =
      std::nullopt;

  for (uint32_t i = southwestTileCoordinates.x; i <= northeastTileCoordinates.x;
       ++i)
  {

    rectangle = tilingScheme.tileToRectangle(
        QuadtreeTileID(level, i, southwestTileCoordinates.y));
    clippedImageryRectangle =
        rectangle.computeIntersection(imageryBounds);

    if (!clippedImageryRectangle)
    {
      continue;
    }

    for (uint32_t j = southwestTileCoordinates.y;
         j <= northeastTileCoordinates.y;
         ++j)
    {

      rectangle =
          tilingScheme.tileToRectangle(QuadtreeTileID(level, i, j));
      clippedImageryRectangle =
          rectangle.computeIntersection(imageryBounds);

      if (!clippedImageryRectangle)
      {
        continue;
      }

      CesiumAsync::SharedFuture<LoadedQuadtreeData> pTile =
          this->getQuadtreeTile(QuadtreeTileID(level, i, j));
      result.emplace_back(std::move(pTile));
    }
  }

  return result;
}

CesiumAsync::SharedFuture<
    QuadtreeVectorOverlayTileProvider::LoadedQuadtreeData>
QuadtreeVectorOverlayTileProvider::getQuadtreeTile(
    const CesiumGeometry::QuadtreeTileID& tileID) {
  auto lookupIt = this->_tileLookup.find(tileID);
  if (lookupIt != this->_tileLookup.end()) {
    auto& cacheIt = lookupIt->second;

    // Move this entry to the end, indicating it's most recently used.
    this->_tilesOldToRecent.splice(
        this->_tilesOldToRecent.end(),
        this->_tilesOldToRecent,
        cacheIt);

    return cacheIt->future;
  }

  // We create this lambda here instead of where it's used below so that we
  // don't need to pass `this` through a thenImmediately lambda, which would
  // create the possibility of accidentally using this pointer to a
  // non-thread-safe object from another thread and creating a (potentially very
  // subtle) race condition.
  auto loadParentTile = [tileID, this]() {
    const Rectangle rectangle = this->getTilingScheme().tileToRectangle(tileID);
    const QuadtreeTileID parentID(
        tileID.level - 1,
        tileID.x >> 1,
        tileID.y >> 1);
    return this->getQuadtreeTile(parentID).thenImmediately(
        [rectangle](const LoadedQuadtreeData& loaded) {
          return LoadedQuadtreeData{loaded.pLoaded, rectangle};
        });
  };

  Future<LoadedQuadtreeData> future =
      this->loadQuadtreeTileData(tileID)
          .catchImmediately([](std::exception&& e) {
            // Turn an exception into an error.
            LoadedVectorOverlayData result;
            result.errors.emplace_back(e.what());
            return result;
          })
          .thenImmediately([&cachedBytes = this->_cachedBytes,
                            currentLevel = tileID.level,
                            minimumLevel = this->getMinimumLevel(),
                            asyncSystem = this->getAsyncSystem(),
                            loadParentTile = std::move(loadParentTile)](
                               LoadedVectorOverlayData&& loaded) {
            if (loaded.vectorModel && loaded.errors.empty() &&
                loaded.vectorModel.has_value())
            {
              // Successfully loaded, continue.
              cachedBytes += int64_t(loaded.vectorModel->layers.size());

#if SHOW_TILE_BOUNDARIES
              // Highlight the edges in red to show tile boundaries.
              gsl::span<uint32_t> pixels =
                  reintepretCastSpan<uint32_t, std::byte>(
                      loaded.image->pixelData);
              for (int32_t j = 0; j < loaded.image->height; ++j) {
                for (int32_t i = 0; i < loaded.image->width; ++i) {
                  if (i == 0 || j == 0 || i == loaded.image->width - 1 ||
                      j == loaded.image->height - 1) {
                    pixels[j * loaded.image->width + i] = 0xFF0000FF;
                  }
                }
              }
#endif

              return asyncSystem.createResolvedFuture(LoadedQuadtreeData{
                  std::make_shared<LoadedVectorOverlayData>(std::move(loaded)),
                  std::nullopt});
            }

            // Tile failed to load, try loading the parent tile instead.
            // We can only initiate a new tile request from the main thread,
            // though.
            if (currentLevel > minimumLevel) {
              return asyncSystem.runInMainThread(loadParentTile);
            } else {
              // No parent available, so return the original failed result.
              return asyncSystem.createResolvedFuture(LoadedQuadtreeData{
                  std::make_shared<LoadedVectorOverlayData>(std::move(loaded)),
                  std::nullopt});
            }
          });

  auto newIt = this->_tilesOldToRecent.emplace(
      this->_tilesOldToRecent.end(),
      CacheEntry{tileID, std::move(future).share()});
  this->_tileLookup[tileID] = newIt;

  SharedFuture<LoadedQuadtreeData> result = newIt->future;

  this->unloadCachedTiles();

  return result;
}

namespace {

PixelRectangle computePixelRectangle(
    const ImageCesium& image,
    const Rectangle& totalRectangle,
    const Rectangle& partRectangle) {
  // Pixel coordinates are measured from the top left.
  // Projected rectangles are measured from the bottom left.

  int32_t x = static_cast<int32_t>(Math::roundDown(
      image.width * (partRectangle.minimumX - totalRectangle.minimumX) /
          totalRectangle.computeWidth(),
      pixelTolerance));
  x = glm::max(0, x);
  int32_t y = static_cast<int32_t>(Math::roundDown(
      image.height * (totalRectangle.maximumY - partRectangle.maximumY) /
          totalRectangle.computeHeight(),
      pixelTolerance));
  y = glm::max(0, y);

  int32_t maxX = static_cast<int32_t>(Math::roundUp(
      image.width * (partRectangle.maximumX - totalRectangle.minimumX) /
          totalRectangle.computeWidth(),
      pixelTolerance));
  maxX = glm::min(maxX, image.width);
  int32_t maxY = static_cast<int32_t>(Math::roundUp(
      image.height * (totalRectangle.maximumY - partRectangle.minimumY) /
          totalRectangle.computeHeight(),
      pixelTolerance));
  maxY = glm::min(maxY, image.height);

  return PixelRectangle{x, y, maxX - x, maxY - y};
}

} // namespace

CesiumAsync::Future<LoadedVectorOverlayData>
QuadtreeVectorOverlayTileProvider::loadTileData(
    VectorOverlayTile& overlayTile)
{
  // Figure out which quadtree level we need, and which tiles from that level.
  // Load each needed tile (or pull it from cache).
  std::vector<CesiumAsync::SharedFuture<LoadedQuadtreeData>> tiles =
      this->mapVectorTilesToGeometryTile(
          overlayTile.getRectangle(),
          overlayTile.getTargetScreenPixels());

  return this->getAsyncSystem()
      .all(std::move(tiles))
      .thenInWorkerThread([projection = this->getProjection(),
                           rectangle = overlayTile.getRectangle()](std::vector<LoadedQuadtreeData>&& models)
        {
        // This set of images is only "useful" if at least one actually has
        // image data, and that image data is _not_ from an ancestor. We can
        // identify ancestor images because they have a `subset`.
        const bool haveAnyUsefulImageData = std::any_of(
            models.begin(),
            models.end(),
            [](const LoadedQuadtreeData& model)
          {
              return model.pLoaded->vectorModel.has_value() &&
                     !model.subset.has_value();
            });

        if (!haveAnyUsefulImageData)
        {
          // For non-useful sets of images, just return an empty image,
          // signalling that the parent tile should be used instead.
          // See https://github.com/CesiumGS/cesium-native/issues/316 for an
          // edge case that is not yet handled.
          return LoadedVectorOverlayData{
              VectorModel(),
              Rectangle(),
              {},
              {},
              {}};
        }

        VectorModel model = models.at(0).pLoaded->vectorModel.value();
        return LoadedVectorOverlayData{std::move(model), rectangle, {}, {}, {}};
      });
}

void QuadtreeVectorOverlayTileProvider::unloadCachedTiles() {
  CESIUM_TRACE("QuadtreeVectorOverlayTileProvider::unloadCachedTiles");

  const int64_t maxCacheBytes = this->getOwner().getOptions().subTileCacheBytes;
  if (this->_cachedBytes <= maxCacheBytes) {
    return;
  }

  auto it = this->_tilesOldToRecent.begin();

  while (it != this->_tilesOldToRecent.end() &&
         this->_cachedBytes > maxCacheBytes) {
    const SharedFuture<LoadedQuadtreeData>& future = it->future;
    if (!future.isReady()) {
      // Don't unload tiles that are still loading.
      ++it;
      continue;
    }

    // Guaranteed not to block because isReady returned true.
    const LoadedQuadtreeData& data = future.wait();

    std::shared_ptr<LoadedVectorOverlayData> pData = data.pLoaded;

    this->_tileLookup.erase(it->tileID);
    it = this->_tilesOldToRecent.erase(it);

    // If this is the last use of this data, it will be freed when the shared
    // pointer goes out of scope, so reduce the cachedBytes accordingly.
    if (pData.use_count() == 1) {
      if (pData->vectorModel) {
        this->_cachedBytes -= int64_t(pData->vectorModel->layers.size());
        assert(this->_cachedBytes >= 0);
      }
    }
  }
}

} // namespace Cesium3DTilesSelection
