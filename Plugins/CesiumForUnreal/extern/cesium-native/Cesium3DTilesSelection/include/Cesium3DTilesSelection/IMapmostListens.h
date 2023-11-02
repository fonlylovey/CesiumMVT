#pragma once

#include "Library.h"


namespace CesiumGltf 
{
struct MapMetaData;
} // namespace CesiumGltf

namespace Cesium3DTilesSelection 
{

    class CESIUM3DTILESSELECTION_API IMapmostListens
    {
    public:
        virtual ~IMapmostListens() = default;
  
        //loader解析完成Style 元数据之后调用
        virtual void finishLoadStyle(CesiumGltf::MapMetaData* styleData) = 0;
    };

} // namespace Cesium3DTilesSelection
