#pragma once

#include "CreditSystem.h"
#include "Library.h"
#include "VectorMappedTo3DTile.h"

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGltfReader/VectorReader.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCountedNonThreadSafe.h>
#include <CesiumUtility/Tracing.h>

#include <spdlog/fwd.h>

#include <cassert>
#include <optional>

namespace Cesium3DTilesSelection 
{

class VectorOverlay;
class VectorOverlayTile;
class IPrepareVectorMapResources;

struct BoxExtent
{
	double LowerCornerLon = 0.0;
	double LowerCornerLat = 0.0;
	double UpperCornerLon = 0.0;
	double UpperCornerLat = 0.0;
};

struct TileMatrix 
{
    std::string Identifier = "";
	double ScaleDenominator = 0.0;
    double TopLeftCornerLon = 0.0;
    double TopLeftCornerLat = 0.0;
    int TileWidth = 0.0;
    int TileHeight = 0.0;
    int MatrixWidth = 0.0;
    int MatrixHeight = 0.0;
};

struct TileMatrixSet 
{
    int MinTileRow = 0;
    int MaxTileRow = 0;
    int MinTileCol = 0;
    int MaxTileCol = 0;
};

/**
 * @brief Summarizes the result of loading an image of a {@link VectorOverlay}.
 */
struct CESIUM3DTILESSELECTION_API LoadedVectorOverlayData {

  CesiumGltf::VectorTile* vectorModel = nullptr;

  CesiumGeometry::Rectangle rectangle = {};

  std::vector<Credit> credits = {};

  std::vector<std::string> errors = {};

  std::vector<std::string> warnings = {};

  // 判断是否为有效的数据请求
  bool isSuccess() {
    if (errors.empty() && warnings.empty()) {
      return true;
    }
    return false;
  }
};

/**
 * @brief Options for {@link VectorOverlayTileProvider::loadTileDataFromUrl}.
 */
struct LoadVectorTileDataFromUrlOptions {
  /**
   * @brief The rectangle definining the bounds of the image being loaded,
   * expressed in the {@link VectorOverlayTileProvider}'s projection.
   */
  CesiumGeometry::Rectangle rectangle{};

  /**
   * @brief The credits to display with this tile.
   *
   * This property is copied verbatim to the
   * {@link LoadedVectorOverlayData::credits} property.
   */
  std::vector<Credit> credits{};

  /**
   * @brief Whether empty (zero length) images are accepted as a valid
   * response.
   *
   */
  bool allowEmptyVector = false;

  // 当前瓦片的层级
  int level = 0;

  //瓦片的行号
  int Row = 0;

  //瓦片的列号
  int Col = 0;

  std::string sourceName = "";

