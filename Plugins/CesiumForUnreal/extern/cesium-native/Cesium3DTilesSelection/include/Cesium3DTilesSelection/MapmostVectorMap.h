#pragma once

#include "BoundingVolume.h"
#include "Library.h"
#include "VectorMapMetadataLoader.h"

namespace CesiumAsync
{
    class AsyncSystem;
}


namespace Cesium3DTilesSelection 
{
/**
 * @brief The loader interface to load the map style
 */
    class CESIUM3DTILESSELECTION_API MapmostVectorMap final 
    {
    public:
        MapmostVectorMap();
        ~MapmostVectorMap() = default;

        void CreateMap(const std::string& styleUrl,
            const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
            const CesiumAsync::AsyncSystem& asyncSystem);

        void AddSource(const CesiumGltf::MapSourceData& mapSource);

        void AddLayer(const CesiumGltf::MapLayerData& mapLayer);


    private:
        std::string _mapName;
        std::string _mapStyleUrl;
        std::shared_ptr<VectorMapMetadataLoader> _pLoader;
    };

} // namespace Cesium3DTilesSelection
