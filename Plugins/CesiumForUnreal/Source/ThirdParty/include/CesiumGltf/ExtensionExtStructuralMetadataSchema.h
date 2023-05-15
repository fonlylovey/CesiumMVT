// This file was generated by generate-classes.
// DO NOT EDIT THIS FILE!
#pragma once

#include "CesiumGltf/ExtensionExtStructuralMetadataClass.h"
#include "CesiumGltf/ExtensionExtStructuralMetadataEnum.h"
#include "CesiumGltf/Library.h"

#include <CesiumUtility/ExtensibleObject.h>

#include <optional>
#include <string>
#include <unordered_map>

namespace CesiumGltf {
/**
 * @brief An object defining classes and enums.
 */
struct CESIUMGLTF_API ExtensionExtStructuralMetadataSchema final
    : public CesiumUtility::ExtensibleObject {
  static inline constexpr const char* TypeName =
      "ExtensionExtStructuralMetadataSchema";

  /**
   * @brief Unique identifier for the schema. Schema IDs must be alphanumeric
   * identifiers matching the regular expression `^[a-zA-Z_][a-zA-Z0-9_]*$`.
   */
  std::string id;

  /**
   * @brief The name of the schema, e.g. for display purposes.
   */
  std::optional<std::string> name;

  /**
   * @brief The description of the schema.
   */
  std::optional<std::string> description;

  /**
   * @brief Application-specific version of the schema.
   */
  std::optional<std::string> version;

  /**
   * @brief A dictionary, where each key is a class ID and each value is an
   * object defining the class. Class IDs must be alphanumeric identifiers
   * matching the regular expression `^[a-zA-Z_][a-zA-Z0-9_]*$`.
   */
  std::unordered_map<
      std::string,
      CesiumGltf::ExtensionExtStructuralMetadataClass>
      classes;

  /**
   * @brief A dictionary, where each key is an enum ID and each value is an
   * object defining the values for the enum. Enum IDs must be alphanumeric
   * identifiers matching the regular expression `^[a-zA-Z_][a-zA-Z0-9_]*$`.
   */
  std::
      unordered_map<std::string, CesiumGltf::ExtensionExtStructuralMetadataEnum>
          enums;
};
} // namespace CesiumGltf
