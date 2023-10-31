#pragma once

#include "Library.h"
#include "RasterOverlayLoadFailureDetails.h"

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCountedNonThreadSafe.h>

#include <nonstd/expected.hpp>
#include <spdlog/fwd.h>

#include <any>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace Cesium3DTilesSelection {

struct Credit;
class CreditSystem;
class IPrepareVectorMapResources;
class VectorOverlayTileProvider;
class VectorOverlayCollection;

using VectorOverlayLoadFailureDetails = RasterOverlayLoadFailureDetails;
using VectorOverlayLoadType = RasterOverlayLoadType;

/**
 * @brief Options for loading Vector overlays.
 */
struct CESIUM3DTILESSELECTION_API VectorOverlayOptions {
  /**
   * @brief The maximum number of overlay tiles that may simultaneously be in
   * the process of loading.
   */
  int32_t maximumSimultaneousTileLoads = 20;

  /**
   * @brief The maximum number of bytes to use to cache sub-tiles in memory.
   *
   * This is used by provider types, such as
   * {@link QuadtreeVectorOverlayTileProvider}, that have an underlying tiling
   * scheme that may not align with the tiling scheme of the geometry tiles on
   * which the Vector overlay tiles are draped. Because a single sub-tile may
   * overlap multiple geometry tiles, it is useful to cache loaded sub-tiles
   * in memory in case they're needed again soon. This property controls the
   * maximum size of that cache.
   */
  int64_t subTileCacheBytes = 16 * 1024 * 1024;

  /**
   * @brief The maximum pixel size of Vector overlay textures, in either
   * direction.
   *
   * Images created by this overlay will be no more than this number of pixels
   * in either direction. This may result in reduced Vector overlay detail in
   * some cases. For example, in a {@link QuadtreeVectorOverlayTileProvider},
   * this property will limit the number of quadtree tiles that may be mapped to
   * a given geometry tile. The selected quadtree level for a geometry tile is
   * reduced in order to stay under this limit.
   */
  int32_t maximumTextureSize = 2048;

  /**
   * @brief The maximum number of pixels of error when rendering this overlay.
   * This is used to select an appropriate level-of-detail.
   *
   * When this property has its default value, 2.0, it means that Vector overlay
   * images will be sized so that, when zoomed in closest, a single pixel in
   * the Vector overlay maps to approximately 2x2 pixels on the screen.
   */
  double maximumScreenSpaceError = 2.0;

  /**
   * @brief A callback function that is invoked when a Vector overlay resource
   * fails to load.
   *
   * Vector overlay resources include a Cesium ion asset endpoint or any
   * resources required for Vector overlay metadata.
   *
   * This callback is invoked by the {@link VectorOverlayCollection} when an
   * error occurs while it is creating a tile provider for this VectorOverlay.
   * It is always invoked in the main thread.
   */
  std::function<void(const RasterOverlayLoadFailureDetails&)> loadErrorCallback;

  /**
   * @brief Whether or not to display the credits on screen.
   */
  bool showCreditsOnScreen = false;

  /**
   * @brief Arbitrary data that will be passed to {@link prepareVectorInLoadThread},
   * for example, data to control the per-Vector overlay client-specific texture
   * properties.
   *
   * This object is copied and given to background texture preparation threads,
   * so it must be inexpensive to copy.
   */
  std::any rendererOptions;
};

/**
 * @brief The base class for a Vectorized image that can be draped
 * over a {@link Tileset}. The image may be very, very high resolution, so only
 * small pieces of it are mapped to the Tileset at a time.
 *
 * Instances of this class can be added to the {@link VectorOverlayCollection}
 * that is returned by {@link Tileset::getOverlays}.
 *
 * Instances of this class must be allocated on the heap, and their lifetimes
 * must be managed with {@link CesiumUtility::IntrusivePointer}.
 *
 * @see BingMapsVectorOverlay
 * @see IonVectorOverlay
 * @see TileMapServiceVectorOverlay
 * @see WebMapServiceVectorOverlay
 */
