#include "Cesium3DTilesSelection/XYZVectorOverlay.h"

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

class XYZVectorTileProvider final
    : public QuadtreeVectorOverlayTileProvider {
public:
  XYZVectorTileProvider(
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
      uint32_t tileHeight,
      bool isDecode,
      const std::string& sourceName
      )
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
        _isDecode(isDecode),
        _sourceName(sourceName),
        _url(url),
        _headers(headers)
        {}

  virtual ~XYZVectorTileProvider() {}

protected:
  virtual CesiumAsync::Future<LoadedVectorOverlayData> loadQuadtreeTileData(
      const CesiumGeometry::QuadtreeTileID& tileID) const override {

    LoadVectorTileDataFromUrlOptions options;
    options.rectangle = this->getTilingScheme().tileToRectangle(tileID);
	
    const CesiumGeospatial::GlobeRectangle tileRectangle =
        CesiumGeospatial::unprojectRectangleSimple(
            this->getProjection(),
            options.rectangle);
    
    double wmtsY = glm::pow(2, tileID.level) - 1.f - (double)tileID.y;
    wmtsY;
    options.level = tileID.level;
    options.Row = tileID.y;
    options.Col = tileID.x;
    options.isDecode = _isDecode;
    options.sourceName = _sourceName;
	//瓦片坐标的X值是Col，Y值是Row
    const std::map<std::string, std::string> urlTemplateMap = {
        {"x", std::to_string(options.Col)},
        {"y", std::to_string(options.Row)},
        {"z", std::to_string(tileID.level)} };

    std::string url = CesiumUtility::Uri::substituteTemplateParameters(
        this->_url,
        [&map = urlTemplateMap](const std::string& placeholder)
        {
            auto it = map.find(placeholder);
            return it == map.end() ? "{" + placeholder + "}"
                : Uri::escape(it->second);
        });
    return this->loadTileDataFromUrl(url, this->_headers, std::move(options));
  }

private:
  bool _isDecode = false;
  std::string _sourceName = "";
  std::string _url;
  std::vector<IAssetAccessor::THeader> _headers;
};

XYZVectorOverlay::XYZVectorOverlay(
    const std::string& name,
    const std::string& url,
    const std::vector<IAssetAccessor::THeader>& headers,
    const XYZVectorOverlayOptions& xyzOptions,
    const VectorOverlayOptions& overlayOptions)
    : VectorOverlay(name, overlayOptions),
      _baseUrl(url),
      _headers(headers),
      _options(xyzOptions) {}

XYZVectorOverlay::~XYZVectorOverlay() {}

Future<VectorOverlay::CreateTileProviderResult>
XYZVectorOverlay::createTileProvider(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareVectorMapResources>& pPrepareMapResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    CesiumUtility::IntrusivePointer<const VectorOverlay> pOwner) const
 {
  pOwner = pOwner ? pOwner : this;

  const std::optional<Credit> credit =
      this->_options.credit ? std::make_optional(pCreditSystem->createCredit(
                                  this->_options.credit.value()))
                            : std::nullopt;

  return asyncSystem.runInMainThread(
          [pOwner,
           asyncSystem,
           pAssetAccessor,
           credit,
           pPrepareMapResources,
           pLogger,
           options = this->_options,
           url = this->_baseUrl,
           headers = this->_headers]() -> CreateTileProviderResult {

			//0级的时候初始的列有几个瓦片
			int StartColCount = 2;
			CesiumGeospatial::Projection projection;
            CesiumGeospatial::GlobeRectangle tilingSchemeRectangle(0.0, 0.0, 0.0, 0.0);
			if (options.MatrixSet == "EPSG:4326") 
			{
				projection = CesiumGeospatial::GeographicProjection();
				tilingSchemeRectangle = CesiumGeospatial::GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;
				StartColCount = 2;
            } 
			else if (options.MatrixSet == "EPSG:3857"  || options.MatrixSet == "EPSG:900913") 
			{
                projection = CesiumGeospatial::WebMercatorProjection();
                tilingSchemeRectangle = CesiumGeospatial::WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE;
                StartColCount = 1;
            } 

            CesiumGeometry::Rectangle coverageRectangle = projectRectangleSimple(projection, tilingSchemeRectangle);

            const int rootTilesX = StartColCount;
            const int rootTilesY = 1;
            CesiumGeometry::QuadtreeTilingScheme tilingScheme(
                coverageRectangle,
                rootTilesX,
                rootTilesY);

            XYZVectorTileProvider* provider = new XYZVectorTileProvider(
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
                options.minimumLevel,
                options.maximumLevel,
                options.tileWidth,
                options.tileHeight,
                options.decode,
                options.sourceName);
            return provider;
          });
}

} // namespace Cesium3DTilesSelection
