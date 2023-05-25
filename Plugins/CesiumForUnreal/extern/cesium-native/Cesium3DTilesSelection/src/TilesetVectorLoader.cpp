#include "TilesetVectorLoader.h"

#include "ImplicitOctreeLoader.h"
#include "ImplicitQuadtreeLoader.h"
#include "TilesetJsonLoader.h"
#include "logTileLoadResult.h"

#include <Cesium3DTilesSelection/GltfConverters.h>
#include <Cesium3DTilesSelection/TileID.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeometry/BoundingSphere.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/S2CellBoundingVolume.h>
#include <CesiumUtility/Uri.h>
#include <spdlog/logger.h>

#include <cctype>

namespace Cesium3DTilesSelection
{

  TilesetVectorLoader::TilesetVectorLoader(
      const std::string& baseUrl)
      : _baseUrl{baseUrl} {}

  CesiumAsync::Future<TilesetContentLoaderResult<TilesetVectorLoader>>
  TilesetVectorLoader::createLoader(
      const TilesetExternals& externals,
      const std::string& tilesetJsonUrl,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders)
{
    return externals.pAssetAccessor->get(externals.asyncSystem, tilesetJsonUrl, requestHeaders)
        .thenInWorkerThread([pLogger = externals.pLogger](
                                const std::shared_ptr<CesiumAsync::IAssetRequest>&
                                    pCompletedRequest)
          {
          const CesiumAsync::IAssetResponse* pResponse = pCompletedRequest->response();
          const std::string& tileUrl = pCompletedRequest->url();
          if (!pResponse)
          {
            TilesetContentLoaderResult<TilesetVectorLoader> result;
            result.errors.emplaceError(fmt::format(
                "Did not receive a valid response for tile content {}",
                tileUrl));
            return result;
          }
              return TilesetContentLoaderResult<TilesetVectorLoader>{};
        });
  }

  TilesetContentLoaderResult<TilesetVectorLoader>
  TilesetVectorLoader::createLoader(
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::string& tilesetJsonUrl)
  {
    pLogger;
    tilesetJsonUrl;
    return TilesetContentLoaderResult<TilesetVectorLoader>{};
  }

  std::vector<CesiumGeospatial::Projection>
  TilesetVectorLoader::mapOverlaysToTile(Tile& tile, VectorOverlayCollection& overlays,
      const TilesetOptions& tilesetOptions)
{
    //清空VectorMappedTo3DTile，然后再VectorMappedTo3DTile::mapOverlayToTile中重新添加
    tile.getMappedVectorTiles().clear();
    std::vector<CesiumGeospatial::Projection> projections;
    const std::vector<
        CesiumUtility::IntrusivePointer<VectorOverlayTileProvider>>&
        tileProviders = overlays.getTileProviders();
    const std::vector<
        CesiumUtility::IntrusivePointer<VectorOverlayTileProvider>>&
        placeholders = overlays.getPlaceholderTileProviders();
    assert(tileProviders.size() == placeholders.size());

    for (size_t i = 0; i < tileProviders.size() && i < placeholders.size();
         ++i) {
      VectorOverlayTileProvider& tileProvider = *tileProviders[i];
      VectorOverlayTileProvider& placeholder = *placeholders[i];

      VectorMappedTo3DTile* pMapped = VectorMappedTo3DTile::mapOverlayToTile(
          tilesetOptions.maximumScreenSpaceError,
          tileProvider,
          placeholder,
          tile,
          projections);
      if (pMapped) {
        // Try to load now, but if the mapped raster tile is a placeholder this
        // won't do anything.
        pMapped->loadThrottled();
      }
    }

    return projections;
  }

  CesiumAsync::Future<VectorTileLoadResult>
  TilesetVectorLoader::loadTileContent(const TileLoadInput& loadInput)
  {
    const std::string* url = std::get_if<std::string>(&loadInput.tile.getTileID());
    const auto& asyncSystem = loadInput.asyncSystem;
    const auto& pAssetAccessor = loadInput.pAssetAccessor;
    const auto& pLogger = loadInput.pLogger;
    const auto& requestHeaders = loadInput.requestHeaders;
    const auto& contentOptions = loadInput.contentOptions;

     return pAssetAccessor->get(asyncSystem, *url, requestHeaders)
        .thenInWorkerThread([pLogger, contentOptions](
                                std::shared_ptr<CesiumAsync::IAssetRequest>&&
                                    pCompletedRequest) mutable
          {
          //auto pResponse = pCompletedRequest->response();
          pCompletedRequest;
          return VectorTileLoadResult{};
          });
  }

  TileChildrenResult TilesetVectorLoader::createTileChildren(const Tile& tile)
  {
     tile;
    return {{}, TileLoadResultState::Failed};
  }

  const std::string& TilesetVectorLoader::getBaseUrl() const noexcept
  {
    return this->_baseUrl;
  }

  void TilesetVectorLoader::addChildLoader(std::unique_ptr<TilesetVectorLoader> pLoader)
  {
    this->_children.emplace_back(std::move(pLoader));
  }
} // namespace Cesium3DTilesSelection
