#pragma once

#include "BoundingVolume.h"
#include "Library.h"
#include "RasterOverlayDetails.h"
#include "TileContent.h"
#include "TileLoadResult.h"
#include "TilesetOptions.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGltf/VectorModel.h>

#include <spdlog/logger.h>

#include <functional>
#include <memory>
#include <optional>
#include <vector>
#include "nonstd/expected.hpp"


namespace CesiumAsync
{
    class AsyncSystem;
}


namespace Cesium3DTilesSelection 
{

    using LoadedResult = nonstd::expected<
    CesiumGltf::MapStyleData*,
    CesiumGltf::FailureInfos>;
/**
 * @brief The loader interface to load the map style
 */
    class CESIUM3DTILESSELECTION_API VectorMapMetadataLoader final 
    {
    public:
      VectorMapMetadataLoader(
          const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
          const CesiumAsync::AsyncSystem& asyncSystem);

        /**
        * @brief Default virtual destructor
        */
        ~VectorMapMetadataLoader() = default;

        CesiumAsync::Future<LoadedResult> loadStyleData(const std::string& url);

    private:
        

    private:
        CesiumAsync::AsyncSystem _asyncSystem;
        std::shared_ptr<CesiumAsync::IAssetAccessor> _pAssetAccessor;
    };

} // namespace Cesium3DTilesSelection
