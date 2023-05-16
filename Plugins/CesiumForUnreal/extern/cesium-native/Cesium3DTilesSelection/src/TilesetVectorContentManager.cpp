#include "TilesetVectorContentManager.h"

#include "CesiumIonTilesetLoader.h"
#include "LayerJsonTerrainLoader.h"
#include "TileContentLoadInfo.h"
#include "TilesetJsonLoader.h"

#include <Cesium3DTilesSelection/GltfUtilities.h>
#include <Cesium3DTilesSelection/IRendererResourcesWorker.h>
#include <Cesium3DTilesSelection/VectorOverlay.h>
#include <Cesium3DTilesSelection/VectorOverlayTile.h>
#include <Cesium3DTilesSelection/VectorOverlayTileProvider.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/joinToString.h>

#include <rapidjson/document.h>
#include <spdlog/logger.h>

#include <chrono>

namespace Cesium3DTilesSelection {
namespace {
struct RegionAndCenter {
  CesiumGeospatial::BoundingRegion region;
  CesiumGeospatial::Cartographic center;
};

struct ContentKindSetter {
  void operator()(TileUnknownContent content) {
    tileContent.setContentKind(content);
  }

  void operator()(TileEmptyContent content) {
    tileContent.setContentKind(content);
  }

  void operator()(TileExternalContent content) {
    tileContent.setContentKind(content);
  }

  void operator()(CesiumGltf::Model&& model) {
    auto pRenderContent = std::make_unique<TileRenderContent>(std::move(model));
    pRenderContent->setRenderResources(pRenderResources);
    if (rectorOverlayDetails) {
      pRenderContent->setRasterOverlayDetails(std::move(*rectorOverlayDetails));
    }

    tileContent.setContentKind(std::move(pRenderContent));
  }

