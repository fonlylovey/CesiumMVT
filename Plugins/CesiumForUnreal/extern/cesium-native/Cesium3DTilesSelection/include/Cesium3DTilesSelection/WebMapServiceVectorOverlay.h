#pragma once

#include "Library.h"
#include "VectorOverlay.h"

#include <CesiumAsync/IAssetRequest.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>

#include <functional>
#include <memory>

namespace Cesium3DTilesSelection {

class CreditSystem;

/**
 * @brief Options for Web Map Service (WMS) overlays.
 */
struct WebMapServiceVectorOverlayOptions {

  /**
   * @brief The Web Map Service version. The default is "1.3.0".
   */
  std::string version = "1.3.0";

  /**
   * @brief Comma separated Web Map Service layer names to request.
   */
  std::string layers;

  /**
   * @brief The image format to request, expressed as a MIME type to be given to
   * the server. The default is "application/vnd.mapbox-vector-tile".
   */
  std::string format = "application/vnd.mapbox-vector-tile";

  /**
   * @brief A credit for the data source, which is displayed on the canvas.
   */
  std::optional<std::string> credit;

  /**
   * @brief The minimum level-of-detail supported by the imagery provider.
   *
   * Take care when specifying this that the number of tiles at the minimum
   * level is small, such as four or less. A larger number is likely to
   * result in rendering problems.
   */
  int32_t minimumLevel = 0;

  /**
   * @brief The maximum level-of-detail supported by the imagery provider.
   */
  int32_t maximumLevel = 14;

  /**
   * @brief Pixel width of image tiles.
   */
  int32_t tileWidth = 256;

  /**
   * @brief Pixel height of image tiles.
   */
  int32_t tileHeight = 256;

  std::string tileMatrixSet = "EPSG:4326";

  std::string style = "EPSG:4326";
};

/**
 * @brief A {@link VectorOverlay} accessing images from a Web Map Service (WMS) server.
 */
class CESIUM3DTILESSELECTION_API WebMapServiceVectorOverlay final
    : public VectorOverlay {
public:
  /**
   * @brief Creates a new instance.
   *
   * @param name The user-given name of this overlay layer.
   * @param url The base URL.
   * @param headers The headers. This is a list of pairs of strings of the
   * form (Key,Value) that will be inserted as request headers internally.
   * @param wmsOptions The {@link WebMapServiceVectorOverlayOptions}.
   * @param overlayOptions The {@link VectorOverlayOptions} for this instance.
   */
  WebMapServiceVectorOverlay(
      const std::string& name,
      const std::string& url,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers = {},
      const WebMapServiceVectorOverlayOptions& wmsOptions = {},
      const VectorOverlayOptions& overlayOptions = {});
  virtual ~WebMapServiceVectorOverlay() override;

  virtual CesiumAsync::Future<CreateTileProviderResult> createTileProvider(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CreditSystem>& pCreditSystem,
      const std::shared_ptr<IPrepareVectorMapResources>&
          pPrepareMapResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      CesiumUtility::IntrusivePointer<const VectorOverlay> pOwner)
      const override;

private:
  std::string _baseUrl;
  std::vector<CesiumAsync::IAssetAccessor::THeader> _headers;
  WebMapServiceVectorOverlayOptions _options;
};

} // namespace Cesium3DTilesSelection
