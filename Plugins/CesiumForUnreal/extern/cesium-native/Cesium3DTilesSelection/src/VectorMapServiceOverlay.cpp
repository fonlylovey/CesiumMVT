#include "Cesium3DTilesSelection/VectorMapServiceOverlay.h"

#include "Cesium3DTilesSelection/CreditSystem.h"
#include "Cesium3DTilesSelection/QuadtreeVectorOverlayTileProvider.h"
#include "Cesium3DTilesSelection/RasterOverlayLoadFailureDetails.h"
#include "Cesium3DTilesSelection/VectorOverlayTile.h"
#include "Cesium3DTilesSelection/TilesetExternals.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"
#include "Cesium3DTilesSelection/tinyxml-cesium.h"

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGeospatial/WebMercatorProjection.h>
#include <CesiumUtility/Uri.h>
#include <CesiumUtility/Math.h>

#include <cstddef>
#include <sstream>
#include <iostream>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {

class VectorMapServiceProvider final
    : public QuadtreeVectorOverlayTileProvider {
public:
  VectorMapServiceProvider(
      const IntrusivePointer<const VectorOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
      std::optional<Credit> credit,
      const std::shared_ptr<IPrepareVectorMapResources>&
          pPrepareMapResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const CesiumGeospatial::Projection& projection,
      const CesiumGeometry::QuadtreeTilingScheme& tilingScheme,
      const CesiumGeometry::Rectangle& coverageRectangle,
      const std::string& url,
      const std::vector<IAssetAccessor::THeader>& headers, 
      uint32_t minimumLevel,
      uint32_t maximumLevel,
      uint32_t tileWidth,
      uint32_t tileHeight)
      : QuadtreeVectorOverlayTileProvider(
            pOwner,
            asyncSystem,
            pAssetAccessor,
            credit,
            pPrepareMapResources,
            pLogger,
            projection,
            tilingScheme,
            coverageRectangle,
            minimumLevel,
            maximumLevel,
            tileWidth,
            tileHeight), 
      _headers(headers),
      _url(url)
      {}

  virtual ~VectorMapServiceProvider() {}

protected:
  virtual CesiumAsync::Future<LoadedVectorOverlayData> loadQuadtreeTileData(
      const CesiumGeometry::QuadtreeTileID& tileID) const override {

    LoadVectorTileDataFromUrlOptions options;
    options.rectangle = this->getTilingScheme().tileToRectangle(tileID);
	
    const CesiumGeospatial::GlobeRectangle tileRectangle =
        CesiumGeospatial::unprojectRectangleSimple(
            this->getProjection(),
            options.rectangle);

    const std::string urlTemplate =
        this->_url +
        "?Request=GetTile&Service=WMTS&Version={version}"
        "&Layer={layer}&Style={style}&TileMatrix={tileMatrix}&TileMatrixSet={tileMatrixSet}"
        "&Format=application/vnd.mapbox-vector-tile"
        "&TileCol={tileCol}&TileRow={tileRow}";

    const auto radiansToDegrees = [](double rad) {
      return std::to_string(CesiumUtility::Math::radiansToDegrees(rad));
    };
    
    double wmtsY = glm::pow(2, tileID.level) - 1.f - (double)tileID.y;

	//瓦片坐标的X值是Col，Y值是Row
    const std::map<std::string, std::string> urlTemplateMap = {
        {"baseUrl", this->_url},
        {"version", this->_version},
        {"layer", this->_layers},
        {"style", this->_style},
        {"tileMatrix", this->_tileMatrixSet + ":" + std::to_string(tileID.level)},
        {"tileMatrixSet", this->_tileMatrixSet},
        {"tileRow", std::to_string((int)wmtsY)},
        {"tileCol", std::to_string(tileID.x)}};

    options.level = tileID.level;
    options.Row = (int)wmtsY;
    options.Col = tileID.x;

    std::string url = CesiumUtility::Uri::substituteTemplateParameters(
        urlTemplate,
        [&map = urlTemplateMap](const std::string& placeholder) {
          auto it = map.find(placeholder);
          return it == map.end() ? "{" + placeholder + "}"
                                 : Uri::escape(it->second);
        });
  
    return this->loadTileDataFromUrl(url, this->_headers, std::move(options));
  }

private:
  std::string _url;
  std::string _version;
  std::string _layers;
  std::string _format;
  std::string _tileMatrixSet;
  std::string _style;
  std::vector<IAssetAccessor::THeader> _headers;
};

