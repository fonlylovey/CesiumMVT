#pragma once

#include "CesiumGltf/Buffer.h"
#include "CesiumGltf/Library.h"
#include "CesiumUtility/ExtensibleObject.h"

#include <functional>

namespace CesiumGltf {

/** @copydoc VectorModel */
struct CESIUMGLTF_API VectorModel : public CesiumUtility::ExtensibleObject
{
  static inline constexpr const char* TypeName = "Model";

  std::vector<CesiumGltf::Buffer> buffers;
  std::vector<std::string> styles = {};
};
} // namespace CesiumGltf