  TileContent& tileContent;
  std::optional<RasterOverlayDetails> rectorOverlayDetails;
  void* pRenderResources;
};

void unloadTileRecursively(
    Tile& tile,
    TilesetVectorContentManager& tilesetContentManager) {
  tilesetContentManager.unloadTileContent(tile);
  for (Tile& child : tile.getChildren()) {
    unloadTileRecursively(child, tilesetContentManager);
  }
}

bool anyVectorOverlaysNeedLoading(const Tile& tile) noexcept {
  for (const VectorMappedTo3DTile& mapped : tile.getMappedVectorTiles()) {
    const VectorOverlayTile* pLoading = mapped.getLoadingTile();
    if (pLoading &&
        pLoading->getState() == VectorOverlayTile::LoadState::Unloaded) {
      return true;
    }
  }

  return false;
}

std::vector<CesiumGeospatial::Projection> mapOverlaysToTile(
    Tile& tile,
    VectorOverlayCollection& overlays,
    const TilesetOptions& tilesetOptions) {
  // when tile fails temporarily, it may still have mapped Vector tiles, so
  // clear it here
  tile.getMappedVectorTiles().clear();

  std::vector<CesiumGeospatial::Projection> projections;
  const std::vector<CesiumUtility::IntrusivePointer<VectorOverlayTileProvider>>&
      tileProviders = overlays.getTileProviders();
  const std::vector<CesiumUtility::IntrusivePointer<VectorOverlayTileProvider>>&
      placeholders = overlays.getPlaceholderTileProviders();
  assert(tileProviders.size() == placeholders.size());

  for (size_t i = 0; i < tileProviders.size() && i < placeholders.size(); ++i) {
    VectorOverlayTileProvider& tileProvider = *tileProviders[i];
    VectorOverlayTileProvider& placeholder = *placeholders[i];

    VectorMappedTo3DTile* pMapped = VectorMappedTo3DTile::mapOverlayToTile(
        tilesetOptions.maximumScreenSpaceError,
        tileProvider,
        placeholder,
        tile,
        projections);
    if (pMapped) {
      // Try to load now, but if the mapped Vector tile is a placeholder this
      // won't do anything.
      pMapped->loadThrottled();
    }
  }

  return projections;
}

const BoundingVolume& getEffectiveBoundingVolume(
    const BoundingVolume& tileBoundingVolume,
    const std::optional<BoundingVolume>& updatedTileBoundingVolume,
    [[maybe_unused]] const std::optional<BoundingVolume>&
        updatedTileContentBoundingVolume) {
  // If we have an updated tile bounding volume, use it.
  if (updatedTileBoundingVolume) {
    return *updatedTileBoundingVolume;
  }

  // If we _only_ have an updated _content_ bounding volume, that's a developer
  // error.
  assert(!updatedTileContentBoundingVolume);

  return tileBoundingVolume;
}

const BoundingVolume& getEffectiveContentBoundingVolume(
    const BoundingVolume& tileBoundingVolume,
    const std::optional<BoundingVolume>& tileContentBoundingVolume,
    const std::optional<BoundingVolume>& updatedTileBoundingVolume,
    const std::optional<BoundingVolume>& updatedTileContentBoundingVolume) {
  // If we have an updated tile content bounding volume, use it.
  if (updatedTileContentBoundingVolume) {
    return *updatedTileContentBoundingVolume;
  }

  // Next best thing is an updated tile non-content bounding volume.
  if (updatedTileBoundingVolume) {
    return *updatedTileBoundingVolume;
  }

  // Then a content bounding volume attached to the tile.
  if (tileContentBoundingVolume) {
    return *tileContentBoundingVolume;
  }

  // And finally the regular tile bounding volume.
  return tileBoundingVolume;
}

void calcRasterOverlayDetailsInWorkerThread(
    TileLoadResult& result,
    std::vector<CesiumGeospatial::Projection>&& projections,
    const TileContentLoadInfo& tileLoadInfo) {
  CesiumGltf::Model& model = std::get<CesiumGltf::Model>(result.contentKind);

  // we will use the fittest bounding volume to calculate Vector overlay details
  // below
  const BoundingVolume& contentBoundingVolume =
      getEffectiveContentBoundingVolume(
          tileLoadInfo.tileBoundingVolume,
          tileLoadInfo.tileContentBoundingVolume,
          result.updatedBoundingVolume,
          result.updatedContentBoundingVolume);

  // If we have projections, generate texture coordinates for all of them. Also
  // remember the min and max height so that we can use them for upsampling.
  const CesiumGeospatial::BoundingRegion* pRegion =
      getBoundingRegionFromBoundingVolume(contentBoundingVolume);

  // remove any projections that are already used to generated UV
  int32_t firstVectorOverlayTexCoord = 0;
  if (result.rasterOverlayDetails) {
    const std::vector<CesiumGeospatial::Projection>& existingProjections =
        result.rasterOverlayDetails->rasterOverlayProjections;
    firstVectorOverlayTexCoord =
        static_cast<int32_t>(existingProjections.size());
    auto removedProjectionIt = std::remove_if(
        projections.begin(),
        projections.end(),
        [&](const CesiumGeospatial::Projection& proj) {
          return std::find(
                     existingProjections.begin(),
                     existingProjections.end(),
                     proj) != existingProjections.end();
        });
    projections.erase(removedProjectionIt, projections.end());
  }

  // generate the overlay details from the rest of projections and merge it with
  // the existing one
  auto overlayDetails = GltfUtilities::createRasterOverlayTextureCoordinates(
      model,
      tileLoadInfo.tileTransform,
      firstVectorOverlayTexCoord,
      pRegion ? std::make_optional(pRegion->getRectangle()) : std::nullopt,
      std::move(projections));

  if (pRegion && overlayDetails) {
    // If the original bounding region was wrong, report it.
    const CesiumGeospatial::GlobeRectangle& original = pRegion->getRectangle();
    const CesiumGeospatial::GlobeRectangle& computed =
        overlayDetails->boundingRegion.getRectangle();
    if ((!CesiumUtility::Math::equalsEpsilon(
             computed.getWest(),
             original.getWest(),
             0.01) &&
         computed.getWest() < original.getWest()) ||
        (!CesiumUtility::Math::equalsEpsilon(
             computed.getSouth(),
             original.getSouth(),
             0.01) &&
         computed.getSouth() < original.getSouth()) ||
        (!CesiumUtility::Math::equalsEpsilon(
             computed.getEast(),
             original.getEast(),
             0.01) &&
         computed.getEast() > original.getEast()) ||
        (!CesiumUtility::Math::equalsEpsilon(
             computed.getNorth(),
             original.getNorth(),
             0.01) &&
         computed.getNorth() > original.getNorth())) {

      auto it = model.extras.find("Cesium3DTiles_TileUrl");
      std::string url = it != model.extras.end()
                            ? it->second.getStringOrDefault("Unknown Tile URL")
                            : "Unknown Tile URL";
      SPDLOG_LOGGER_WARN(
          tileLoadInfo.pLogger,
          "Tile has a bounding volume that does not include all of its "
          "content, so culling and Vector overlays may be incorrect: {}",
          url);
    }
  }

  if (result.rasterOverlayDetails && overlayDetails) {
    result.rasterOverlayDetails->merge(*overlayDetails);
  } else if (overlayDetails) {
    result.rasterOverlayDetails = std::move(*overlayDetails);
  }
}

void calcFittestBoundingRegionForLooseTile(
    TileLoadResult& result,
    const TileContentLoadInfo& tileLoadInfo) {
  CesiumGltf::Model& model = std::get<CesiumGltf::Model>(result.contentKind);

  const BoundingVolume& boundingVolume = getEffectiveBoundingVolume(
      tileLoadInfo.tileBoundingVolume,
      result.updatedBoundingVolume,
      result.updatedContentBoundingVolume);
  if (std::get_if<CesiumGeospatial::BoundingRegionWithLooseFittingHeights>(
          &boundingVolume) != nullptr) {
    if (result.rasterOverlayDetails) {
      // We already computed the bounding region for overlays, so use it.
      result.updatedBoundingVolume =
          result.rasterOverlayDetails->boundingRegion;
    } else {
      // We need to compute an accurate bounding region
      result.updatedBoundingVolume = GltfUtilities::computeBoundingRegion(
          model,
          tileLoadInfo.tileTransform);
    }
  }
}

void postProcessGltfInWorkerThread(
    TileLoadResult& result,
    std::vector<CesiumGeospatial::Projection>&& projections,
    const TileContentLoadInfo& tileLoadInfo) {
  CesiumGltf::Model& model = std::get<CesiumGltf::Model>(result.contentKind);

  if (result.pCompletedRequest) {
    model.extras["Cesium3DTiles_TileUrl"] = result.pCompletedRequest->url();
  }

  // have to pass the up axis to extra for backward compatibility
  model.extras["gltfUpAxis"] =
      static_cast<std::underlying_type_t<CesiumGeometry::Axis>>(
          result.glTFUpAxis);

  // calculate Vector overlay details
  calcRasterOverlayDetailsInWorkerThread(
      result,
      std::move(projections),
      tileLoadInfo);

  // If our tile bounding region has loose fitting heights, find the real ones.
  calcFittestBoundingRegionForLooseTile(result, tileLoadInfo);

  // generate missing smooth normal
  if (tileLoadInfo.contentOptions.generateMissingNormalsSmooth) {
    model.generateMissingNormalsSmooth();
  }
}

CesiumAsync::Future<TileLoadResultAndRenderResources>
postProcessContentInWorkerThread(
    TileLoadResult&& result,
    std::vector<CesiumGeospatial::Projection>&& projections,
    TileContentLoadInfo&& tileLoadInfo,
    const std::any& rendererOptions) {
  assert(
      result.state == TileLoadResultState::Success &&
      "This function requires result to be success");

  CesiumGltf::Model& model = std::get<CesiumGltf::Model>(result.contentKind);

  // Download any external image or buffer urls in the gltf if there are any
  CesiumGltfReader::GltfReaderResult gltfResult{std::move(model), {}, {}};

  CesiumAsync::HttpHeaders requestHeaders;
  std::string baseUrl;
  if (result.pCompletedRequest) {
    requestHeaders = result.pCompletedRequest->headers();
    baseUrl = result.pCompletedRequest->url();
  }

  CesiumGltfReader::GltfReaderOptions gltfOptions;
  gltfOptions.ktx2TranscodeTargets =
      tileLoadInfo.contentOptions.ktx2TranscodeTargets;

  auto asyncSystem = tileLoadInfo.asyncSystem;
  auto pAssetAccessor = tileLoadInfo.pAssetAccessor;
  return CesiumGltfReader::GltfReader::resolveExternalData(
             asyncSystem,
             baseUrl,
             requestHeaders,
             pAssetAccessor,
             gltfOptions,
             std::move(gltfResult))
      .thenInWorkerThread(
          [result = std::move(result),
           projections = std::move(projections),
           tileLoadInfo = std::move(tileLoadInfo),
           rendererOptions](
              CesiumGltfReader::GltfReaderResult&& gltfResult) mutable {
            if (!gltfResult.errors.empty()) {
              if (result.pCompletedRequest) {
                SPDLOG_LOGGER_ERROR(
                    tileLoadInfo.pLogger,
                    "Failed resolving external glTF buffers from {}:\n- {}",
                    result.pCompletedRequest->url(),
                    CesiumUtility::joinToString(gltfResult.errors, "\n- "));
              } else {
                SPDLOG_LOGGER_ERROR(
                    tileLoadInfo.pLogger,
                    "Failed resolving external glTF buffers:\n- {}",
                    CesiumUtility::joinToString(gltfResult.errors, "\n- "));
              }
            }

            if (!gltfResult.warnings.empty()) {
              if (result.pCompletedRequest) {
                SPDLOG_LOGGER_WARN(
                    tileLoadInfo.pLogger,
                    "Warning when resolving external gltf buffers from "
                    "{}:\n- {}",
                    result.pCompletedRequest->url(),
                    CesiumUtility::joinToString(gltfResult.errors, "\n- "));
              } else {
                SPDLOG_LOGGER_ERROR(
                    tileLoadInfo.pLogger,
                    "Warning resolving external glTF buffers:\n- {}",
                    CesiumUtility::joinToString(gltfResult.errors, "\n- "));
              }
            }

            if (!gltfResult.model) {
              return tileLoadInfo.asyncSystem.createResolvedFuture(
                  TileLoadResultAndRenderResources{
                      TileLoadResult::createFailedResult(nullptr),
                      nullptr});
            }

            result.contentKind = std::move(*gltfResult.model);

            postProcessGltfInWorkerThread(
                result,
                std::move(projections),
                tileLoadInfo);

            // create render resources
            return tileLoadInfo.pPrepareRendererResources->prepareInLoadThread(
                tileLoadInfo.asyncSystem,
                std::move(result),
                tileLoadInfo.tileTransform,
                rendererOptions);
          });
}
} // namespace

TilesetVectorContentManager::TilesetVectorContentManager(
    const TilesetExternals& externals,
    const TilesetOptions& tilesetOptions,
    VectorOverlayCollection&& overlayCollection,
    std::vector<CesiumAsync::IAssetAccessor::THeader>&& requestHeaders,
    std::unique_ptr<TilesetContentLoader>&& pLoader,
    std::unique_ptr<Tile>&& pRootTile)
    : _externals{externals},
      _requestHeaders{std::move(requestHeaders)},
      _pLoader{std::move(pLoader)},
      _pRootTile{std::move(pRootTile)},
      _userCredit(
          (tilesetOptions.credit && externals.pCreditSystem)
              ? std::optional<Credit>(externals.pCreditSystem->createCredit(
                    tilesetOptions.credit.value(),
                    tilesetOptions.showCreditsOnScreen))
              : std::nullopt),
      _tilesetCredits{},
      _overlayCollection{std::move(overlayCollection)},
      _tileLoadsInProgress{0},
      _loadedTilesCount{0},
      _tilesDataUsed{0},
      _destructionCompletePromise{externals.asyncSystem.createPromise<void>()},
      _destructionCompleteFuture{
          this->_destructionCompletePromise.getFuture().share()} {}

TilesetVectorContentManager::TilesetVectorContentManager(
    const TilesetExternals& externals,
    const TilesetOptions& tilesetOptions,
    VectorOverlayCollection&& overlayCollection,
    const std::string& url)
    : _externals{externals},
      _requestHeaders{},
      _pLoader{},
      _pRootTile{},
      _userCredit(
          (tilesetOptions.credit && externals.pCreditSystem)
              ? std::optional<Credit>(externals.pCreditSystem->createCredit(
                    tilesetOptions.credit.value(),
                    tilesetOptions.showCreditsOnScreen))
              : std::nullopt),
      _tilesetCredits{},
      _overlayCollection{std::move(overlayCollection)},
      _tileLoadsInProgress{0},
      _loadedTilesCount{0},
      _tilesDataUsed{0},
      _destructionCompletePromise{externals.asyncSystem.createPromise<void>()},
      _destructionCompleteFuture{
          this->_destructionCompletePromise.getFuture().share()} {
  if (!url.empty()) {
    this->notifyTileStartLoading(nullptr);

    CesiumUtility::IntrusivePointer<TilesetVectorContentManager> thiz = this;

    externals.pAssetAccessor
        ->get(externals.asyncSystem, url, this->_requestHeaders)
        .thenInWorkerThread(
            [pLogger = externals.pLogger,
             asyncSystem = externals.asyncSystem,
             pAssetAccessor = externals.pAssetAccessor,
             contentOptions = tilesetOptions.contentOptions,
             showCreditsOnScreen = tilesetOptions.showCreditsOnScreen](
                const std::shared_ptr<CesiumAsync::IAssetRequest>&
                    pCompletedRequest) {
              // Check if request is successful
              const CesiumAsync::IAssetResponse* pResponse =
                  pCompletedRequest->response();
              const std::string& url = pCompletedRequest->url();
              if (!pResponse) {
                TilesetContentLoaderResult<TilesetContentLoader> result;
                result.errors.emplaceError(fmt::format(
                    "Did not receive a valid response for tileset {}",
                    url));
                return asyncSystem.createResolvedFuture(std::move(result));
              }

              uint16_t statusCode = pResponse->statusCode();
              if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
                TilesetContentLoaderResult<TilesetContentLoader> result;
                result.errors.emplaceError(fmt::format(
                    "Received status code {} for tileset {}",
                    statusCode,
                    url));
                return asyncSystem.createResolvedFuture(std::move(result));
              }

              // Parse Json response
              gsl::span<const std::byte> tilesetJsonBinary = pResponse->data();
              rapidjson::Document tilesetJson;
              tilesetJson.Parse(
                  reinterpret_cast<const char*>(tilesetJsonBinary.data()),
                  tilesetJsonBinary.size());
              if (tilesetJson.HasParseError()) {
                TilesetContentLoaderResult<TilesetContentLoader> result;
                result.errors.emplaceError(fmt::format(
                    "Error when parsing tileset JSON, error code {} at byte "
                    "offset {}",
                    tilesetJson.GetParseError(),
                    tilesetJson.GetErrorOffset()));
                return asyncSystem.createResolvedFuture(std::move(result));
              }

              // Check if the json is a tileset.json format or layer.json format
              // and create corresponding loader
              const auto rootIt = tilesetJson.FindMember("root");
              if (rootIt != tilesetJson.MemberEnd()) {
                TilesetContentLoaderResult<TilesetContentLoader> result =
                    TilesetJsonLoader::createLoader(pLogger, url, tilesetJson);
                return asyncSystem.createResolvedFuture(std::move(result));
              } else {
                const auto formatIt = tilesetJson.FindMember("format");
                bool isLayerJsonFormat = formatIt != tilesetJson.MemberEnd() &&
                                         formatIt->value.IsString();
                isLayerJsonFormat = isLayerJsonFormat &&
                                    std::string(formatIt->value.GetString()) ==
                                        "quantized-mesh-1.0";
                if (isLayerJsonFormat) {
                  const CesiumAsync::HttpHeaders& completedRequestHeaders =
                      pCompletedRequest->headers();
                  std::vector<CesiumAsync::IAssetAccessor::THeader> flatHeaders(
                      completedRequestHeaders.begin(),
                      completedRequestHeaders.end());
                  return LayerJsonTerrainLoader::createLoader(
                             asyncSystem,
                             pAssetAccessor,
                             contentOptions,
                             url,
                             flatHeaders,
                             showCreditsOnScreen,
                             tilesetJson)
                      .thenImmediately(
                          [](TilesetContentLoaderResult<TilesetContentLoader>&&
                                 result) { return std::move(result); });
                }

                TilesetContentLoaderResult<TilesetContentLoader> result;
                result.errors.emplaceError("tileset json has unsupport format");
                return asyncSystem.createResolvedFuture(std::move(result));
              }
            })
        .thenInMainThread(
            [thiz, errorCallback = tilesetOptions.loadErrorCallback](
                TilesetContentLoaderResult<TilesetContentLoader>&& result) {
              thiz->notifyTileDoneLoading(result.pRootTile.get());
              thiz->propagateTilesetContentLoaderResult(
                  TilesetLoadType::TilesetJson,
                  errorCallback,
                  std::move(result));
            })
        .catchInMainThread([thiz](std::exception&& e) {
          thiz->notifyTileDoneLoading(nullptr);
          SPDLOG_LOGGER_ERROR(
              thiz->_externals.pLogger,
              "An unexpected error occurs when loading tile: {}",
              e.what());
        });
  }
}

