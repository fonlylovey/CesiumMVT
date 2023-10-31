#pragma once

#include "IPrepareVectorMapResources.h"
#include "VectorOverlayTile.h"

#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <memory>

namespace Cesium3DTilesSelection {

class Tile;

/**
 * @brief The result of applying a {@link VectorOverlayTile} to geometry.
 *
 * Instances of this class are used by a {@link Tile} in order to map
 * imagery data that is given as {@link VectorOverlayTile} instances
 * to the 2D region that is covered by the tile geometry.
 */
class VectorMappedTo3DTile final {
public:
  /**
   * @brief The states indicating whether the Vector tile is attached to the
   * geometry.
   */
  enum class AttachmentState {
    /**
     * @brief This Vector tile is not yet attached to the geometry at all.
     */
    Unattached = 0,

    /**
     * @brief This Vector tile is attached to the geometry, but it is a
     * temporary, low-res version usable while the full-res version is loading.
     */
    TemporarilyAttached = 1,

    /**
     * @brief This Vector tile is attached to the geometry.
     */
    Attached = 2
  };

  /**
   * @brief Creates a new instance.
   *
   * @param pVectorTile The {@link VectorOverlayTile} that is mapped to the
   * geometry.
   * @param textureCoordinateIndex The index of the texture coordinates to use
   * with this mapped Vector overlay.
   */
  VectorMappedTo3DTile(
      const CesiumUtility::IntrusivePointer<VectorOverlayTile>& pVectorTile,
      int32_t textureCoordinateIndex);

  /**
   * @brief Returns a {@link VectorOverlayTile} that is currently loading.
   *
   * The caller has to check the exact state of this tile, using
   * {@link Tile::getState}.
   *
   * @return The tile that is loading, or `nullptr`.
   */
  VectorOverlayTile* getLoadingTile() noexcept {
    return this->_pLoadingTile.get();
  }

  /** @copydoc getLoadingTile */
  const VectorOverlayTile* getLoadingTile() const noexcept {
    return this->_pLoadingTile.get();
  }

  /**
   * @brief Returns the {@link VectorOverlayTile} that represents the imagery
   * data that is ready to render.
   *
   * This will be `nullptr` when the tile data has not yet been loaded.
   *
   * @return The tile, or `nullptr`.
   */
  VectorOverlayTile* getReadyTile() noexcept { return this->_pReadyTile.get(); }

  /** @copydoc getReadyTile */
  const VectorOverlayTile* getReadyTile() const noexcept {
    return this->_pReadyTile.get();
  }

  /**
   * @brief Returns an identifier for the texture coordinates of this tile.
   *
   * The texture coordinates for this Vector are found in the glTF as an
   * attribute named `_CESIUMOVERLAY_n` where `n` is this value.
   *
   * @return The texture coordinate ID.
   */
  int32_t getTextureCoordinateID() const noexcept {
    return this->_textureCoordinateID;
  }

  /**
   * @brief Sets the texture coordinate ID.
   *
   * @see getTextureCoordinateID
   *
   * @param textureCoordinateID The ID.
   */
  void setTextureCoordinateID(int32_t textureCoordinateID) noexcept {
    this->_textureCoordinateID = textureCoordinateID;
  }

  /**
   * @brief Returns the translation that converts between the geometry texture
   * coordinates and the texture coordinates that should be used to sample this
   * Vector texture.
   *
   * `VectorCoordinates = geometryCoordinates * scale + translation`
   *
   * @returns The translation.
   */
  const glm::dvec2& getTranslation() const noexcept {
    return this->_translation;
  }

  /**
   * @brief Returns the scaling that converts between the geometry texture
   * coordinates and the texture coordinates that should be used to sample this
   * Vector texture.
   *
   * @see getTranslation
   *
   * @returns The scaling.
   */
  const glm::dvec2& getScale() const noexcept { return this->_scale; }

  /**
   * @brief Indicates whether this overlay tile is currently attached to its
   * owning geometry tile.
   *
   * When a Vector overlay tile is attached to a geometry tile,
   * {@link IPrepareRendererResources::attachVectorInMainThread} is invoked.
   * When it is detached,
   * {@link IPrepareRendererResources::detachVectorInMainThread} is invoked.
   */
  AttachmentState getState() const noexcept { return this->_state; }

  /**
   * @brief Update this tile during the update of its owner.
   *
   * This is only supposed to be called by {@link Tile::update}. It
   * will return whether there is a more detailed version of the
   * Vector data available.
   *
   * @param prepareRendererResources The IPrepareRendererResources used to
   * create render resources for Vector overlay
   * @param tile The owner tile.
   * @return The {@link MoreDetailAvailable} state.
   */
  bool update(IPrepareVectorMapResources& pPrepareMapResources, Tile& tile);

  /**
   * @brief Detach the Vector from the given tile.
   * @param prepareRendererResources The IPrepareRendererResources used to
   * detach Vector overlay from the tile geometry
   * @param tile The owner tile.
   */
  void detachFromTile(
      IPrepareVectorMapResources& pPrepareMapResources,
      Tile& tile) noexcept;

  /**
   * @brief Does a throttled load of the mapped {@link VectorOverlayTile}.
   *
   * @return If the mapped tile is already in the process of loading or it has
   * already finished loading, this method does nothing and returns true. If too
   * many loads are already in progress, this method does nothing and returns
   * false. Otherwise, it begins the asynchronous process to load the tile and
   * returns true.
   */
  bool loadThrottled() noexcept;

  /**
   * @brief Creates a maping between a {@link VectorOverlay} and a {@link Tile}.
   *
   * The returned mapping will be to a placeholder {@link VectorOverlayTile} if
   * the overlay's tile provider is not yet ready (i.e. it's still a
   * placeholder) or if the overlap between the tile and the Vector overlay
   * cannot yet be determined because the projected rectangle of the tile is not
   * yet known.
   *
   * Returns a pointer to the created `VectorMappedTo3DTile` in the Tile's
   * {@link Tile::getMappedVectorTiles} collection. Note that this pointer may
   * become invalid as soon as another item is added to or removed from this
   * collection.
   *
   * @param maximumScreenSpaceError The maximum screen space error that is used
   * for the current tile
   * @param tileProvider The overlay tile provider to map to the tile. This may
   * be a placeholder if the tile provider is not yet ready.
   * @param placeholder The placeholder tile provider for this overlay. This is
   * always a placeholder, even if the tile provider is already ready.
   * @param tile The tile to which to map the overlay.
   * @param missingProjections The list of projections for which there are not
   * yet any texture coordiantes. On return, the given overlay's Projection may
   * be added to this collection if the Tile does not yet have texture
   * coordinates for the Projection and the Projection is not already in the
   * collection.
   * @return A pointer the created mapping, which may be to a placeholder, or
   * nullptr if no mapping was created at all because the Tile does not overlap
   * the Vector overlay.
   */
  static VectorMappedTo3DTile* mapOverlayToTile(
      double maximumScreenSpaceError,
      VectorOverlayTileProvider& tileProvider,
      VectorOverlayTileProvider& placeholder,
      Tile& tile,
      std::vector<CesiumGeospatial::Projection>& missingProjections);

private:
  void computeTranslationAndScale(const Tile& tile);

  CesiumUtility::IntrusivePointer<VectorOverlayTile> _pLoadingTile;
  CesiumUtility::IntrusivePointer<VectorOverlayTile> _pReadyTile;
  int32_t _textureCoordinateID;
  glm::dvec2 _translation;
  glm::dvec2 _scale;
  AttachmentState _state;
  bool _originalFailed;
};

} // namespace Cesium3DTilesSelection
