#include "Cesium3DTilesSelection/WebMapTileServiceVectorOverlay.h"

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

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {

class WMTSVectorTileProvider final
    : public QuadtreeVectorOverlayTileProvider {
public:
  WMTSVectorTileProvider(
      const IntrusivePointer<const VectorOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
      std::optional<Credit> credit,
      const std::shared_ptr<IRendererResourcesWorker>&
          pRendererResourcesWorker,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const CesiumGeospatial::Projection& projection,
      const CesiumGeometry::QuadtreeTilingScheme& tilingScheme,
      const CesiumGeometry::Rectangle& coverageRectangle,
      const std::string& url,
      const std::vector<IAssetAccessor::THeader>& headers,
      const std::string& version,
      const std::string& layers,
      const std::string& format,
      const std::string& style,
      const std::string& tileMatrixSet,
      uint32_t width,
      uint32_t height,
      uint32_t minimumLevel,
      uint32_t maximumLevel)
      : QuadtreeVectorOverlayTileProvider(
            pOwner,
            asyncSystem,
            pAssetAccessor,
            credit,
            pRendererResourcesWorker,
            pLogger,
            projection,
            tilingScheme,
            coverageRectangle,
            minimumLevel,
            maximumLevel,
            width,
            height),
        _url(url),
        _headers(headers),
        _version(version),
        _layers(layers),
        _format(format),
        _style(style),
        _tileMatrixSet(tileMatrixSet)
        {}

  virtual ~WMTSVectorTileProvider() {}

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
        "&TileCol={tileRow}&TileRow={tileCol}";

    const auto radiansToDegrees = [](double rad) {
      return std::to_string(CesiumUtility::Math::radiansToDegrees(rad));
    };
    
    double wmtsY = glm::pow(2, tileID.level) - 1.f - (double)tileID.y;

    const std::map<std::string, std::string> urlTemplateMap = {
        {"baseUrl", this->_url},
        {"version", this->_version},
        {"layer", this->_layers},
        {"style", this->_style},
        {"tileMatrix", this->_tileMatrixSet + ":" + std::to_string(tileID.level)},
        {"tileMatrixSet", this->_tileMatrixSet},
        {"tileRow", std::to_string(tileID.x)},
        {"tileCol", std::to_string((int)wmtsY)}};

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

WebMapTileServiceVectorOverlay::WebMapTileServiceVectorOverlay(
    const std::string& name,
    const std::string& url,
    const std::vector<IAssetAccessor::THeader>& headers,
    const WebMapTileServiceVectorOverlayOptions& wmsOptions,
    const VectorOverlayOptions& overlayOptions)
    : VectorOverlay(name, overlayOptions),
      _baseUrl(url),
      _headers(headers),
      _options(wmsOptions) {}

WebMapTileServiceVectorOverlay::~WebMapTileServiceVectorOverlay() {}

Future<VectorOverlay::CreateTileProviderResult>
WebMapTileServiceVectorOverlay::createTileProvider(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IRendererResourcesWorker>& pRendererResourcesWorker,
    const std::shared_ptr<spdlog::logger>& pLogger,
    CesiumUtility::IntrusivePointer<const VectorOverlay> pOwner) const {

  std::string xmlUrlGetcapabilities =
      CesiumUtility::Uri::substituteTemplateParameters(
          "{baseUrl}?Request=GetCapabilities&Version={version}&Service=WMTS",
          [this](const std::string& placeholder) {
            if (placeholder == "baseUrl") {
              return this->_baseUrl;
            } else if (placeholder == "version") {
              return Uri::escape(this->_options.version);
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
           pRendererResourcesWorker,
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

            std::string str(reinterpret_cast<const char*>(data.data()));
            tinyxml2::XMLDocument doc;
            const tinyxml2::XMLError error = doc.Parse(
                reinterpret_cast<const char*>(data.data()),
                data.size_bytes());

            if (error != tinyxml2::XMLError::XML_SUCCESS) {
              return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                  RasterOverlayLoadType::TileProvider,
                  std::move(pRequest),
                  "Could not parse web map tile service XML."});
            }

            tinyxml2::XMLElement* pRoot = doc.RootElement();
            if (!pRoot)
            {
              return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                  RasterOverlayLoadType::TileProvider,
                  std::move(pRequest),
                  "Web map tile service XML document does not have a root "
                  "element."});
            }

            tinyxml2::XMLElement* pContents = pRoot->FirstChildElement("Contents");
            if (!pContents)
            {
              return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                  RasterOverlayLoadType::TileProvider,
                  std::move(pRequest),
                  "Web map tile service XML document does not have a Contents "
                  "element."});
            }

            tinyxml2::XMLElement* pLayer = pContents->FirstChildElement("Layer");
            if (!pLayer)
            {
              return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                  RasterOverlayLoadType::TileProvider,
                  std::move(pRequest),
                  "Web map tile service XML document does not have a Layer "
                  "element."});
            }
            tinyxml2::XMLElement* pTitle = pLayer->FirstChildElement("ows:Title");
            std::string strLayerName = "";
            if (pTitle != nullptr)
            {
              strLayerName = pTitle->GetText();
            }

            const auto projection = CesiumGeospatial::GeographicProjection();

            CesiumGeospatial::GlobeRectangle tilingSchemeRectangle =
                CesiumGeospatial::GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;

            CesiumGeometry::Rectangle coverageRectangle =
                projectRectangleSimple(projection, tilingSchemeRectangle);

            const int rootTilesX = 2;
            const int rootTilesY = 1;
            CesiumGeometry::QuadtreeTilingScheme tilingScheme(
                coverageRectangle,
                rootTilesX,
                rootTilesY);

            WMTSVectorTileProvider* provider = new WMTSVectorTileProvider(
                pOwner,
                asyncSystem,
                pAssetAccessor,
                credit,
                pRendererResourcesWorker,
                pLogger,
                projection,
                tilingScheme,
                coverageRectangle,
                url,
                headers,
                options.version,
                strLayerName,
                options.format,
                options.style,
                options.tileMatrixSet,
                options.tileWidth < 1 ? 1 : uint32_t(options.tileWidth),
                options.tileHeight < 1 ? 1 : uint32_t(options.tileHeight),
                options.minimumLevel < 0 ? 0 : uint32_t(options.minimumLevel),
                options.maximumLevel < 0 ? 0 : uint32_t(options.maximumLevel));
            
            return provider;
          });
}

} // namespace Cesium3DTilesSelection