TilesetVectorContentManager::TilesetVectorContentManager(
    const TilesetExternals& externals,
    const TilesetOptions& tilesetOptions,
    VectorOverlayCollection&& overlayCollection,
    int64_t ionAssetID,
    const std::string& ionAccessToken,
    const std::string& ionAssetEndpointUrl)
    : _externals{externals},
      _requestHeaders{},
      _pLoader{},
      _pRootTile{},
      _userCredit(
          (tilesetOptions.credit && externals.pCreditSystem)
              ? std::optional<Credit>(externals.pCreditSystem->createCredit(
                    tilesetOptions.credit.value(),
                    tilesetOptions.showCreditsOnScreen))
              : std::nullopt),
      _tilesetCredits{},
      _overlayCollection{std::move(overlayCollection)},
      _tileLoadsInProgress{0},
      _loadedTilesCount{0},
      _tilesDataUsed{0},
      _destructionCompletePromise{externals.asyncSystem.createPromise<void>()},
      _destructionCompleteFuture{
          this->_destructionCompletePromise.getFuture().share()} {
  if (ionAssetID > 0) {
    auto authorizationChangeListener = [this](
                                           const std::string& header,
                                           const std::string& headerValue) {
      auto& requestHeaders = this->_requestHeaders;
      auto authIt = std::find_if(
          requestHeaders.begin(),
          requestHeaders.end(),
          [&header](auto& headerPair) { return headerPair.first == header; });
      if (authIt != requestHeaders.end()) {
        authIt->second = headerValue;
      } else {
        requestHeaders.emplace_back(header, headerValue);
      }
    };

    this->notifyTileStartLoading(nullptr);

    CesiumUtility::IntrusivePointer<TilesetVectorContentManager> thiz = this;

    CesiumIonTilesetLoader::createLoader(
        externals,
        tilesetOptions.contentOptions,
        static_cast<uint32_t>(ionAssetID),
        ionAccessToken,
        ionAssetEndpointUrl,
        authorizationChangeListener,
        tilesetOptions.showCreditsOnScreen)
        .thenInMainThread(
            [thiz, errorCallback = tilesetOptions.loadErrorCallback](
                TilesetContentLoaderResult<CesiumIonTilesetLoader>&& result) {
              thiz->notifyTileDoneLoading(result.pRootTile.get());
              thiz->propagateTilesetContentLoaderResult(
                  TilesetLoadType::CesiumIon,
                  errorCallback,
                  std::move(result));
            })
        .catchInMainThread([thiz](std::exception&& e) {
          thiz->notifyTileDoneLoading(nullptr);
          SPDLOG_LOGGER_ERROR(
              thiz->_externals.pLogger,
              "An unexpected error occurs when loading tile: {}",
              e.what());
        });
  }
}

