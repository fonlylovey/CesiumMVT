#pragma once

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumGeometry/Rectangle.h>
#include <CesiumGltf/VectorModel.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCountedNonThreadSafe.h>

#include <vector>

namespace Cesium3DTilesSelection {

struct Credit;
class VectorOverlay;
class VectorOverlayTileProvider;

/**
 * @brief Vector image data for a tile in a quadtree.
 *
 * Instances of this clas represent tiles of a quadtree that have
 * an associated image, which us used as an imagery overlay
 * for tile geometry. The connection between the imagery data
 * and the actual tile geometry is established via the
 * {@link VectorMappedTo3DTile} class, which combines a
 * Vector overlay tile with texture coordinates, to map the
 * image on the geometry of a {@link Tile}.
 */
class VectorOverlayTile final
    : public CesiumUtility::ReferenceCountedNonThreadSafe<VectorOverlayTile> {
public:
  /**
   * @brief Lifecycle states of a Vector overlay tile.
   */
  enum class LoadState {
    /**
     * @brief Indicator for a placeholder tile.
     */
    Placeholder = -2,

    /**
     * @brief The image request or image creation failed.
     */
    Failed = -1,

    /**
     * @brief The initial state
     */
    Unloaded = 0,

    /**
     * @brief The request for loading the image data is still pending.
     */
    Loading = 1,

    /**
     * @brief The image data has been loaded and the image has been created.
     */
    Loaded = 2,

    /**
     * @brief The rendering resources for the image data have been created.
     */
    Done = 3
  };

  /**
   * @brief Constructs a placeholder tile for the tile provider.
   *
   * The {@link getState} of this instance will always be
   * {@link LoadState::Placeholder}.
   *
   * @param tileProvider The {@link VectorOverlayTileProvider}. This object
   * _must_ remain valid for the entire lifetime of the tile. If the tile
   * provider is destroyed before the tile, undefined behavior will result.
   */
  VectorOverlayTile(VectorOverlayTileProvider& tileProvider) noexcept;

  /**
   * @brief Creates a new instance.
   *
   * The tile will start in the `Unloaded` state, and will not begin loading
   * until {@link VectorOverlayTileProvider::loadTile} or
   * {@link VectorOverlayTileProvider::loadTileThrottled} is called.
   *
   * @param tileProvider The {@link VectorOverlayTileProvider}. This object
   * _must_ remain valid for the entire lifetime of the tile. If the tile
   * provider is destroyed before the tile, undefined behavior may result.
   * @param targetScreenPixels The maximum number of pixels on the screen that
   * this tile is meant to cover. The overlay image should be approximately this
   * many pixels divided by the
   * {@link VectorOverlayOptions::maximumScreenSpaceError} in order to achieve
   * the desired level-of-detail, but it does not need to be exactly this size.
   * @param imageryRectangle The rectangle that the returned image must cover.
   * It is allowed to cover a slightly larger rectangle in order to maintain
   * pixel alignment. It may also cover a smaller rectangle when the overlay
   * itself does not cover the entire rectangle.
   */
  VectorOverlayTile(
      VectorOverlayTileProvider& tileProvider,
      const glm::dvec2& targetScreenPixels,
      const CesiumGeometry::Rectangle& imageryRectangle) noexcept;

  /** @brief Default destructor. */
  ~VectorOverlayTile();

  /**
   * @brief Returns the {@link VectorOverlayTileProvider} that created this instance.
   */
  VectorOverlayTileProvider& getTileProvider() noexcept {
    return *this->_pTileProvider;
  }

  /**
   * @brief Returns the {@link VectorOverlayTileProvider} that created this instance.
   */
  const VectorOverlayTileProvider& getTileProvider() const noexcept {
    return *this->_pTileProvider;
  }

  /**
   * @brief Returns the {@link VectorOverlay} that created this instance.
   */
  VectorOverlay& getOverlay() noexcept;

  /**
   * @brief Returns the {@link VectorOverlay} that created this instance.
   */
  const VectorOverlay& getOverlay() const noexcept;

  /**
   * @brief Returns the {@link CesiumGeometry::Rectangle} that defines the bounds
   * of this tile in the Vector overlay's projected coordinates.
   */
  const CesiumGeometry::Rectangle& getRectangle() const noexcept {
    return this->_rectangle;
  }

  /**
   * @brief Gets the number of screen pixels in each direction that should be
   * covered by this tile's texture.
   *
   * This is used to control which content (how highly detailed) the
   * {@link VectorOverlayTileProvider} uses within the bounds of this tile.
   */
  glm::dvec2 getTargetScreenPixels() const noexcept {
    return this->_targetScreenPixels;
  }

  /**
   * @brief Returns the current {@link LoadState}.
   */
  LoadState getState() const noexcept { return this->_state; }

  /**
   * @brief Returns the list of {@link Credit}s needed for this tile.
   */
  const std::vector<Credit>& getCredits() const noexcept {
    return this->_tileCredits;
  }

  /**
   * @brief Returns the image data for the tile.
   *
   * This will only contain valid image data if the {@link getState} of
   * this tile is {@link LoadState `Loaded`} or {@link LoadState `Done`}.
   *
   * @return The image data.
   */
  const CesiumGltf::VectorModel* getVectorModel() const noexcept {
    return this->_vectorModel;
  }

  /**
   * @brief Returns the image data for the tile.
   *
   * This will only contain valid image data if the {@link getState} of
   * this tile is {@link LoadState `Loaded`} or {@link LoadState `Done`}.
   *
   * @return The image data.
   */
  CesiumGltf::VectorModel* getVectorModel() noexcept {
    return this->_vectorModel;
  }

  /**
   * @brief Create the renderer resources for the loaded image.
   *
   * If the {@link getState} of this tile is not {@link LoadState `Loaded`},
   * then nothing will be done. Otherwise, the renderer resources will be
   * prepared, so that they may later be obtained with
   * {@link getRendererResources}, and the {@link getState} of this tile
   * will change to {@link LoadState `Done`}.
   */
  void loadInMainThread();

  /**
   * @brief Returns the renderer resources that have been created for this tile.
   */
  void* getRendererResources() const noexcept {
    return this->_pRendererResources;
  }

  /**
   * @brief Set the renderer resources for this tile.
   *
   * This function is not supposed to be called by clients.
   */
  void setRendererResources(void* pValue) noexcept {
    this->_pRendererResources = pValue;
  }

 void setTileID(int level, int row, int col, int tmsRow) {
    _level = level;
    _row = row;
    _col = col;
    _tmsRow = tmsRow;
  }

  std::string getTileID();

private:
  friend class VectorOverlayTileProvider;

  void setState(LoadState newState) noexcept;

  // This is a raw pointer instead of an IntrusivePointer in order to avoid
  // circular references, particularly among a placeholder tile provider and
  // placeholder tile. However, to avoid undefined behavior, the tile provider
  // is required to outlive the tile. In normal use, the VectorOverlayCollection
  // ensures that this is true.
  VectorOverlayTileProvider* _pTileProvider;
  glm::dvec2 _targetScreenPixels;
  CesiumGeometry::Rectangle _rectangle;
  std::vector<Credit> _tileCredits;
  LoadState _state;
  CesiumGltf::VectorModel* _vectorModel;
  void* _pRendererResources;
  int _level;
  int _row;
  int _tmsRow;
  int _col;
};
} // namespace Cesium3DTilesSelection