  bool isDecode = false;
};

/**
 * @brief Provides individual tiles for a {@link VectorOverlay} on demand.
 *
 * Instances of this class must be allocated on the heap, and their lifetimes
 * must be managed with {@link CesiumUtility::IntrusivePointer}.
 */
class CESIUM3DTILESSELECTION_API VectorOverlayTileProvider
    : public CesiumUtility::ReferenceCountedNonThreadSafe<
          VectorOverlayTileProvider> {
public:
  /**
   * Constructs a placeholder tile provider.
   *
   * @see VectorOverlayTileProvider::isPlaceholder
   *
   * @param pOwner The Vector overlay that created this tile provider.
   * @param asyncSystem The async system used to do work in threads.
   * @param pAssetAccessor The interface used to obtain assets (tiles, etc.) for
   * this Vector overlay.
   */
  VectorOverlayTileProvider(
      const CesiumUtility::IntrusivePointer<const VectorOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>&
          pAssetAccessor) noexcept;

  /**
   * @brief Creates a new instance.
   *
   * @param pOwner The Vector overlay that created this tile provider.
   * @param asyncSystem The async system used to do work in threads.
   * @param pAssetAccessor The interface used to obtain assets (tiles, etc.) for
   * this Vector overlay.
   * @param credit The {@link Credit} for this tile provider, if it exists.
   * @param pPrepareMapResources The interface used to prepare Vector
   * images for rendering.
   * @param pLogger The logger to which to send messages about the tile provider
   * and tiles.
   * @param projection The {@link CesiumGeospatial::Projection}.
   * @param coverageRectangle The rectangle that bounds all the area covered by
   * this overlay, expressed in projected coordinates.
   */
  VectorOverlayTileProvider(
      const CesiumUtility::IntrusivePointer<const VectorOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      std::optional<Credit> credit,
      const std::shared_ptr<IPrepareVectorMapResources>&
          pPrepareMapResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const CesiumGeospatial::Projection& projection,
      const CesiumGeometry::Rectangle& coverageRectangle) noexcept;

  /** @brief Default destructor. */
  virtual ~VectorOverlayTileProvider() noexcept;

  /**
   * @brief Returns whether this is a placeholder.
   *
   * For many types of {@link VectorOverlay}, we can't create a functioning
   * `VectorOverlayTileProvider` right away. For example, we may not know the
   * bounds of the overlay, or what projection it uses, until after we've
   * (asynchronously) loaded a metadata service that gives us this information.
   *
   * So until that real `VectorOverlayTileProvider` becomes available, we use
   * a placeholder. When {@link VectorOverlayTileProvider::getTile} is invoked
   * on a placeholder, it returns a {@link VectorOverlayTile} that is also
   * a placeholder. And whenever we see a placeholder `VectorOverTile` in
   * {@link Tile::update}, we check if the corresponding `VectorOverlay` is
   * ready yet. Once it's ready, we remove the placeholder tile and replace
   * it with the real tiles.
   *
   * So the placeholder system gives us a way to defer the mapping of Vector
   * overlay tiles to geometry tiles until that mapping can be determined.
   */
  bool isPlaceholder() const noexcept { return this->_pPlaceholder != nullptr; }

  /**
   * @brief Returns the {@link VectorOverlay} that created this instance.
   */
  VectorOverlay& getOwner() noexcept { return *this->_pOwner; }

  /** @copydoc getOwner */
  const VectorOverlay& getOwner() const noexcept { return *this->_pOwner; }

  /**
   * @brief Get the system to use for asychronous requests and threaded work.
   */
  const std::shared_ptr<CesiumAsync::IAssetAccessor>&
  getAssetAccessor() const noexcept {
    return this->_pAssetAccessor;
  }

  /**
   * @brief Gets the async system used to do work in threads.
   */
  const CesiumAsync::AsyncSystem& getAsyncSystem() const noexcept {
    return this->_asyncSystem;
  }

  /**
   * @brief Gets the interface used to prepare Vector overlay images for
   * rendering.
   */
  const std::shared_ptr<IPrepareVectorMapResources>&
  getPrepareMapResources() const noexcept {
    return this->_pPrepareMapResources;
  }

  /**
   * @brief Gets the logger to which to send messages about the tile provider
   * and tiles.
   */
  const std::shared_ptr<spdlog::logger>& getLogger() const noexcept {
    return this->_pLogger;
  }

  /**
   * @brief Returns the {@link CesiumGeospatial::Projection} of this instance.
   */
  const CesiumGeospatial::Projection& getProjection() const noexcept {
    return this->_projection;
  }

  /**
   * @brief Returns the coverage {@link CesiumGeometry::Rectangle} of this
   * instance.
   */
  const CesiumGeometry::Rectangle& getCoverageRectangle() const noexcept {
    return this->_coverageRectangle;
  }

  /**
   * @brief Returns a new {@link VectorOverlayTile} with the given
   * specifications.
   *
   * The returned tile will not start loading immediately. To start loading,
   * call {@link VectorOverlayTileProvider::loadTile} or
   * {@link VectorOverlayTileProvider::loadTileThrottled}.
   *
   * @param rectangle The rectangle that the returned image must cover. It is
   * allowed to cover a slightly larger rectangle in order to maintain pixel
   * alignment. It may also cover a smaller rectangle when the overlay itself
   * does not cover the entire rectangle.
   * @param targetScreenPixels The maximum number of pixels on the screen that
   * this tile is meant to cover. The overlay image should be approximately this
   * many pixels divided by the
   * {@link VectorOverlayOptions::maximumScreenSpaceError} in order to achieve
   * the desired level-of-detail, but it does not need to be exactly this size.
   * @return The tile.
   */
  CesiumUtility::IntrusivePointer<VectorOverlayTile> getTile(
      const CesiumGeometry::Rectangle& rectangle,
      const glm::dvec2& targetScreenPixels);

  /**
   * @brief Gets the number of bytes of tile data that are currently loaded.
   */
  int64_t getTileDataBytes() const noexcept { return this->_tileDataBytes; }

  /**
   * @brief Returns the number of tiles that are currently loading.
   */
  uint32_t getNumberOfTilesLoading() const noexcept {
    assert(this->_totalTilesCurrentlyLoading > -1);
    return this->_totalTilesCurrentlyLoading;
  }

  /**
   * @brief Removes a no-longer-referenced tile from this provider's cache and
   * deletes it.
   *
   * This function is not supposed to be called by client. Calling this method
   * in a tile with a reference count greater than 0 will result in undefined
   * behavior.
   *
   * @param pTile The tile, which must have no oustanding references.
   */
  void removeTile(VectorOverlayTile* pTile) noexcept;

  /**
   * @brief Get the per-TileProvider {@link Credit} if one exists.
   */
  const std::optional<Credit>& getCredit() const noexcept { return _credit; }

  /**
   * @brief Loads a tile immediately, without throttling requests.
   *
   * If the tile is not in the `Tile::LoadState::Unloading` state, this method
   * returns without doing anything. Otherwise, it puts the tile into the
   * `Tile::LoadState::Loading` state and begins the asynchronous process
   * to load the tile. When the process completes, the tile will be in the
   * `Tile::LoadState::Loaded` or `Tile::LoadState::Failed` state.
   *
   * Calling this method on many tiles at once can result in very slow
   * performance. Consider using {@link loadTileThrottled} instead.
   *
   * @param tile The tile to load.
   */
  void loadTile(VectorOverlayTile& tile);

  /**
   * @brief Loads a tile, unless there are too many tile loads already in
   * progress.
   *
   * If the tile is not in the `Tile::LoadState::Unloading` state, this method
   * returns true without doing anything. If too many tile loads are
   * already in flight, it returns false without doing anything. Otherwise, it
   * puts the tile into the `Tile::LoadState::Loading` state, begins the
   * asynchronous process to load the tile, and returns true. When the process
   * completes, the tile will be in the `Tile::LoadState::Loaded` or
   * `Tile::LoadState::Failed` state.
   *
   * The number of allowable simultaneous tile requests is provided in the
   * {@link VectorOverlayOptions::maximumSimultaneousTileLoads} property of
   * {@link VectorOverlay::getOptions}.
   *
   * @param tile The tile to load.
   * @returns True if the tile load process is started or is already complete,
   * false if the load could not be started because too many loads are already
   * in progress.
   */
  bool loadTileThrottled(VectorOverlayTile& tile);

  BoxExtent _boxExtent;

  //<level,TileMatrix>
  std::map<int, TileMatrix> _TileMatrixMap;

  std::map<int, TileMatrixSet> _TileMatrixSetMap;

  CesiumGltf::VectorStyle _vecStyle;

protected:
  /**
   * @brief Loads the image for a tile.
   *
   * @param overlayTile The overlay tile for which to load the image.
   * @return A future that resolves to the image or error information.
   */
  virtual CesiumAsync::Future<LoadedVectorOverlayData>
  loadTileData(VectorOverlayTile& overlayTile) = 0;

  /**
   * @brief Loads an image from a URL and optionally some request headers.
   *
   * This is a useful helper function for implementing {@link loadTileData}.
   *
   * @param url The URL.
   * @param headers The request headers.
   * @param options Additional options for the load process.
   * @return A future that resolves to the image or error information.
   */
  CesiumAsync::Future<LoadedVectorOverlayData> loadTileDataFromUrl(
      const std::string& url,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers = {},
      LoadVectorTileDataFromUrlOptions&& options = {}) const;

private:
  void doLoad(VectorOverlayTile& tile, bool isThrottledLoad);

  /**
   * @brief Begins the process of loading of a tile.
   *
   * This method should be called at the beginning of the tile load process.
   *
   * @param isThrottledLoad True if the load was originally throttled.
   */
  void beginTileLoad(bool isThrottledLoad) noexcept;

  /**
   * @brief Finalizes loading of a tile.
   *
   * This method should be called at the end of the tile load process,
   * no matter whether the load succeeded or failed.
   *
   * @param isThrottledLoad True if the load was originally throttled.
   */
  void finalizeTileLoad(bool isThrottledLoad) noexcept;

private:
  CesiumUtility::IntrusivePointer<VectorOverlay> _pOwner;
  CesiumAsync::AsyncSystem _asyncSystem;
  std::shared_ptr<CesiumAsync::IAssetAccessor> _pAssetAccessor;
  std::optional<Credit> _credit;
  std::shared_ptr<IPrepareVectorMapResources> _pPrepareMapResources;
  std::shared_ptr<spdlog::logger> _pLogger;
  CesiumGeospatial::Projection _projection;
  CesiumGeometry::Rectangle _coverageRectangle;
  CesiumUtility::IntrusivePointer<VectorOverlayTile> _pPlaceholder;
  int64_t _tileDataBytes;
  int32_t _totalTilesCurrentlyLoading;
  int32_t _throttledTilesCurrentlyLoading;
 
  CESIUM_TRACE_DECLARE_TRACK_SET(
      _loadingSlots,
      "Vector Overlay Tile Loading Slot");

  static CesiumGltfReader::VectorReader _vectorReader;
};
} // namespace Cesium3DTilesSelection
