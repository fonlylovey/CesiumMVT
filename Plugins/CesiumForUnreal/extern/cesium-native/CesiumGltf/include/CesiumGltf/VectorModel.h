#pragma once

#include "CesiumGltf/Buffer.h"
#include "CesiumGltf/Library.h"
#include "CesiumUtility/ExtensibleObject.h"

#include <functional>

namespace CesiumGltf
{
enum class FeatureType {
  Point, LineString, Polygon, Spline
};

//һ��ʸ��Ҫ��
struct FeatureBuffer {
  /**
   * @brief An array of buffers.
   *
   * A buffer points to binary geometry
   */
  std::vector<CesiumGltf::Buffer> buffers;

  /**
   * @brief An array of bufferViews.
   *
   * A bufferView is a view into a buffer generally representing a subset of the
   * buffer.
   */
  std::vector<CesiumGltf::BufferView> bufferViews;

  /**
   * @brief ʸ�����ݵ�����
   *
   */
  FeatureType mvtType;
};

/** @copydoc VectorModel */
struct CESIUMGLTF_API VectorModel : public CesiumUtility::ExtensibleObject
{
  static inline constexpr const char* TypeName = "VectorModel";

  //һ����Ƭ�е�����Ҫ��
  std::vector<FeatureBuffer> features;

  std::vector<std::string> styles = {};
};
} // namespace CesiumGltf