CesiumAsync::SharedFuture<void>&
TilesetVectorContentManager::getAsyncDestructionCompleteEvent() {
  return this->_destructionCompleteFuture;
}

TilesetVectorContentManager::~TilesetVectorContentManager() noexcept {
  assert(this->_tileLoadsInProgress == 0);
  this->unloadAll();

  this->_destructionCompletePromise.resolve();
}

void TilesetVectorContentManager::loadTileContent(
    Tile& tile,
    const TilesetOptions& tilesetOptions) {
  CESIUM_TRACE("TilesetVectorContentManager::loadTileContent");

  if (tile.getState() == TileLoadState::Unloading) {
    // We can't load a tile that is unloading; it has to finish unloading first.
    return;
  }

  if (tile.getState() != TileLoadState::Unloaded &&
      tile.getState() != TileLoadState::FailedTemporarily) {
    // No need to load geometry, but give previously-throttled
    // Vector overlay tiles a chance to load.
    for (VectorMappedTo3DTile& VectorTile : tile.getMappedVectorTiles()) {
      VectorTile.loadThrottled();
    }

    return;
  }

  // Below are the guarantees the loader can assume about upsampled tile. If any
  // of those guarantees are wrong, it's a bug:
  // - Any tile that is marked as upsampled tile, we will guarantee that the
  // parent is always loaded. It lets the loader takes care of upsampling only
  // without requesting the parent tile. If a loader tries to upsample tile, but
  // the parent is not loaded, it is a bug.
  // - This manager will also guarantee that the parent tile will be alive until
  // the upsampled tile content returns to the main thread. So the loader can
  // capture the parent geometry by reference in the worker thread to upsample
  // the current tile. Warning: it's not thread-safe to modify the parent
  // geometry in the worker thread at the same time though
  const CesiumGeometry::UpsampledQuadtreeNode* pUpsampleID =
      std::get_if<CesiumGeometry::UpsampledQuadtreeNode>(&tile.getTileID());
  if (pUpsampleID) {
    // We can't upsample this tile until its parent tile is done loading.
    Tile* pParentTile = tile.getParent();
    if (pParentTile) {
      if (pParentTile->getState() != TileLoadState::Done) {
        loadTileContent(*pParentTile, tilesetOptions);
        return;
      }
    } else {
      // we cannot upsample this tile if it doesn't have parent
      return;
    }
  }

  // map Vector overlay to tile
  std::vector<CesiumGeospatial::Projection> projections =
      mapOverlaysToTile(tile, this->_overlayCollection, tilesetOptions);

  // begin loading tile
  notifyTileStartLoading(&tile);
  tile.setState(TileLoadState::ContentLoading);

  TileContentLoadInfo tileLoadInfo{
      this->_externals.asyncSystem,
      this->_externals.pAssetAccessor,
      this->_externals.pPrepareRendererResources,
      this->_externals.pLogger,
      tilesetOptions.contentOptions,
      tile};

  TilesetContentLoader* pLoader = this->_pLoader.get();
  
  TileLoadInput loadInput{
      tile,
      tilesetOptions.contentOptions,
      this->_externals.asyncSystem,
      this->_externals.pAssetAccessor,
      this->_externals.pLogger,
      this->_requestHeaders};

  // Keep the manager alive while the load is in progress.
  CesiumUtility::IntrusivePointer<TilesetVectorContentManager> thiz = this;

  pLoader->loadTileContent(loadInput)
      .thenImmediately([tileLoadInfo = std::move(tileLoadInfo),
                        projections = std::move(projections),
                        rendererOptions = tilesetOptions.rendererOptions](
                           TileLoadResult&& result) mutable {
        // the reason we run immediate continuation, instead of in the
        // worker thread, is that the loader may run the task in the main
        // thread. And most often than not, those main thread task is very
        // light weight. So when those tasks return, there is no need to
        // spawn another worker thread if the result of the task isn't
        // related to render content. We only ever spawn a new task in the
        // worker thread if the content is a render content
        if (result.state == TileLoadResultState::Success) {
          if (std::holds_alternative<CesiumGltf::Model>(result.contentKind)) {
            auto asyncSystem = tileLoadInfo.asyncSystem;
            return asyncSystem.runInWorkerThread(
                [result = std::move(result),
                 projections = std::move(projections),
                 tileLoadInfo = std::move(tileLoadInfo),
                 rendererOptions]() mutable {
                  return postProcessContentInWorkerThread(
                      std::move(result),
                      std::move(projections),
                      std::move(tileLoadInfo),
                      rendererOptions);
                });
          }
        }

        return tileLoadInfo.asyncSystem
            .createResolvedFuture<TileLoadResultAndRenderResources>(
                {std::move(result), nullptr});
      })
      .thenInMainThread([&tile, thiz](TileLoadResultAndRenderResources&& pair) {
        setTileContent(tile, std::move(pair.result), pair.pRenderResources);

        thiz->notifyTileDoneLoading(&tile);
      })
      .catchInMainThread([pLogger = this->_externals.pLogger, &tile, thiz](
                             std::exception&& e) {
        thiz->notifyTileDoneLoading(&tile);
        SPDLOG_LOGGER_ERROR(
            pLogger,
            "An unexpected error occurs when loading tile: {}",
            e.what());
      });
}

