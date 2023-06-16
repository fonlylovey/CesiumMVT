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

//一个矢量要素
struct VectorFeature
{

  //要素坐标集合
  std::vector<glm::ivec3> points;

  // 矢量要素的类型
  FeatureType mvtType = FeatureType::Point;

  //要素ID
  int featureID = -1;

  //要素个数
  size_t featureCount = 0;

  int ringType = -1;
};

// 一个矢量图层
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
  //一个瓦片中的所有图层
  std::vector<VectorLayer> layers {};

  std::vector<std::string> styles = {};
};
} // namespace CesiumGltf
