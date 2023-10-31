#include <Cesium3DTilesSelection/VectorMapMetadataLoader.h>
#include "Cesium3DTilesSelection/CreditSystem.h"

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>

#include <rapidjson/document.h>
#include <CesiumUtility/JsonHelpers.h>

using namespace CesiumUtility;
using namespace CesiumAsync;
using namespace Cesium3DTilesSelection;

namespace
{
    CesiumGltf::MapStyleData* resolvedStyleJson(const rapidjson::Document& jsonDoc) 
    {
      CesiumGltf::MapStyleData* metaData = new CesiumGltf::MapStyleData;
      int32_t vers = JsonHelpers::getInt32OrDefault(jsonDoc, "version", 8);
      metaData->version = vers;
      return metaData;
    }
}

Cesium3DTilesSelection::VectorMapMetadataLoader::VectorMapMetadataLoader(
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const AsyncSystem& asyncSystem) 
    :_asyncSystem(asyncSystem),
    _pAssetAccessor(pAssetAccessor)
{
}

Future<LoadedResult>
Cesium3DTilesSelection::VectorMapMetadataLoader::loadStyleData(const std::string& url) 
{
    auto result = _pAssetAccessor->get(_asyncSystem, url).thenInWorkerThread(
    [](const std::shared_ptr<IAssetRequest>& pRequest) -> LoadedResult
    {
        LoadedResult result;
            CesiumGltf::FailureInfos faildResult;
        const CesiumAsync::IAssetResponse* pResponse = pRequest->response();
        std::string styleUrl = pRequest->url();
        if(pResponse == nullptr)
        {
            faildResult.pRequest = nullptr;
            faildResult.message = fmt::format("url:{}. bad request!", styleUrl);
            return nonstd::make_unexpected(faildResult);
        } 
        else 
        {
            int stateCode = pResponse->statusCode();
            if (stateCode != 0 && (stateCode < 200 || stateCode >= 300)) 
            {
              faildResult.pRequest = nullptr;
              faildResult.message = fmt::format(
                  "url:{}. faild request, Received status code {}!",
                  styleUrl,
                  stateCode);
              return nonstd::make_unexpected(faildResult);
            }
        }

        gsl::span<const std::byte> byteData = pResponse->data();
        rapidjson::Document jsonDoc;
        jsonDoc.Parse(
            reinterpret_cast<const char*>(byteData.data()),
            byteData.size());
        if (jsonDoc.HasParseError()) {
            faildResult.pRequest = nullptr;
            faildResult.message = fmt::format(
                "url:{}. Parse byte data to json faild: {}!",
                jsonDoc.GetParseError());
            return nonstd::make_unexpected(faildResult);
        }
        return resolvedStyleJson(jsonDoc);
    });
    return result;
}
