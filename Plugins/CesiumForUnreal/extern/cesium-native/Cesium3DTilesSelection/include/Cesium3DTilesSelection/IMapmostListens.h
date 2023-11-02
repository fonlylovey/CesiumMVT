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
  
        //loader�������Style Ԫ����֮�����
        virtual void finishLoadStyle(CesiumGltf::MapMetaData* styleData) = 0;
    };

} // namespace Cesium3DTilesSelection
