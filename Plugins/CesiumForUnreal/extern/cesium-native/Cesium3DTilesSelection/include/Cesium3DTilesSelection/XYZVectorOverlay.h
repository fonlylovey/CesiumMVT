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
 * @brief Options for XYZ overlays.
 */
struct XYZVectorOverlayOptions {
  std::optional<std::string> credit;

  int32_t minimumLevel = 0;

  int32_t maximumLevel = 20;

  std::string MatrixSet = "EPSG:4326";

  uint32_t tileWidth = 256;

  uint32_t tileHeight = 256;

  bool decode = false;

  std::string sourceName = "";

};

/**
 * @brief A {@link VectorOverlay} accessing vector from a XYZ server.
 */
class CESIUM3DTILESSELECTION_API XYZVectorOverlay final
    : public VectorOverlay {
public:
  /**
   * @brief Creates a new instance.
   *
   * @param name The user-given name of this overlay layer.
   * @param url The base URL.
   * @param headers The headers. This is a list of pairs of strings of the
   * form (Key,Value) that will be inserted as request headers internally.
   * @param wmsOptions The {@link VectorOverlayOptions}.
   * @param overlayOptions The {@link VectorOverlayOptions} for this instance.
   */
  XYZVectorOverlay(
      const std::string& name,
      const std::string& url,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers = {},
      const XYZVectorOverlayOptions& xyzOptions = {},
      const VectorOverlayOptions& overlayOptions = {});
  virtual ~XYZVectorOverlay() override;

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
  XYZVectorOverlayOptions _options;
};

} // namespace Cesium3DTilesSelection
