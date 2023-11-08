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
    CesiumGltf::MapMetaData* resolvedStyleJson(const rapidjson::Document& jsonDoc)
    {
        CesiumGltf::MapMetaData* metaData = new CesiumGltf::MapMetaData;
        metaData->version = JsonHelpers::getInt32OrDefault(jsonDoc, "version", 8);
        metaData->name = JsonHelpers::getStringOrDefault(jsonDoc, "name", "");
        metaData->sprite = JsonHelpers::getStringOrDefault(jsonDoc, "sprite", "");
        metaData->glyphs = JsonHelpers::getStringOrDefault(jsonDoc, "glyphs", "");
        if(jsonDoc.HasMember("sources"))
        {
            const rapidjson::Value& sourcesObj = jsonDoc["sources"];
            auto iter = sourcesObj.MemberBegin();
            for (; iter != sourcesObj.MemberEnd(); iter++)
            {
                CesiumGltf::MapSourceData sourceData;
                sourceData.sourceName = iter->name.GetString();
                sourceData.setType(JsonHelpers::getStringOrDefault(iter->value, "type", ""));

                const rapidjson::Value& tilesArray = iter->value["tiles"];
                auto tileIter = tilesArray.Begin();
                for (; tileIter != tilesArray.End(); tileIter++)
                {
                    sourceData.url = tileIter->GetString();
                }
                metaData->sources.emplace_back(sourceData);
            }
        }
        if (jsonDoc.HasMember("layers"))
        {
            const rapidjson::Value& layersArray = jsonDoc["layers"];
            auto iter = layersArray.Begin();
            for (; iter != layersArray.End(); iter++)
            {
                CesiumGltf::MapLayerData layerData;
                layerData.id = JsonHelpers::getStringOrDefault(*iter, "id", "");
                std::string strType = JsonHelpers::getStringOrDefault(*iter, "type", "");
                layerData.setType(strType);
                layerData.source = JsonHelpers::getStringOrDefault(*iter, "source", "");
                layerData.sourceLayer = JsonHelpers::getStringOrDefault(*iter, "source-layer", "");

                const rapidjson::Value& paintObj = (*iter)["paint"];
                CesiumGltf::StyleData styleData;
                std::string strKey = strType + "-color";
                //auto styleItem = paintObj.FindMember(strKey.data());
                styleData.parseStringColor(JsonHelpers::getStringOrDefault(paintObj, strKey.data(), ""));
                strKey = strType + "-opacity";
                styleData.opacity = (float)JsonHelpers::getDoubleOrDefault(paintObj, strKey.data(), 1.0);
                layerData.style = std::move(styleData);

                auto theIter = iter->FindMember("layout");
                if(theIter != iter->MemberEnd())
                {
                    //
                    std::string strVisible = JsonHelpers::getStringOrDefault(theIter->value, "visibility", "none");
                    layerData.style.visibility = strVisible == "visible" ? true : false;
                }

                metaData->laysers.emplace_back(layerData);
            }
        }
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
    auto result =  _pAssetAccessor->get(_asyncSystem, url).thenInWorkerThread(
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
        std::string str(reinterpret_cast<const char*>(byteData.data()));
        str;
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