VectorMapServiceOverlay::VectorMapServiceOverlay(
    const std::string& name,
    const std::string& url,
    const std::vector<IAssetAccessor::THeader>& headers,
    const VectorMapOverlayOptions& xyzOptions,
    const VectorOverlayOptions& overlayOptions)
    : VectorOverlay(name, overlayOptions),
      _baseUrl(url),
      _headers(headers),
      _options(xyzOptions) {}

VectorMapServiceOverlay::~VectorMapServiceOverlay() {}

Future<VectorOverlay::CreateTileProviderResult>
VectorMapServiceOverlay::createTileProvider(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareVectorMapResources>& pPrepareMapResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    CesiumUtility::IntrusivePointer<const VectorOverlay> pOwner) const {

  std::string xmlUrlGetcapabilities =
      CesiumUtility::Uri::substituteTemplateParameters(
          "{baseUrl}?Request=GetCapabilities&Version={version}&Service=WMTS",
          [this](const std::string& placeholder) {
            if (placeholder == "baseUrl") {
              return this->_baseUrl;
            }
            // Keep other placeholders
            return "{" + placeholder + "}";
          });

  pOwner = pOwner ? pOwner : this;

  const std::optional<Credit> credit =
      this->_options.credit ? std::make_optional(pCreditSystem->createCredit(
                                  this->_options.credit.value()))
                            : std::nullopt;

  return pAssetAccessor->get(asyncSystem, xmlUrlGetcapabilities, this->_headers)
      .thenInMainThread(
          [pOwner,
           asyncSystem,
           pAssetAccessor,
           credit,
           pPrepareMapResources,
           pLogger,
           options = this->_options,
           url = this->_baseUrl,
           headers =
               this->_headers](const std::shared_ptr<IAssetRequest>& pRequest)
              -> CreateTileProviderResult {
            const IAssetResponse* pResponse = pRequest->response();
            if (!pResponse) {
              return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                  RasterOverlayLoadType::TileProvider,
                  std::move(pRequest),
                  "No response received from web map tile service."});
            }

            const gsl::span<const std::byte> data = pResponse->data();
            if(pResponse->statusCode() != 200) 
            {
              return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                  RasterOverlayLoadType::TileProvider,
                  std::move(pRequest),
                  "No response received from web map tile service."});
            }
            std::string str(reinterpret_cast<const char*>(data.data()));
            
            CesiumGeospatial::GlobeRectangle tilingSchemeRectangle =
                CesiumGeospatial::GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;
            CesiumGeospatial::Projection projection;
            tilingSchemeRectangle =
                CesiumGeospatial::GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;
            CesiumGeometry::Rectangle coverageRectangle =
                projectRectangleSimple(projection, tilingSchemeRectangle);
            CesiumGeometry::QuadtreeTilingScheme tilingScheme(
                projectRectangleSimple(projection, tilingSchemeRectangle),
                0,
                1);
            VectorMapServiceProvider* provider = new VectorMapServiceProvider(
                pOwner,
                asyncSystem,
                pAssetAccessor,
                credit,
                pPrepareMapResources,
                pLogger, 
                projection,
                tilingScheme,
                coverageRectangle,
                url,
                headers,
                0,
                0,
                0,
                0);
            return provider;
          });
}

} // namespace Cesium3DTilesSelection
