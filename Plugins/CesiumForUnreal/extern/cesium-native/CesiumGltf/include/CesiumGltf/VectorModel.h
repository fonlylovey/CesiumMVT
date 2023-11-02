#pragma once

#include "CesiumGltf/Buffer.h"
#include "CesiumGltf/BufferView.h"
#include "CesiumGltf/Library.h"
#include "CesiumUtility/ExtensibleObject.h"
#include <functional>
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include <memory>
/*
矢量瓦片标准 https://github.com/jingsam/vector-tile-spec/blob/master/2.1/README_zh.md#42-%E8%A6%81%E7%B4%A0
4.1. 图层
none
4.2. 要素
每个要素必须包含一个geometry字段。

每个要素必须包含一个type字段，该字段将在几何类型章节描述（4.3.4）。

每个要素可以包含一个tags字段。如果存在属于要素级别的元数据，应该存储到tags字段中。

每个要素可以包含一个id字段。如果一个要素包含一个id字段，那么id字段的值应该相对于图层中的其他要素是唯一的。

4.3.4.4. Polygon几何类型
POLYGON几何类型表示面或多面几何，每个面有且只有一个外环和零个或多个内环。面几何的指令序列包含一个或多个下列序列：
一个ExteriorRing
零个或多个InteriorRing

*/

namespace CesiumAsync
{
    class IAssetRequest;
}

namespace CesiumGltf
{
    class FailureInfos 
    {
    public:

      std::shared_ptr<CesiumAsync::IAssetRequest> pRequest = nullptr;

      std::string message = "";
    };

    enum class FeatureType { UNKNOWN, Point, LineString, Polygon };

    enum class RingType { Outer, Inner, Invalid };

    struct VectorGeometry 
    {
      // 要素坐标集合
      std::vector<glm::ivec3> points;

      // 多边形环的类型 outer = 0,inner = 1, invalid = 2
      RingType ringType = RingType::Invalid;
    };

    //一个矢量要素
    struct VectorFeature
    {

	    std::vector<VectorGeometry> geometry;

	    // 矢量要素的类型
	    FeatureType featureType = FeatureType::Point;

	    //要素ID
	    size_t featureID = 0;

	    //要素个数
	    size_t featureCount = 0;

	
    };

    // 一个矢量图层
    struct VectorLayer
    {
	    uint32_t version = 0;

	    std::string name = "";

	    //瓦片的大小
	    uint32_t extent = 4096;

	    std::vector<VectorFeature> features;
    };


    struct VectorStyle
    {
        //是否填充多边形
        bool isFill = true;

        //填充多边形的颜色
        glm::dvec4 fillColor = glm::dvec4(0, 0, 0, 0);

        //
        bool isOutline = false;

        float lineWidth = 5;

        // 填充多边形的颜色
        glm::dvec4 outlineColor = glm::dvec4(0, 0, 0, 0);
    };


    /** @copydoc VectorModel */
    struct CESIUMGLTF_API VectorModel
    {
      char* TypeName = "VectorModel";
      ~VectorModel()
      {
	      layers.clear();
      }
      //一个瓦片中的所有图层
      std::vector<VectorLayer> layers {};

      VectorStyle style;

      // 层级
      int level = 0;

      // 行号
      int row = 0;

      // 列号
      int col = 0;

      //瓦片范围的左下角 经纬度
      glm::dvec2 extentMin = glm::dvec2();

      //瓦片范围的右上角 经纬度
      glm::dvec2 extentMax = glm::dvec2();

    };


/************************************************************************/
/* 矢量瓦片地图相关定义                                                  */
/************************************************************************/

    enum class SoureType
    {
        Vector,
        Raster,
        RasterDEM,
        Geojson,
        Image,
        Video
    };

    enum class LayerType 
    { 
        Background,
        Circle,
        Line,
        Fill,
        Symbol,
        Raster,
        FillExtrusion,
        Heatmap,
        Hillshade
    };

    struct StyleData
    {
        glm::ivec4 color = glm::ivec4(255, 255, 0, 255);
        float opacity = 1.0;
        bool visibility = true;
        void parseStringColor(std::string strColor);
    };

    struct MapSourceData 
    {
        std::string sourceName = "";
        SoureType type = SoureType::Vector;
        std::string url = "";

        void setType(const std::string& strType);
    };

    struct MapLayerData
    {
        std::string id = "";
        std::string source = "";
        std::string sourceLayer = "";
        LayerType type = LayerType::Background;
        StyleData style;
        void setType(const std::string& strType);
    };

    struct MapMetaData 
    {
        int version = 8;
        std::string name = "";
        std::string sprite = "";
        std::string glyphs = "";
        std::vector<MapSourceData> sources;
        std::vector<MapLayerData> laysers;
    };

} // namespace CesiumGltf
