#include "Cesium3DTilesSelection/VectorOverlayTileProvider.h"

#include "Cesium3DTilesSelection/IRendererResourcesWorker.h"
#include "Cesium3DTilesSelection/VectorOverlay.h"
#include "Cesium3DTilesSelection/VectorOverlayTile.h"
#include "Cesium3DTilesSelection/TileID.h"
#include "Cesium3DTilesSelection/TilesetExternals.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"

#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/Tracing.h>
#include <CesiumUtility/joinToString.h>

#include <rapidjson/document.h>
#include <iostream>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumGltf;
using namespace CesiumGltfReader;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {

/*static*/ CesiumGltfReader::VectorReader VectorOverlayTileProvider::_vectorReader {};

VectorOverlayTileProvider::VectorOverlayTileProvider(
    const CesiumUtility::IntrusivePointer<const VectorOverlay>& pOwner,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor) noexcept
    : _pOwner(const_intrusive_cast<VectorOverlay>(pOwner)),
      _asyncSystem(asyncSystem),
      _pAssetAccessor(pAssetAccessor),
      _credit(std::nullopt),
      _pRendererResourcesWorker(nullptr),
      _pLogger(nullptr),
      _projection(CesiumGeospatial::GeographicProjection()),
      _coverageRectangle(CesiumGeospatial::GeographicProjection::
                             computeMaximumProjectedRectangle()),
      _pPlaceholder(),
      _tileDataBytes(0),
      _totalTilesCurrentlyLoading(0),
      _throttledTilesCurrentlyLoading(0) {
  this->_pPlaceholder = new VectorOverlayTile(*this);
}

VectorOverlayTileProvider::VectorOverlayTileProvider(
    const CesiumUtility::IntrusivePointer<const VectorOverlay>& pOwner,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    std::optional<Credit> credit,
    const std::shared_ptr<IRendererResourcesWorker>& pRendererResourcesWorker,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const CesiumGeospatial::Projection& projection,
    const Rectangle& coverageRectangle) noexcept
    : _pOwner(const_intrusive_cast<VectorOverlay>(pOwner)),
      _asyncSystem(asyncSystem),
      _pAssetAccessor(pAssetAccessor),
      _credit(credit),
      _pRendererResourcesWorker(pRendererResourcesWorker),
      _pLogger(pLogger),
      _projection(projection),
      _coverageRectangle(coverageRectangle),
      _pPlaceholder(nullptr),
      _tileDataBytes(0),
      _totalTilesCurrentlyLoading(0),
      _throttledTilesCurrentlyLoading(0) {}

VectorOverlayTileProvider::~VectorOverlayTileProvider() noexcept {
  // Explicitly release the placeholder first, because VectorOverlayTiles must
  // be destroyed before the tile provider that created them.
  if (this->_pPlaceholder)
  {
    assert(this->_pPlaceholder->getReferenceCount() == 1);
    this->_pPlaceholder = nullptr;
  }
}

CesiumUtility::IntrusivePointer<VectorOverlayTile>
VectorOverlayTileProvider::getTile(
    const CesiumGeometry::Rectangle& rectangle,
    const glm::dvec2& targetScreenPixels) {
  if (this->_pPlaceholder) {
    return this->_pPlaceholder;
  }

  if (!rectangle.overlaps(this->_coverageRectangle))
  {
    return nullptr;
  }

  return new VectorOverlayTile(*this, targetScreenPixels, rectangle);
}

void VectorOverlayTileProvider::removeTile(VectorOverlayTile* pTile) noexcept
{
  assert(pTile->getReferenceCount() == 0);
  pTile; //test
  //this->_tileDataBytes -= int64_t(pTile->getVectorModel().pixelData.size());
}

void VectorOverlayTileProvider::loadTile(VectorOverlayTile& tile)
{
  if (this->_pPlaceholder) {
    // Refuse to load placeholders.
    return;
  }

  this->doLoad(tile, false);
}

bool VectorOverlayTileProvider::loadTileThrottled(VectorOverlayTile& tile) {
  if (tile.getState() != VectorOverlayTile::LoadState::Unloaded) {
    return true;
  }

  if (this->_throttledTilesCurrentlyLoading >=
      this->getOwner().getOptions().maximumSimultaneousTileLoads) {
    return false;
  }

  this->doLoad(tile, true);
  return true;
}

CesiumAsync::Future<LoadedVectorOverlayData>
VectorOverlayTileProvider::loadTileDataFromUrl(
    const std::string& url,
    const std::vector<IAssetAccessor::THeader>& headers,
    LoadVectorTileDataFromUrlOptions&& options) const
{
  return this->getAssetAccessor()
      ->get(this->getAsyncSystem(), url, headers)
      .thenInWorkerThread(
      [options = std::move(options)](
          std::shared_ptr<IAssetRequest>&& pRequest) mutable
      {
        std::string tileUrl = pRequest->url();
        CESIUM_TRACE("load: " + tileUrl);
        const IAssetResponse* pResponse = pRequest->response();
        if (pResponse == nullptr)
        {
          return LoadedVectorOverlayData{
              {},
              options.rectangle,
              std::move(options.credits),
              {"Vector request for " + tileUrl + " failed."},
              {}};
        }
		
        if (pResponse->statusCode() != 0 &&
            (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300))
        {
          std::string message = "vector response code " +
                                std::to_string(pResponse->statusCode()) +
                                " for " + tileUrl;
          return LoadedVectorOverlayData{
              {},
              options.rectangle,
              std::move(options.credits),
              {message},
              {}};
        }

        if (pResponse->data().empty())
        {

          return LoadedVectorOverlayData{
              {},
              options.rectangle,
              std::move(options.credits),
              {"vector request for " + tileUrl + " failed."},
              {}};
        }
        const gsl::span<const std::byte> data = pResponse->data();
        std::string strDataLength = pResponse->headers().at("Content-Length");
        size_t dataSize = std::stoll(strDataLength);
        const char* charData = reinterpret_cast<const char*>(data.data());
        const std::string stringData(charData, dataSize);
        CesiumGltfReader::VectorReaderResult loadedData = _vectorReader.readVector(stringData);
		loadedData.model.level = options.level;
		loadedData.model.Row = options.Row;
		loadedData.model.Col = options.Col;

        if (!loadedData.errors.empty()) {
          loadedData.errors.push_back("tile url: " + tileUrl);
        }
        if (!loadedData.warnings.empty()) {
          loadedData.warnings.push_back("tile url: " + tileUrl);
        }

        LoadedVectorOverlayData resurl = LoadedVectorOverlayData{
            loadedData.model,
            options.rectangle,
            std::move(options.credits),
            std::move(loadedData.errors),
            std::move(loadedData.warnings)};
        return resurl;
      }
  );
 
}

namespace {
struct LoadResult {
  VectorOverlayTile::LoadState state = VectorOverlayTile::LoadState::Unloaded;
  CesiumGltf::VectorModel model = {};
  CesiumGeometry::Rectangle rectangle = {};
  std::vector<Credit> credits = {};
  void* pRendererResources = nullptr;
};

static LoadResult createLoadResultFromLoadedData(
    const std::shared_ptr<IRendererResourcesWorker>& pRendererResourcesWorker,
    const std::shared_ptr<spdlog::logger>& pLogger,
    LoadedVectorOverlayData&& loadedData,
    const std::any& rendererOptions)
{
  if (!loadedData.vectorModel.has_value()) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Failed to load vector for tile {}:\n- {}",
        "TODO",
        // Cesium3DTilesSelection::TileIdUtilities::createTileIdString(tileId),
        CesiumUtility::joinToString(loadedData.errors, "\n- "));
    LoadResult result;
    result.state = VectorOverlayTile::LoadState::Failed;
    return result;
  }

  if (!loadedData.warnings.empty()) {
    SPDLOG_LOGGER_WARN(
        pLogger,
        "Warnings while loading vector for tile {}:\n- {}",
        "TODO",
        // Cesium3DTilesSelection::TileIdUtilities::createTileIdString(tileId),
        CesiumUtility::joinToString(loadedData.warnings, "\n- "));
  }

  CesiumGltf::VectorModel& model = loadedData.vectorModel.value();
  if (model.layers.size() > 0)
  {
    void* pRendererResources = nullptr;
    if (pRendererResourcesWorker) 
	{
      pRendererResources = pRendererResourcesWorker->prepareVectorInLoadThread(
          model,
          rendererOptions);
    }

    LoadResult result;
    result.state = VectorOverlayTile::LoadState::Loaded;
    result.model = std::move(model);
    result.rectangle = loadedData.rectangle;
    result.credits = std::move(loadedData.credits);
    result.pRendererResources = pRendererResources;
    return result;
  }

  LoadResult result;
  result.pRendererResources = nullptr;
  result.state = VectorOverlayTile::LoadState::Failed;
  return result;
}

} // namespace

