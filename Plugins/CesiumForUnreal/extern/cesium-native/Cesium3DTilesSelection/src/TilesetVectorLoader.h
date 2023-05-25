#pragma once

#include "TilesetContentLoaderResult.h"
#include "Cesium3DTilesSelection/VectorTileLoadResult.h"
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <Cesium3DTilesSelection/VectorOverlayCollection.h>
#include <rapidjson/fwd.h>

#include <memory>
#include <string>
#include <vector>

namespace Cesium3DTilesSelection {

struct VectorContentKindSetter {
  void operator()(TileUnknownContent content) {
    vectorContent.setContentKind(content);
  }

  void operator()(TileEmptyContent content) {
    vectorContent.setContentKind(content);
  }

  void operator()(TileExternalContent content) {
    vectorContent.setContentKind(content);
  }

  void operator()(CesiumGltf::VectorModel&& model) {
    auto pRenderContent =
        std::make_unique<VectorTileRenderContent>(std::move(model));
    pRenderContent->setRenderResources(pRenderResources);
    if (vectorOverlayDetails)
    {
      pRenderContent->setVectorOverlayDetails(std::move(*vectorOverlayDetails));
    }

    vectorContent.setContentKind(std::move(pRenderContent));
  }

  std::optional<VectorOverlayDetails> vectorOverlayDetails;
  void* pRenderResources;
  VectorTileContent& vectorContent;
};

class TilesetVectorLoader
{
public:
  TilesetVectorLoader() = default;
  TilesetVectorLoader(const std::string& baseUrl);

  CesiumAsync::Future<VectorTileLoadResult>
  loadTileContent(const TileLoadInput& loadInput);

  TileChildrenResult createTileChildren(const Tile& tile);

  const std::string& getBaseUrl() const noexcept;

  void addChildLoader(std::unique_ptr<TilesetVectorLoader> pLoader);

  static CesiumAsync::Future<TilesetContentLoaderResult<TilesetVectorLoader>>
  createLoader(
      const TilesetExternals& externals,
      const std::string& tilesetJsonUrl,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders);

  static TilesetContentLoaderResult<TilesetVectorLoader> createLoader(
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::string& tilesetJsonUrl);

  //将矢量瓦片和地形瓦片关联到一起
  std::vector<CesiumGeospatial::Projection> mapOverlaysToTile(
      Tile& tile,
      VectorOverlayCollection& overlays,
      const TilesetOptions& tilesetOptions);

private:
  std::string _baseUrl;

  std::vector<std::unique_ptr<TilesetVectorLoader>> _children;
};
} // namespace Cesium3DTilesSelection
