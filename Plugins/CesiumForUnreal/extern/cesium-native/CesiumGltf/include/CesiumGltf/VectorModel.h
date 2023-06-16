#pragma once

#include "CesiumGltf/Buffer.h"
#include "CesiumGltf/BufferView.h"
#include "CesiumGltf/Library.h"
#include "CesiumUtility/ExtensibleObject.h"
#include "glm/vec3.hpp"
#include <functional>

namespace CesiumGltf
{
enum class FeatureType {
  Point, LineString, Polygon, Spline
};

//һ��ʸ��Ҫ��
struct VectorFeature
{

  //Ҫ�����꼯��
  std::vector<glm::ivec3> points;

  // ʸ��Ҫ�ص�����
  FeatureType mvtType = FeatureType::Point;

  //Ҫ��ID
  int featureID = -1;

  //Ҫ�ظ���
  size_t featureCount = 0;

  int ringType = -1;
};

// һ��ʸ��ͼ��
struct VectorLayer
{
  std::vector<VectorFeature> features;
  int layerID = -1;
};

/** @copydoc VectorModel */
struct CESIUMGLTF_API VectorModel
{
  static inline constexpr const char* TypeName = "VectorModel";
  ~VectorModel()
  {
	  layers.clear();
	  styles.clear();
  }
  //һ����Ƭ�е�����ͼ��
  std::vector<VectorLayer> layers {};

  std::vector<std::string> styles = {};
};
} // namespace CesiumGltf