void TilesetVectorContentManager::updateTileContent(
    Tile& tile,
    double priority,
    const TilesetOptions& tilesetOptions) {
  if (tile.getState() == TileLoadState::Unloading) {
    unloadTileContent(tile);
  }

  if (tile.getState() == TileLoadState::ContentLoaded) {
    updateContentLoadedState(tile, priority, tilesetOptions);
  }

  if (tile.getState() == TileLoadState::Done) {
    updateDoneState(tile, tilesetOptions);
  }

  if (tile.shouldContentContinueUpdating()) {
    TileChildrenResult childrenResult =
        this->_pLoader->createTileChildren(tile);
    if (childrenResult.state == TileLoadResultState::Success) {
      tile.createChildTiles(std::move(childrenResult.children));
    }

    bool shouldTileContinueUpdated =
        childrenResult.state == TileLoadResultState::RetryLater;
    tile.setContentShouldContinueUpdating(shouldTileContinueUpdated);
  }
}

bool TilesetVectorContentManager::unloadTileContent(Tile& tile) {
  TileLoadState state = tile.getState();
  if (state == TileLoadState::Unloaded) {
    return true;
  }

  if (state == TileLoadState::ContentLoading) {
    return false;
  }

  TileContent& content = tile.getContent();

  // don't unload external or empty tile
  if (content.isExternalContent() || content.isEmptyContent()) {
    return false;
  }

  tile.getMappedVectorTiles().clear();

  // Unload the renderer resources and clear any Vector overlay tiles. We can do
  // this even if the tile can't be fully unloaded because this tile's geometry
  // is being using by an async upsample operation (checked below).
  switch (state) {
  case TileLoadState::ContentLoaded:
    unloadContentLoadedState(tile);
    break;
  case TileLoadState::Done:
    unloadDoneState(tile);
    break;
  default:
    break;
  }

  // Are any children currently being upsampled from this tile?
  for (const Tile& child : tile.getChildren()) {
    if (child.getState() == TileLoadState::ContentLoading &&
        std::holds_alternative<CesiumGeometry::UpsampledQuadtreeNode>(
            child.getTileID())) {
      // Yes, a child is upsampling from this tile, so it may be using the
      // tile's content from another thread via lambda capture. We can't unload
      // it right now. So mark the tile as in the process of unloading and stop
      // here.
      tile.setState(TileLoadState::Unloading);
      return false;
    }
  }

  // If we make it this far, the tile's content will be fully unloaded.
  notifyTileUnloading(&tile);
  content.setContentKind(TileUnknownContent{});
  tile.setState(TileLoadState::Unloaded);
  return true;
}

