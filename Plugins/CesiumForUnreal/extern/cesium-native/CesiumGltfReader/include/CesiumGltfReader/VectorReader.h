#pragma once

#include "CesiumGltfReader/Library.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/HttpHeaders.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGltf/VectorModel.h>
#include <CesiumJsonReader/ExtensionReaderContext.h>
#include <CesiumJsonReader/IExtensionJsonHandler.h>

#include <gsl/span>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace CesiumGltfReader {

/**
 * @brief The result of reading a glTF model with
 * {@link GltfReader::readGltf}.
 */
struct CESIUMGLTFREADER_API VectorReaderResult {
  /**
   * @brief The read model, or std::nullopt if the model could not be read.
   */
  CesiumGltf::VectorTile* model;

  /**
   * @brief Errors, if any, that occurred during the load process.
   */
  std::vector<std::string> errors;

  /**
   * @brief Warnings, if any, that occurred during the load process.
   */
  std::vector<std::string> warnings;
};

/**
 * @brief Options for how to read a glTF.
 */
struct CESIUMGLTFREADER_API VectorReaderOptions {
  /**
   * @brief Whether geometry compressed using the `KHR_draco_mesh_compression`
   * extension should be automatically decoded as part of the load process.
   */
  bool decodeDraco = false;

};

/**
 * @brief Reads glTF models and images.
 */
class CESIUMGLTFREADER_API VectorReader {
public:
  /**
   * @brief Constructs a new instance.
   */
  VectorReader();

  /**
   * @brief Gets the context used to control how extensions are loaded from glTF
   * files.
   */
  CesiumJsonReader::ExtensionReaderContext& getExtensions();

  /**
   * @brief Gets the context used to control how extensions are loaded from glTF
   * files.
   */
  const CesiumJsonReader::ExtensionReaderContext& getExtensions() const;

  /**
   * @brief Reads a vector or binary pbf/mvt from a buffer.
   *
   * @param data The buffer from which to read the vector.
   * @param options Options for how to read the vector.
   * @return The result of reading the vector.
   */
  VectorReaderResult readVector(
      const std::string& data,
      const VectorReaderOptions& options = VectorReaderOptions()) const;

private:
  CesiumJsonReader::ExtensionReaderContext _context;
};

} // namespace CesiumGltfReader