class VectorOverlay
    : public CesiumUtility::ReferenceCountedNonThreadSafe<VectorOverlay> {
public:
  /**
   * @brief Creates a new instance.
   *
   * @param name The user-given name of this overlay layer.
   * @param overlayOptions The {@link VectorOverlayOptions} for this instance.
   */
  VectorOverlay(
      const std::string& name,
      const VectorOverlayOptions& overlayOptions = VectorOverlayOptions());
  virtual ~VectorOverlay() noexcept;

  /**
   * @brief A future that resolves when this VectorOverlay has been destroyed
   * (i.e. its destructor has been called) and all async operations that it was
   * executing have completed.
   *
   * @param asyncSystem The AsyncSystem to use for the returned SharedFuture,
   * if required. If this method is called multiple times, all invocations
   * must pass {@link AsyncSystem} instances that compare equal to each other.
   */
  CesiumAsync::SharedFuture<void>&
  getAsyncDestructionCompleteEvent(const CesiumAsync::AsyncSystem& asyncSystem);

  /**
   * @brief Gets the name of this overlay.
   */
  const std::string& getName() const noexcept { return this->_name; }

  /**
   * @brief Gets options for this overlay.
   */
  VectorOverlayOptions& getOptions() noexcept { return this->_options; }

  /** @copydoc getOptions */
  const VectorOverlayOptions& getOptions() const noexcept {
    return this->_options;
  }

  /**
   * @brief Gets the credits for this overlay.
   */
  const std::vector<Credit>& getCredits() const noexcept {
    return this->_credits;
  }

  /**
   * @brief Gets the credits for this overlay.
   */
  std::vector<Credit>& getCredits() noexcept { return this->_credits; }

  /**
   * @brief Create a placeholder tile provider can be used in place of the real
   * one while {@link createTileProvider} completes asynchronously.
   *
   * @param asyncSystem The async system used to do work in threads.
   * @param pAssetAccessor The interface used to download assets like overlay
   * metadata and tiles.
   * @return The placeholder.
   */
  CesiumUtility::IntrusivePointer<VectorOverlayTileProvider> createPlaceholder(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor) const;

  using CreateTileProviderResult = nonstd::expected<
      CesiumUtility::IntrusivePointer<VectorOverlayTileProvider>,
      RasterOverlayLoadFailureDetails>;

  /**
   * @brief Begins asynchronous creation of a tile provider for this overlay
   * and eventually returns it via a Future.
   *
   * @param asyncSystem The async system used to do work in threads.
   * @param pAssetAccessor The interface used to download assets like overlay
   * metadata and tiles.
   * @param pCreditSystem The {@link CreditSystem} to use when creating a
   * per-TileProvider {@link Credit}.
   * @param pPrepareRendererResources The interface used to prepare Vector
   * images for rendering.
   * @param pLogger The logger to which to send messages about the tile provider
   * and tiles.
   * @param pOwner The overlay that owns this overlay, or nullptr if this
   * overlay is not aggregated.
   * @return The future that resolves to the tile provider when it is ready, or
   * to error details in the case of an error.
   */
  virtual CesiumAsync::Future<CreateTileProviderResult> createTileProvider(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CreditSystem>& pCreditSystem,
      const std::shared_ptr<IPrepareVectorMapResources>&
          pPrepareMapResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      CesiumUtility::IntrusivePointer<const VectorOverlay> pOwner) const = 0;

private:
  struct DestructionCompleteDetails {
    CesiumAsync::AsyncSystem asyncSystem;
    CesiumAsync::Promise<void> promise;
    CesiumAsync::SharedFuture<void> future;
  };

  std::string _name;
  VectorOverlayOptions _options;
  std::vector<Credit> _credits;
  std::optional<DestructionCompleteDetails> _destructionCompleteDetails;
};

} // namespace Cesium3DTilesSelection
