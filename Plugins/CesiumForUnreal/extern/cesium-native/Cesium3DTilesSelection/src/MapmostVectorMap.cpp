#include "Cesium3DTilesSelection/MapmostVectorMap.h"

#include "Cesium3DTilesSelection/CreditSystem.h"
#include "Cesium3DTilesSelection/QuadtreeVectorOverlayTileProvider.h"
#include "Cesium3DTilesSelection/RasterOverlayLoadFailureDetails.h"
#include "Cesium3DTilesSelection/VectorOverlayTile.h"
#include "Cesium3DTilesSelection/TilesetExternals.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"
#include "Cesium3DTilesSelection/tinyxml-cesium.h"
#include "Cesium3DTilesSelection/IMapmostListens.h"

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGeospatial/WebMercatorProjection.h>
#include <CesiumUtility/Uri.h>
#include <CesiumUtility/Math.h>

#include <cstddef>
#include <sstream>
#include <iostream>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection 
{
    MapmostVectorMap::MapmostVectorMap(const MapmostExternals& externals) 
    : _externals(externals)
    {
        _pLoader = nullptr;
    }

    void MapmostVectorMap::loadMetaStyle(const std::string& styleUrl) 
    {
        if(_pLoader == nullptr)
        {
            _pLoader = std::make_shared<VectorMapMetadataLoader>(_externals.pAssetAccessor, _externals.asyncSystem);
        }
        _pLoader->loadStyleData(styleUrl)
            .thenInWorkerThread
            (
                [](LoadedResult&& result)
                {
                    CesiumGltf::MapMetaData* pStyleData = std::move(*result);
                    return pStyleData;
                }
            )
            .thenInMainThread
            (
                [this](CesiumGltf::MapMetaData* pStyleData)
                {
                    this->_externals.pMapListens->finishLoadStyle(pStyleData);
                }
            );

        
    }

// namespace Cesium3DTilesSelection
} 
