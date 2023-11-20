#include "Cesium3DTilesSelection/VectorOverlayTileProvider.h"

#include "Cesium3DTilesSelection/IPrepareVectorMapResources.h"
#include "Cesium3DTilesSelection/VectorOverlay.h"
#include "Cesium3DTilesSelection/VectorOverlayTile.h"
#include "Cesium3DTilesSelection/TileID.h"
#include "Cesium3DTilesSelection/TilesetExternals.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"

#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/Tracing.h>
#include <CesiumUtility/joinToString.h>

#include <rapidjson/document.h>
#include <sstream>
#include <iostream>
#include "../include/Cesium3DTilesSelection/MVTUtilities.h"

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumGltf;
using namespace CesiumGltfReader;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {

/*static*/ CesiumGltfReader::VectorReader VectorOverlayTileProvider::_vectorReader {};

void replace(std::string& srs, std::string target, std::string replace) {
  size_t pos = 0;
  while ((pos = srs.find(target)) != std::string::npos) {
    size_t aa = target.length();
    srs = srs.replace(pos, aa, replace);
  }
}

VectorOverlayTileProvider::VectorOverlayTileProvider(
    const CesiumUtility::IntrusivePointer<const VectorOverlay>& pOwner,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor) noexcept
    : _pOwner(const_intrusive_cast<VectorOverlay>(pOwner)),
      _asyncSystem(asyncSystem),
      _pAssetAccessor(pAssetAccessor),
      _credit(std::nullopt),
      _pPrepareMapResources(nullptr),
      _pLogger(nullptr),
      _projection(CesiumGeospatial::GeographicProjection()),
      _coverageRectangle(CesiumGeospatial::GeographicProjection::
                             computeMaximumProjectedRectangle()),
      _pPlaceholder(),
      _tileDataBytes(0),
      _totalTilesCurrentlyLoading(0),
      _throttledTilesCurrentlyLoading(0) 
    {
          this->_pPlaceholder = new VectorOverlayTile(*this);
    }

VectorOverlayTileProvider::VectorOverlayTileProvider(
    const CesiumUtility::IntrusivePointer<const VectorOverlay>& pOwner,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    std::optional<Credit> credit,
    const std::shared_ptr<IPrepareVectorMapResources>& pPrepareMapResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const CesiumGeospatial::Projection& projection,
    const Rectangle& coverageRectangle) noexcept
    : _pOwner(const_intrusive_cast<VectorOverlay>(pOwner)),
      _asyncSystem(asyncSystem),
      _pAssetAccessor(pAssetAccessor),
      _credit(credit),
      _pPrepareMapResources(pPrepareMapResources),
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
  if(pTile->getVectorModel() != nullptr)
    this->_tileDataBytes -= int64_t(pTile->getVectorModel()->layers.size());
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
      [options = std::move(options), vecStyle = this->_vecStyle](
          std::shared_ptr<IAssetRequest>&& pRequest) mutable
      {
        std::string tileUrl = pRequest->url();
        CESIUM_TRACE("load: " + tileUrl);
        const IAssetResponse* pResponse = pRequest->response();
        if (pResponse == nullptr)
        {
          return LoadedVectorOverlayData{
              nullptr,
              options.rectangle,
              std::move(options.credits),
              {"Vector request for " + tileUrl + " failed."},
              {}};
        }

	    auto noneModel = new CesiumGltf::VectorTile;
        noneModel->level = options.level;
        noneModel->row = options.Row;
        noneModel->col = options.Col;
        noneModel->sourceName = options.sourceName;
        int statusCode = pResponse->statusCode();
        if(statusCode == 400 || statusCode == 204) 
        {
            noneModel->status = "400 None Data";
          return LoadedVectorOverlayData{
              noneModel,
              options.rectangle,
              std::move(options.credits),
              {},
              {}};
        }

        if (statusCode != 200)
        {
          std::string message = "vector response code " +
                                std::to_string(statusCode) +
                                " for " + tileUrl;
          noneModel->status = std::to_string(statusCode) + " None Data";
          return LoadedVectorOverlayData{
              nullptr,
              options.rectangle,
              std::move(options.credits),
              {message},
              {}};
        }

        if (pResponse->data().empty())
        {
            noneModel->status = "200 None Data";
          return LoadedVectorOverlayData{
              noneModel,
              options.rectangle,
              std::move(options.credits),
              {},
              {}};
        }
        const gsl::span<const std::byte> data = pResponse->data();
        if (pResponse->headers().find("x-encrypted") != pResponse->headers().end())
        {
            std::string strEncrypted = pResponse->headers().at("x-encrypted");
            bool isDecode = strEncrypted._Equal("true");
            isDecode;
        }
        std::string strDataLength = pResponse->headers().at("Content-Length");
        size_t dataSize = std::stoll(strDataLength);
        const char* charData = reinterpret_cast<const char*>(data.data());
        const std::string stringData(charData, dataSize);
        VectorReaderOptions readOpt;
        
        readOpt.decodeDraco = options.isDecode;
        CesiumGltfReader::VectorReaderResult loadedData = _vectorReader.readVector(stringData, readOpt);
        if (!loadedData.errors.empty())
        {
          return LoadedVectorOverlayData{
              nullptr,
              options.rectangle,
              std::move(options.credits),
              {"vector request for " + tileUrl + " failed."},
              {}};
        }
		loadedData.model->level = options.level;
		loadedData.model->row = options.Row;
		loadedData.model->col = options.Col;
        loadedData.model->sourceName = options.sourceName;
        //loadedData.model->style = vecStyle;
        //解析瓦片范围
        if (pResponse->headers().find("geowebcache-tile-bounds") != pResponse->headers().end())
        {
            std::string strTileBound = pResponse->headers().at("geowebcache-tile-bounds");
            replace(strTileBound, ",", " ");
            std::istringstream streamBound(strTileBound);
            streamBound >> loadedData.model->extentMin.x;
            streamBound >> loadedData.model->extentMin.y;
            streamBound >> loadedData.model->extentMax.x;
            streamBound >> loadedData.model->extentMax.y;
        }

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

        delete noneModel;
        return resurl;
      }
  );
 
}

