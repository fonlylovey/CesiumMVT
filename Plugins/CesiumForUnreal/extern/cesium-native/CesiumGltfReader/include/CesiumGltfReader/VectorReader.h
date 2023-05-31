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
  std::optional<CesiumGltf::VectorModel> model;

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
   * @brief Whether data URLs in buffers and images should be automatically
   * decoded as part of the load process.
   */
  bool decodeDataUrls = true;

  /**
   * @brief Whether data URLs should be cleared after they are successfully
   * decoded.
   *
   * This reduces the memory usage of the model.
   */
  bool clearDecodedDataUrls = true;

  /**
   * @brief Whether embedded images in {@link Model::buffers} should be
   * automatically decoded as part of the load process.
   *
   * The {@link ImageSpec::mimeType} property is ignored, and instead the
   * [stb_image](https://github.com/nothings/stb) library is used to decode
   * images in `JPG`, `PNG`, `TGA`, `BMP`, `PSD`, `GIF`, `HDR`, or `PIC` format.
   */
  bool decodeEmbeddedImages = true;

  /**
   * @brief Whether geometry compressed using the `KHR_draco_mesh_compression`
   * extension should be automatically decoded as part of the load process.
   */
  bool decodeDraco = true;

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
      const gsl::span<const std::byte>& data,
      const VectorReaderOptions& options = VectorReaderOptions()) const;

  /**
   * @brief Accepts the result of {@link readGltf} and resolves any remaining
   * external buffers and images.
   *
   * @param asyncSystem The async system to use for resolving external data.
   * @param baseUrl The base url that all the external uris are relative to.
   * @param headers The http headers needed to make any external data requests.
   * @param pAssetAccessor The asset accessor to use to request the external
   * buffers and images.
   * @param options Options for how to read the glTF.
   * @param result The result of the synchronous readGltf invocation.
   */
  static CesiumAsync::Future<VectorReaderResult> resolveExternalData(
      CesiumAsync::AsyncSystem asyncSystem,
      const std::string& baseUrl,
      const CesiumAsync::HttpHeaders& headers,
      std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor,
      const VectorReaderOptions& options,
      VectorReaderResult&& result);

private:
  CesiumJsonReader::ExtensionReaderContext _context;
};

} // namespace CesiumGltfReader
