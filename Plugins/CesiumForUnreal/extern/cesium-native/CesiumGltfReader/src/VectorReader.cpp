#include "CesiumGltfReader/VectorReader.h"

#include "ModelJsonHandler.h"
#include "decodeDataUrls.h"
#include "decodeDraco.h"
#include "registerExtensions.h"

#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGltf/ExtensionKhrTextureBasisu.h>
#include <CesiumGltf/ExtensionTextureWebp.h>
#include <CesiumJsonReader/ExtensionReaderContext.h>
#include <CesiumJsonReader/JsonHandler.h>
#include <CesiumJsonReader/JsonReader.h>
#include <CesiumUtility/Tracing.h>
#include <CesiumUtility/Uri.h>

#include <ktx.h>
#include <rapidjson/reader.h>
#include <webp/decode.h>
#include <mvt_utils.hpp>
#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>

using namespace CesiumAsync;
using namespace CesiumGltf;
using namespace CesiumGltfReader;
using namespace CesiumJsonReader;
using namespace CesiumUtility;

namespace
{
  VectorReaderResult readBinaryVector(
      const CesiumJsonReader::ExtensionReaderContext& context,
      const gsl::span<const std::byte>& data)
  {
    context;
    const char* charData = reinterpret_cast<const char*>(data.data());
    mvt_pbf::mvtpbf_reader mvtReader(charData,
        mvt_pbf::mvtpbf_reader::ePathType::eData);

    VectorReaderResult result;
    try
    {
      mvt_pbf::mvtpbf_reader::GeomVector geoms;
      mvtReader.getVectileData(geoms);
    }
    catch (...)
    {
      result.errors.push_back("Failed to process data!");
    }
   
    return result;
  }

  // ½âÂëÊý¾Ý
  void decodeData(GltfReaderResult& readVector) {
    readVector;
  }

} // namespace


VectorReader::VectorReader() : _context() {
    registerExtensions(this->_context);
}

  CesiumJsonReader::ExtensionReaderContext& VectorReader::getExtensions() {
    return this->_context;
}

const CesiumJsonReader::ExtensionReaderContext& VectorReader::getExtensions() const
{
    return this->_context;
}

VectorReaderResult VectorReader::readVector(
    const gsl::span<const std::byte>& data,
    const VectorReaderOptions& options) const
{
    const CesiumJsonReader::ExtensionReaderContext& context =
        this->getExtensions();
    VectorReaderResult result = readBinaryVector(context, data);
    if (options.decodeDraco) {
      //decodeData(result);
    }

    return result;
}

/*static*/
Future<VectorReaderResult> VectorReader::resolveExternalData(
    AsyncSystem asyncSystem,
    const std::string& baseUrl,
    const HttpHeaders& headers,
    std::shared_ptr<IAssetAccessor> pAssetAccessor,
    const VectorReaderOptions& options,
    VectorReaderResult&& result)
{

  options;
  // TODO: Can we avoid this copy conversion?
  std::vector<IAssetAccessor::THeader> tHeaders(headers.begin(), headers.end());

  if (!result.model) {
    return asyncSystem.createResolvedFuture(std::move(result));
  }

  // Get a rough count of how many external buffers we may have.
  // Some of these may be data uris though.
  size_t uriBuffersCount = 0;
  for (const Buffer& buffer : result.model->buffers) {
    if (buffer.uri) {
      ++uriBuffersCount;
    }
  }

  if (uriBuffersCount == 0) {
    return asyncSystem.createResolvedFuture(std::move(result));
  }

  auto pResult = std::make_unique<VectorReaderResult>(std::move(result));

  struct ExternalBufferLoadResult {
    bool success = false;
    std::string bufferUri;
  };

  std::vector<Future<ExternalBufferLoadResult>> resolvedBuffers;
  resolvedBuffers.reserve(uriBuffersCount);

  // We need to skip data uris.
  constexpr std::string_view dataPrefix = "data:";
  constexpr size_t dataPrefixLength = dataPrefix.size();

  for (Buffer& buffer : pResult->model->buffers) {
    if (buffer.uri && buffer.uri->substr(0, dataPrefixLength) != dataPrefix) {
      resolvedBuffers.push_back(
          pAssetAccessor
              ->get(asyncSystem, Uri::resolve(baseUrl, *buffer.uri), tHeaders)
              .thenInWorkerThread(
                  [pBuffer =
                       &buffer](std::shared_ptr<IAssetRequest>&& pRequest) {
                    const IAssetResponse* pResponse = pRequest->response();

                    std::string bufferUri = *pBuffer->uri;

                    if (pResponse) {
                      pBuffer->uri = std::nullopt;
                      pBuffer->cesium.data = std::vector<std::byte>(
                          pResponse->data().begin(),
                          pResponse->data().end());
                      return ExternalBufferLoadResult{true, bufferUri};
                    }

                    return ExternalBufferLoadResult{false, bufferUri};
                  }));
    }
  }
  return asyncSystem.all(std::move(resolvedBuffers))
      .thenInWorkerThread(
          [pResult = std::move(pResult)](
              std::vector<ExternalBufferLoadResult>&& loadResults) mutable {
            for (auto& bufferResult : loadResults) {
              if (!bufferResult.success) {
                pResult->warnings.push_back(
                    "Could not load the external gltf buffer: " +
                    bufferResult.bufferUri);
              }
            }
            return std::move(*pResult.release());
          });
}