void TilesetVectorContentManager::unloadAll() {
  // TODO: use the linked-list of loaded tiles instead of walking the entire
  // tile tree.
  if (this->_pRootTile) {
    unloadTileRecursively(*this->_pRootTile, *this);
  }
}

void TilesetVectorContentManager::waitUntilIdle() {
  // Wait for all asynchronous loading to terminate.
  // If you're hanging here, it's most likely caused by _tileLoadsInProgress not
  // being decremented correctly when an async load ends.
  while (this->_tileLoadsInProgress > 0) {
    this->_externals.pAssetAccessor->tick();
    this->_externals.asyncSystem.dispatchMainThreadTasks();
  }

  // Wait for all overlays to wrap up their loading, too.
  uint32_t VectorOverlayTilesLoading = 1;
  while (VectorOverlayTilesLoading > 0) {
    this->_externals.pAssetAccessor->tick();
    this->_externals.asyncSystem.dispatchMainThreadTasks();

    VectorOverlayTilesLoading = 0;
    for (const auto& pTileProvider :
         this->_overlayCollection.getTileProviders()) {
      VectorOverlayTilesLoading += pTileProvider->getNumberOfTilesLoading();
    }
  }
}

const Tile* TilesetVectorContentManager::getRootTile() const noexcept {
  return this->_pRootTile.get();
}

Tile* TilesetVectorContentManager::getRootTile() noexcept {
  return this->_pRootTile.get();
}

const std::vector<CesiumAsync::IAssetAccessor::THeader>&
TilesetVectorContentManager::getRequestHeaders() const noexcept {
  return this->_requestHeaders;
}

std::vector<CesiumAsync::IAssetAccessor::THeader>&
TilesetVectorContentManager::getRequestHeaders() noexcept {
  return this->_requestHeaders;
}

const VectorOverlayCollection&
TilesetVectorContentManager::getVectorOverlayCollection() const noexcept {
  return this->_overlayCollection;
}

VectorOverlayCollection&
TilesetVectorContentManager::getVectorOverlayCollection() noexcept {
  return this->_overlayCollection;
}

const Credit* TilesetVectorContentManager::getUserCredit() const noexcept {
  if (this->_userCredit) {
    return &*this->_userCredit;
  }

  return nullptr;
}

const std::vector<Credit>&
TilesetVectorContentManager::getTilesetCredits() const noexcept {
  return this->_tilesetCredits;
}

