#include "Cesium3DTilesSelection/MapmostVectorMap.h"

#include "Cesium3DTilesSelection/CreditSystem.h"
#include "Cesium3DTilesSelection/QuadtreeVectorOverlayTileProvider.h"
#include "Cesium3DTilesSelection/RasterOverlayLoadFailureDetails.h"
#include "Cesium3DTilesSelection/VectorOverlayTile.h"
#include "Cesium3DTilesSelection/TilesetExternals.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"
#include "Cesium3DTilesSelection/tinyxml-cesium.h"

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
    MapmostVectorMap::MapmostVectorMap() 
    {
        _pLoader = nullptr;
    }

    void MapmostVectorMap::CreateMap(
        const std::string& styleUrl,
        const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
        const CesiumAsync::AsyncSystem& asyncSystem) 
    {
        if(_pLoader == nullptr)
        {
            _pLoader = std::make_shared<VectorMapMetadataLoader>(pAssetAccessor, asyncSystem);
        }
        _pLoader->loadStyleData(styleUrl).thenInWorkerThread(
            [this](LoadedResult&& result) 
            {
                CesiumGltf::MapStyleData* pStyleData = std::move(*result);
                for(auto& source : pStyleData->sources) 
                {
                    this->AddSource(source);
                }
            });

        
    }


    void MapmostVectorMap::AddSource(const CesiumGltf::MapSourceData& mapSource)
    {
        mapSource;
    }

    void MapmostVectorMap::AddLayer(const CesiumGltf::MapLayerData& mapLayer)
    {
        mapLayer;
    }

// namespace Cesium3DTilesSelection
} 