void VectorOverlayTileProvider::doLoad(VectorOverlayTile& tile, bool isThrottledLoad)
{
  if (tile.getState() != VectorOverlayTile::LoadState::Unloaded)
  {
    // Already loading or loaded, do nothing.
    return;
  }

  // CESIUM_TRACE_USE_TRACK_SET(this->_loadingSlots);

  // Don't let this tile be destroyed while it's loading.
  tile.setState(VectorOverlayTile::LoadState::Loading);

  this->beginTileLoad(isThrottledLoad);

  // Keep the tile and tile provider alive while the async operation is in
  // progress.
  IntrusivePointer<VectorOverlayTile> pTile = &tile;
  IntrusivePointer<VectorOverlayTileProvider> thisPtr = this;

  this->loadTileData(tile)
      .thenInWorkerThread(
          [pRendererResourcesWorker = this->getRendererResourcesWorker(),
           pLogger = this->getLogger(),
           rendererOptions = this->_pOwner->getOptions().rendererOptions](
              LoadedVectorOverlayData&& loadedData)
        {
            return createLoadResultFromLoadedData(
                pRendererResourcesWorker,
                pLogger,
                std::move(loadedData),
                rendererOptions);
          })
      .thenInMainThread(
          [thisPtr, pTile, isThrottledLoad](LoadResult&& result) noexcept
        {
            pTile->_rectangle = result.rectangle;
            pTile->_pRendererResources = result.pRendererResources;
            pTile->_vectorModel = std::move(result.model);
            pTile->_tileCredits = std::move(result.credits);
            pTile->setState(result.state);
            //ÒªÖØÐ´
            thisPtr->_tileDataBytes +=
                int64_t(pTile->getVectorModel().layers.size());

            thisPtr->finalizeTileLoad(isThrottledLoad);
          })
      .catchInMainThread(
          [thisPtr, pTile, isThrottledLoad](const std::exception& /*e*/)
        {
            pTile->_pRendererResources = nullptr;
            pTile->_vectorModel = {};
            pTile->_tileCredits = {};
            pTile->setState(VectorOverlayTile::LoadState::Failed);

            thisPtr->finalizeTileLoad(isThrottledLoad);
          });
}

void VectorOverlayTileProvider::beginTileLoad(bool isThrottledLoad) noexcept {
  ++this->_totalTilesCurrentlyLoading;
  if (isThrottledLoad) {
    ++this->_throttledTilesCurrentlyLoading;
  }
}

void VectorOverlayTileProvider::finalizeTileLoad(
    bool isThrottledLoad) noexcept {
  --this->_totalTilesCurrentlyLoading;
  if (isThrottledLoad) {
    --this->_throttledTilesCurrentlyLoading;
  }
}
} // namespace Cesium3DTilesSelection