int32_t TilesetVectorContentManager::getNumberOfTilesLoading() const noexcept {
  return this->_tileLoadsInProgress;
}

int32_t TilesetVectorContentManager::getNumberOfTilesLoaded() const noexcept {
  return this->_loadedTilesCount;
}

int64_t TilesetVectorContentManager::getTotalDataUsed() const noexcept {
  int64_t bytes = this->_tilesDataUsed;
  for (const auto& pTileProvider :
       this->_overlayCollection.getTileProviders()) {
    bytes += pTileProvider->getTileDataBytes();
  }

  return bytes;
}

bool TilesetVectorContentManager::tileNeedsWorkerThreadLoading(
    const Tile& tile) const noexcept {
  auto state = tile.getState();
  return state == TileLoadState::Unloaded ||
         state == TileLoadState::FailedTemporarily ||
         anyVectorOverlaysNeedLoading(tile);
}

bool TilesetVectorContentManager::tileNeedsMainThreadLoading(
    const Tile& tile) const noexcept {
  return tile.getState() == TileLoadState::ContentLoaded &&
         tile.isRenderContent();
}

void TilesetVectorContentManager::finishLoading(
    Tile& tile,
    const TilesetOptions& tilesetOptions) {
  assert(tile.getState() == TileLoadState::ContentLoaded);

  // Run the main thread part of loading.
  TileContent& content = tile.getContent();
  TileRenderContent* pRenderContent = content.getRenderContent();

  assert(pRenderContent != nullptr);

  // add copyright
  pRenderContent->setCredits(GltfUtilities::parseGltfCopyright(
      *this->_externals.pCreditSystem,
      pRenderContent->getModel(),
      tilesetOptions.showCreditsOnScreen));

  //void* pWorkerRenderResources = pRenderContent->getRenderResources();
  /*void* pMainThreadRenderResources =
      this->_externals.pRendererResourcesWorker->prepareInMainThread(
          tile,
          pWorkerRenderResources);*/

  //pRenderContent->setRenderResources(pMainThreadRenderResources);
  tile.setState(TileLoadState::Done);

  // This allows the Vector tile to be updated and children to be created, if
  // necessary.
  // Priority doesn't matter here since loading is complete.
  updateTileContent(tile, 0.0, tilesetOptions);
}

void TilesetVectorContentManager::setTileContent(
    Tile& tile,
    TileLoadResult&& result,
    void* pWorkerRenderResources) {
  if (result.state == TileLoadResultState::Failed) {
    tile.getMappedVectorTiles().clear();
    tile.setState(TileLoadState::Failed);
  } else if (result.state == TileLoadResultState::RetryLater) {
    tile.getMappedVectorTiles().clear();
    tile.setState(TileLoadState::FailedTemporarily);
  } else {
    // update tile if the result state is success
    if (result.updatedBoundingVolume) {
      tile.setBoundingVolume(*result.updatedBoundingVolume);
    }

    if (result.updatedContentBoundingVolume) {
      tile.setContentBoundingVolume(*result.updatedContentBoundingVolume);
    }

    auto& content = tile.getContent();
    std::visit(
        ContentKindSetter{
            content,
            std::move(result.rasterOverlayDetails),
            pWorkerRenderResources},
        std::move(result.contentKind));

    if (result.tileInitializer) {
      result.tileInitializer(tile);
    }

    tile.setState(TileLoadState::ContentLoaded);
  }
}

void TilesetVectorContentManager::updateContentLoadedState(
    Tile& tile,
    double /*priority*/,
    const TilesetOptions& tilesetOptions) {
  // initialize this tile content first
  TileContent& content = tile.getContent();
  if (content.isExternalContent()) {
    // if tile is external tileset, then it will be refined no matter what
    tile.setUnconditionallyRefine();
    tile.setState(TileLoadState::Done);
  } else if (content.isRenderContent()) {
    // If the main thread part of render content loading is not throttled,
    // do it right away. Otherwise we'll do it later in
    // Tileset::_processMainThreadLoadQueue with prioritization and throttling.
    if (tilesetOptions.mainThreadLoadingTimeLimit <= 0.0) {
      finishLoading(tile, tilesetOptions);
    }
  } else if (content.isEmptyContent()) {
    // There are two possible ways to handle a tile with no content:
    //
    // 1. Treat it as a placeholder used for more efficient culling, but
    //    never render it. Refining to this tile is equivalent to refining
    //    to its children.
    // 2. Treat it as an indication that nothing need be rendered in this
    //    area at this level-of-detail. In other words, "render" it as a
    //    hole. To have this behavior, the tile should _not_ have content at
    //    all.
    //
    // We distinguish whether the tileset creator wanted (1) or (2) by
    // comparing this tile's geometricError to the geometricError of its
    // parent tile. If this tile's error is greater than or equal to its
    // parent, treat it as (1). If it's less, treat it as (2).
    //
    // For a tile with no parent there's no difference between the
    // behaviors.
    double myGeometricError = tile.getNonZeroGeometricError();
    const Tile* pAncestor = tile.getParent();
    while (pAncestor && pAncestor->getUnconditionallyRefine()) {
      pAncestor = pAncestor->getParent();
    }

    double parentGeometricError = pAncestor
                                      ? pAncestor->getNonZeroGeometricError()
                                      : myGeometricError * 2.0;
    if (myGeometricError >= parentGeometricError) {
      tile.setUnconditionallyRefine();
    }

    tile.setState(TileLoadState::Done);
  }
}

