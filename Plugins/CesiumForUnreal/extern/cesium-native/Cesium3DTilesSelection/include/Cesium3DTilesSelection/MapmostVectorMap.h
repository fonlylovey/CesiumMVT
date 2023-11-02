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
    class IMapmostListens;

    //地图需要从UE传入的参数
    class CESIUM3DTILESSELECTION_API MapmostExternals final
    {
    public:
        std::shared_ptr<IMapmostListens> pMapListens;
        const CesiumAsync::AsyncSystem& asyncSystem;
        const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor;
    };
/**
 * @brief The loader interface to load the map style
 */
    class CESIUM3DTILESSELECTION_API MapmostVectorMap final 
    {
    public:
        MapmostVectorMap(const MapmostExternals& externals);
        ~MapmostVectorMap() = default;

        void CreateMap(const std::string& styleUrl);

        void AddSource(const CesiumGltf::MapSourceData& mapSource);

        void AddLayer(const CesiumGltf::MapLayerData& mapLayer);

        const MapmostExternals& getExternals() const noexcept
        {
            return this->_externals;
        }

    private:
        std::string _mapName;
        std::string _mapStyleUrl;
        MapmostExternals _externals;
        std::shared_ptr<VectorMapMetadataLoader> _pLoader;
    };

} // namespace Cesium3DTilesSelection
