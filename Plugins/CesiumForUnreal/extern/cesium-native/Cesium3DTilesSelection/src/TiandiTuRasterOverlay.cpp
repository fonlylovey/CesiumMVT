#include "Cesium3DTilesSelection/TiandiTuRasterOverlay.h"

#include "Cesium3DTilesSelection/CreditSystem.h"
#include "Cesium3DTilesSelection/QuadtreeRasterOverlayTileProvider.h"
#include "Cesium3DTilesSelection/RasterOverlayLoadFailureDetails.h"
#include "Cesium3DTilesSelection/RasterOverlayTile.h"
#include "Cesium3DTilesSelection/TilesetExternals.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"
#include "Cesium3DTilesSelection/tinyxml-cesium.h"

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/WebMercatorProjection.h>
#include <CesiumUtility/Uri.h>

#include <cstddef>

using namespace CesiumAsync;
using namespace CesiumUtility;



namespace Cesium3DTilesSelection {

namespace {
struct TiandiTuTileset {
  std::string tileMatrix;
  uint32_t level;
};
void string_replace(
    std::string& url,
    std::string& placeHolder,
    std::string& replaceString) {
  std::string::size_type position = 0;
  std::string::size_type placeHolderSize = placeHolder.size();
  std::string::size_type replaceStringSize = replaceString.size();
  position = url.find(placeHolder);
  if (position != std::string::npos) {
    url.erase(position, placeHolderSize);
    url.insert(position, replaceString);
    position += replaceStringSize;
  }
}
} // namespace

class TiandiTuTileProvider final : public QuadtreeRasterOverlayTileProvider {
public:
  TiandiTuTileProvider(const IntrusivePointer<const RasterOverlay>& pOwner,
                       const CesiumAsync::AsyncSystem& asyncSystem,
                       const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
                       std::optional<Credit> credit,
                       const std::shared_ptr<IPrepareRendererResources>&
                           pPrepareRendererResources,
                       const std::shared_ptr<spdlog::logger>& pLogger,
                       const CesiumGeospatial::Projection& projection,
                       const CesiumGeometry::QuadtreeTilingScheme& tilingScheme,
                       const CesiumGeometry::Rectangle& coverageRectangle,
                       const std::vector<IAssetAccessor::THeader>& headers,
                       uint32_t width,
                       uint32_t height,
                       uint32_t minimumLevel,
                       uint32_t maximumLevel,
                       std::string mapStyle,
                       std::string token,
    std::vector<uint32_t> tileSetLevel): QuadtreeRasterOverlayTileProvider(
            pOwner,
            asyncSystem,
            pAssetAccessor,
            credit,
            pPrepareRendererResources,
            pLogger,
            projection,
            tilingScheme,
            coverageRectangle,
            minimumLevel,
            maximumLevel,
            width,
            height),
        _headers(headers),
        _MapStyle(mapStyle),
        _token(token),
    _tileSetLevel(tileSetLevel) {}

  virtual ~TiandiTuTileProvider() {}

protected:
  virtual CesiumAsync::Future<LoadedRasterOverlayImage> loadQuadtreeTileImage(
      const CesiumGeometry::QuadtreeTileID& tileID) const override {

    LoadTileImageFromUrlOptions options;
    CesiumGeometry::Rectangle rectangle =
        this->getTilingScheme().tileToRectangle(tileID);
    options.rectangle = rectangle;
    options.moreDetailAvailable = tileID.level < this->getMaximumLevel();

    uint32_t level = tileID.level;
    if (level < _tileSetLevel.size()) {

        uint32_t tile_Y = pow(2, tileID.level) - 1;
        tile_Y -= tileID.y;
       int num =1+ rand() % 7;
        std::string url =
            "https://t" + std::to_string(num) + ".tianditu.gov.cn/" +
            this->_MapStyle +"_w/wmts?tk="+this->_token+"&layer=" + this->_MapStyle +
            "&style=default&tilematrixset=w&Service=WMTS&Request=GetTile&Version=1.0.0&Format=tiles"+
           "&TileMatrix=" +std::to_string(level) +
           "&TileCol=" + std::to_string(tileID.x) +
           "&TileRow=" + std::to_string(tile_Y);

      return this->loadTileImageFromUrl(
          url,
          this->_headers,
          std::move(options));
    } else {
      return this->getAsyncSystem()
          .createResolvedFuture<LoadedRasterOverlayImage>(
              {std::nullopt,
               options.rectangle,
               {},
               {"Failed to load image from TianDiTu Tiles."},
               {},
               options.moreDetailAvailable});
    }
  }

private:
  std::string _MapStyle;
  std::vector<IAssetAccessor::THeader> _headers;
  std::vector<uint32_t> _tileSetLevel;
  std::string _token;
};

TiandiTuRasterOverlay::TiandiTuRasterOverlay(
    const std::string& name,
    const std::vector<IAssetAccessor::THeader>& headers,
    const TiandiTuRasterOverlayOptions& TiandiTuOptions,
    const RasterOverlayOptions& overlayOptions)
    : RasterOverlay(name, overlayOptions),
      _headers(headers),
      _options(TiandiTuOptions) {}

TiandiTuRasterOverlay::~TiandiTuRasterOverlay() {}

// namespace

Future<RasterOverlay::CreateTileProviderResult>
TiandiTuRasterOverlay::createTileProvider(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareRendererResources>& pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner) const {

  pOwner = pOwner ? pOwner : this;

  const std::optional<Credit> credit =
      this->_options.credit ? std::make_optional(pCreditSystem->createCredit(
                                  this->_options.credit.value(),
                                  pOwner->getOptions().showCreditsOnScreen))
                            : std::nullopt;

  return asyncSystem.runInMainThread(
      [pOwner,
       asyncSystem,
       pAssetAccessor,
       credit,
       pPrepareRendererResources,
       pLogger,
       options = this->_options,
       headers = this->_headers]() -> CreateTileProviderResult {
        uint32_t tileWidth = options.tileWidth.value_or(256);
        uint32_t tileHeight = options.tileHeight.value_or(256);
        uint32_t minimumLevel = 0;
        uint32_t maximumLevel = 18;
        std::vector<uint32_t> tileSetLevel;
        for (int i = 1; i < 19; i++) {

          tileSetLevel.emplace_back(i);
        }
        CesiumGeospatial::Projection projection= CesiumGeospatial::WebMercatorProjection();
          CesiumGeospatial::GlobeRectangle tilingSchemeRectangle =
              CesiumGeospatial::WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE;

        CesiumGeometry::Rectangle coverageRectangle =
            projectRectangleSimple(projection, tilingSchemeRectangle);

        CesiumGeometry::QuadtreeTilingScheme tilingScheme(
            projectRectangleSimple(projection, tilingSchemeRectangle),
            1,
            1);
        return new TiandiTuTileProvider(
            pOwner,
            asyncSystem,
            pAssetAccessor,
            credit,
            pPrepareRendererResources,
            pLogger,
            projection,
            tilingScheme,
            coverageRectangle,
            headers,
            tileWidth,
            tileHeight,
            minimumLevel,
            maximumLevel,
            options.MapStyle.value(),
            options.token.value(),
          tileSetLevel);
      });
}
} // namespace Cesium3DTilesSelection