namespace {
struct LoadResult {
  VectorOverlayTile::LoadState state = VectorOverlayTile::LoadState::Unloaded;
  CesiumGltf::VectorTile* model = {};
  CesiumGeometry::Rectangle rectangle = {};
  std::vector<Credit> credits = {};
  void* pRendererResources = nullptr;
};

    static LoadResult createLoadResultFromLoadedData(
        const std::shared_ptr<IPrepareVectorMapResources>& pPrepareMapResources,
        const std::shared_ptr<spdlog::logger>& pLogger,
        LoadedVectorOverlayData&& loadedData,
        const std::any& rendererOptions)
    {
     //警告级别太高，变量，参数声明没有引用会报错
      pLogger;

      if (!loadedData.errors.empty()) {
        LoadResult result;
        result.state = VectorOverlayTile::LoadState::Failed;
        SPDLOG_INFO("TileError {}", loadedData.errors[0]);
        return result;
      }

      if (loadedData.vectorModel == nullptr) 
      {
        LoadResult result;
        result.state = VectorOverlayTile::LoadState::Failed;
        return result;
      } 
      else 
      {
        void* pRendererResources = nullptr;
        if (pPrepareMapResources) {
          pRendererResources = pPrepareMapResources->prepareVectorInLoadThread(
              loadedData.vectorModel,
              rendererOptions);
        }

        LoadResult result;
        result.state = VectorOverlayTile::LoadState::Loaded;
        result.model = loadedData.vectorModel;
        result.rectangle = loadedData.rectangle;
        result.credits = std::move(loadedData.credits);
        result.pRendererResources = pRendererResources;
        return result;
      }
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
          [pPrepareMapResources = this->getPrepareMapResources(),
           pLogger = this->getLogger(),
           rendererOptions = this->_pOwner->getOptions().rendererOptions,
           strID = tile.getTileID()](
              LoadedVectorOverlayData&& loadedData)
        {
              LoadResult result = createLoadResultFromLoadedData(
                pPrepareMapResources,
                pLogger,
                std::move(loadedData),
                rendererOptions);
              if(result.state == VectorOverlayTile::LoadState::Unloaded) {
                  return result;
              }
            return result;
          })
      .thenInMainThread(
          [thisPtr, pTile, isThrottledLoad](LoadResult&& result) noexcept
        {
            pTile->_rectangle = result.rectangle;
            pTile->_pRendererResources = result.pRendererResources;
            pTile->_vectorModel = result.model;
            pTile->_tileCredits = std::move(result.credits);
            pTile->setState(result.state);
            // 
            if (pTile->getVectorModel() != nullptr) 
			{
				 thisPtr->_tileDataBytes +=
                int64_t(pTile->getVectorModel()->layers.size());
            } 

            thisPtr->finalizeTileLoad(isThrottledLoad);
          })
      .catchInMainThread(
          [thisPtr, pTile, isThrottledLoad](const std::exception& /*e*/)
        {
            pTile->_pRendererResources = nullptr;
            pTile->_vectorModel = nullptr;
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
