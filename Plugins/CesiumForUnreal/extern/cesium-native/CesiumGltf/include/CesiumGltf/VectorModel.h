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
ʸ����Ƭ��׼ https://github.com/jingsam/vector-tile-spec/blob/master/2.1/README_zh.md#42-%E8%A6%81%E7%B4%A0
4.1. ͼ��
none
4.2. Ҫ��
ÿ��Ҫ�ر������һ��geometry�ֶΡ�

ÿ��Ҫ�ر������һ��type�ֶΣ����ֶν��ڼ��������½�������4.3.4����

ÿ��Ҫ�ؿ��԰���һ��tags�ֶΡ������������Ҫ�ؼ����Ԫ���ݣ�Ӧ�ô洢��tags�ֶ��С�

ÿ��Ҫ�ؿ��԰���һ��id�ֶΡ����һ��Ҫ�ذ���һ��id�ֶΣ���ôid�ֶε�ֵӦ�������ͼ���е�����Ҫ����Ψһ�ġ�

4.3.4.4. Polygon��������
POLYGON�������ͱ�ʾ�����漸�Σ�ÿ��������ֻ��һ���⻷����������ڻ����漸�ε�ָ�����а���һ�������������У�
һ��ExteriorRing
�������InteriorRing

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
      // Ҫ�����꼯��
      std::vector<glm::ivec3> points;

      // ����λ������� outer = 0,inner = 1, invalid = 2
      RingType ringType = RingType::Invalid;
    };

    //һ��ʸ��Ҫ��
    struct VectorFeature
    {

	    std::vector<VectorGeometry> geometry;

	    // ʸ��Ҫ�ص�����
	    FeatureType featureType = FeatureType::Point;

	    //Ҫ��ID
	    size_t featureID = 0;

	    //Ҫ�ظ���
	    size_t featureCount = 0;

	
    };

    // һ��ʸ��ͼ��
    struct VectorLayer
    {
	    uint32_t version = 0;

	    std::string name = "";

	    //��Ƭ�Ĵ�С
	    uint32_t extent = 4096;

	    std::vector<VectorFeature> features;
    };


    struct VectorStyle
    {
        //�Ƿ��������
        bool isFill = true;

        //������ε���ɫ
        glm::dvec4 fillColor = glm::dvec4(0, 0, 0, 0);

        //
        bool isOutline = false;

        float lineWidth = 5;

        // ������ε���ɫ
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
      //һ����Ƭ�е�����ͼ��
      std::vector<VectorLayer> layers {};

      VectorStyle style;

      // �㼶
      int level = 0;

      // �к�
      int row = 0;

      // �к�
      int col = 0;

      //��Ƭ��Χ�����½� ��γ��
      glm::dvec2 extentMin = glm::dvec2();

      //��Ƭ��Χ�����Ͻ� ��γ��
      glm::dvec2 extentMax = glm::dvec2();

    };


/************************************************************************/
/* ʸ����Ƭ��ͼ��ض���                                                  */
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
