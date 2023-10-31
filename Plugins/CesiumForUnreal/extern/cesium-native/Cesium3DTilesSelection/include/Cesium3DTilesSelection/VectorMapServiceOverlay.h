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
struct VectorMapOverlayOptions {
 /**
   * @brief A credit for the data source, which is displayed on the canvas.
   */
  std::optional<std::string> credit;
  /**
   * @brief The {@link CesiumGeometry::Rectangle}, in radians, covered by the
   * image.
   */
  std::optional<CesiumGeometry::Rectangle> coverageRectangle;

  /**
   * @brief The {@link CesiumGeospatial::Projection} that is used.
   */
  std::optional<CesiumGeospatial::Projection> projection;

  /**
   * @brief The {@link CesiumGeometry::QuadtreeTilingScheme} specifying how
   * the ellipsoidal surface is broken into tiles.
   */
  std::optional<CesiumGeometry::QuadtreeTilingScheme> tilingScheme;

  /**
   * @brief The {@link CesiumGeospatial::Ellipsoid}.
   *
   * If the `tilingScheme` is specified, this parameter is ignored and
   * the tiling scheme's ellipsoid is used instead. If neither parameter
   * is specified, the {@link CesiumGeospatial::Ellipsoid::WGS84} is used.
   */
  std::optional<CesiumGeospatial::Ellipsoid> ellipsoid;
};

/**
 * @brief A {@link VectorOverlay} accessing vector from a XYZ server.
 */
class CESIUM3DTILESSELECTION_API VectorMapServiceOverlay final
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
  VectorMapServiceOverlay(
      const std::string& name,
      const std::string& url,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers = {},
      const VectorMapOverlayOptions& xyzOptions = {},
      const VectorOverlayOptions& overlayOptions = {});
  virtual ~VectorMapServiceOverlay() override;

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
  VectorMapOverlayOptions _options;
};

} // namespace Cesium3DTilesSelection