void TilesetVectorContentManager::updateDoneState(Tile& tile, const TilesetOptions& tilesetOptions)
{
  // The reason for this method to terminate early when
  // Tile::shouldContentContinueUpdating() returns true is that: When a tile has
  // Tile::shouldContentContinueUpdating() to be true, it means the tile's
  // children need to be created by the
  // TilesetContentLoader::createTileChildren() which is invoked in the
  // TilesetVectorContentManager::updateTileContent() method. In the
  // updateDoneState(), VectorOverlayTiles that are mapped to the tile will
  // begin updating. If there are more VectorOverlayTiles with higher LOD and
  // the current tile is a leaf, more upsample children will be created for that
  // tile. So to accurately determine if a tile is a leaf, it needs the tile to
  // have no children and Tile::shouldContentContinueUpdating() to return false
  // which means the loader has no more children for this tile.
  if (tile.shouldContentContinueUpdating())
  {
    return;
  }

  // update Vector overlay
  TileContent& content = tile.getContent();
  const TileRenderContent* pRenderContent = content.getRenderContent();
  if (pRenderContent)
  {
    std::vector<VectorMappedTo3DTile>& VectorTiles =
        tile.getMappedVectorTiles();
    for (size_t i = 0; i < VectorTiles.size(); ++i) {
      VectorMappedTo3DTile& mappedVectorTile = VectorTiles[i];

      VectorOverlayTile* pLoadingTile = mappedVectorTile.getLoadingTile();
      if (pLoadingTile && pLoadingTile->getState() ==
                              VectorOverlayTile::LoadState::Placeholder) {
        VectorOverlayTileProvider* pProvider =
            this->_overlayCollection.findTileProviderForOverlay(
                pLoadingTile->getOverlay());
        VectorOverlayTileProvider* pPlaceholder =
            this->_overlayCollection.findPlaceholderTileProviderForOverlay(
                pLoadingTile->getOverlay());

        // Try to replace this placeholder with real tiles.

        if (pProvider && pPlaceholder && !pProvider->isPlaceholder()) {
          // Remove the existing placeholder mapping
          VectorTiles.erase(
              VectorTiles.begin() +
              static_cast<std::vector<VectorMappedTo3DTile>::difference_type>(
                  i));
          --i;

          // Add a new mapping.
          std::vector<CesiumGeospatial::Projection> missingProjections;
          VectorMappedTo3DTile::mapOverlayToTile(
              tilesetOptions.maximumScreenSpaceError,
              *pProvider,
              *pPlaceholder,
              tile,
              missingProjections);

          if (!missingProjections.empty()) {
            // The mesh doesn't have the right texture coordinates for this
            // overlay's projection, so we need to kick it back to the unloaded
            // state to fix that.
            // In the future, we could add the ability to add the required
            // texture coordinates without starting over from scratch.
            unloadTileContent(tile);
            return;
          }
        }

        continue;
      }
    }
  }
}

void TilesetVectorContentManager::unloadContentLoadedState(Tile& tile)
  {
  TileContent& content = tile.getContent();
  TileRenderContent* pRenderContent = content.getRenderContent();
  assert(pRenderContent && "Tile must have render content to be unloaded");

  void* pWorkerRenderResources = pRenderContent->getRenderResources();
  this->_externals.pPrepareRendererResources->free(
      tile,
      pWorkerRenderResources,
      nullptr);
  pRenderContent->setRenderResources(nullptr);
}

void TilesetVectorContentManager::unloadDoneState(Tile& tile)
  {
  TileContent& content = tile.getContent();
  TileRenderContent* pRenderContent = content.getRenderContent();
  assert(pRenderContent && "Tile must have render content to be unloaded");

  void* pMainThreadRenderResources = pRenderContent->getRenderResources();
  this->_externals.pPrepareRendererResources->free(
      tile,
      nullptr,
      pMainThreadRenderResources);
  pRenderContent->setRenderResources(nullptr);
}

void TilesetVectorContentManager::notifyTileStartLoading([[maybe_unused]] const Tile* pTile) noexcept
{
  ++this->_tileLoadsInProgress;
}

void TilesetVectorContentManager::notifyTileDoneLoading(const Tile* pTile) noexcept
  {
  assert(
      this->_tileLoadsInProgress > 0 &&
      "There are no tile loads currently in flight");
  --this->_tileLoadsInProgress;
  ++this->_loadedTilesCount;

  if (pTile)
  {
    this->_tilesDataUsed += pTile->computeByteSize();
  }
}

void TilesetVectorContentManager::notifyTileUnloading(const Tile* pTile) noexcept {
  if (pTile) {
    this->_tilesDataUsed -= pTile->computeByteSize();
  }

  --this->_loadedTilesCount;
}

template <class TilesetContentLoaderType>
void TilesetVectorContentManager::propagateTilesetContentLoaderResult(
    TilesetLoadType type,
    const std::function<void(const TilesetLoadFailureDetails&)>&
        loadErrorCallback,
    TilesetContentLoaderResult<TilesetContentLoaderType>&& result) {
  if (result.errors) {
    if (loadErrorCallback) {
      loadErrorCallback(TilesetLoadFailureDetails{
          nullptr,
          type,
          result.statusCode,
          CesiumUtility::joinToString(result.errors.errors, "\n- ")});
    } else {
      result.errors.logError(
          this->_externals.pLogger,
          "Errors when loading tileset");

      result.errors.logWarning(
          this->_externals.pLogger,
          "Warnings when loading tileset");
    }
  } else {
    this->_tilesetCredits.reserve(
        this->_tilesetCredits.size() + result.credits.size());
    for (const auto& creditResult : result.credits) {
      this->_tilesetCredits.emplace_back(_externals.pCreditSystem->createCredit(
          creditResult.creditText,
          creditResult.showOnScreen));
    }

    this->_requestHeaders = std::move(result.requestHeaders);
    this->_pLoader = std::move(result.pLoader);
    this->_pRootTile = std::move(result.pRootTile);
  }
}
} // namespace Cesium3DTilesSelection
