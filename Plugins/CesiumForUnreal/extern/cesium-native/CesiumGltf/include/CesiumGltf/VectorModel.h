#pragma once

#include "CesiumGltf/Buffer.h"
#include "CesiumGltf/Library.h"
#include "CesiumUtility/ExtensibleObject.h"

#include <functional>

namespace CesiumGltf {

/** @copydoc ModelSpec */
struct CESIUMGLTF_API VectorModel : public CesiumUtility::ExtensibleObject {
  static inline constexpr const char* TypeName = "Model";

   std::vector<CesiumGltf::Buffer> buffers;

};
} // namespace CesiumGltf
