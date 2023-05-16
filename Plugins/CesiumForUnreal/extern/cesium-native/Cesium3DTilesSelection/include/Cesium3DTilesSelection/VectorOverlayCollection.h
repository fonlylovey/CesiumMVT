#pragma once

#include "Library.h"
#include "VectorOverlay.h"
#include "VectorOverlayTileProvider.h"
#include "Tile.h"
#include "TilesetVectorExternals.h"

#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCountedNonThreadSafe.h>
#include <CesiumUtility/Tracing.h>

#include <gsl/span>

#include <memory>
#include <vector>

namespace Cesium3DTilesSelection {

/**
 * @brief A collection of {@link VectorOverlay} instances that are associated
 * with a {@link Tileset}.
 *
 * The Vector overlay instances may be added to the Vector overlay collection
 * of a tileset that is returned with {@link Tileset::getOverlays}. When the
 * tileset is loaded, one {@link VectorOverlayTileProvider} will be created
 * for each Vector overlay that had been added. The Vector overlay tile provider
 * instances will be passed to the {@link VectorOverlayTile} instances that
 * they create when the tiles are updated.
 */
class CESIUM3DTILESSELECTION_API VectorOverlayCollection final {
public:
  /**
   * @brief Creates a new instance.
   *
   * @param loadedTiles The list of loaded tiles. The collection does not own
   * this list, so the list needs to be kept alive as long as the collection's
   * lifetime
   * @param externals A collection of loading system to load a Vector overlay
   */
  VectorOverlayCollection(
      Tile::LoadedLinkedList& loadedTiles,
      const TilesetVectorExternals& externals) noexcept;

  /**
   * @brief Deleted Copy constructor.
   *
   * @param rhs The other instance.
   */
  VectorOverlayCollection(const VectorOverlayCollection& rhs) = delete;

  /**
   * @brief Move constructor.
   *
   * @param rhs The other instance.
   */
  VectorOverlayCollection(VectorOverlayCollection&& rhs) noexcept = default;

  /**
   * @brief Deleted copy assignment.
   *
   * @param rhs The other instance.
   */
  VectorOverlayCollection&
  operator=(const VectorOverlayCollection& rhs) = delete;

  /**
   * @brief Move assignment.
   *
   * @param rhs The other instance.
   */
  VectorOverlayCollection&
  operator=(VectorOverlayCollection&& rhs) noexcept = default;

  ~VectorOverlayCollection() noexcept;

  /**
   * @brief Adds the given {@link VectorOverlay} to this collection.
   *
   * @param pOverlay The pointer to the overlay. This may not be `nullptr`.
   */
  void add(const CesiumUtility::IntrusivePointer<VectorOverlay>& pOverlay);

  /**
   * @brief Remove the given {@link VectorOverlay} from this collection.
   */
  void remove(
      const CesiumUtility::IntrusivePointer<VectorOverlay>& pOverlay) noexcept;

  /**
   * @brief Gets the overlays in this collection.
   */
  const std::vector<CesiumUtility::IntrusivePointer<VectorOverlay>>&
  getOverlays() const;

  /**
   * @brief Gets the tile providers in this collection. Each tile provider
   * corresponds with the overlay at the same position in the collection
   * returned by {@link getOverlays}.
   */
  const std::vector<CesiumUtility::IntrusivePointer<VectorOverlayTileProvider>>&
  getTileProviders() const;

  /**
   * @brief Gets the placeholder tile providers in this collection. Each
   * placeholder tile provider corresponds with the overlay at the same position
   * in the collection returned by {@link getOverlays}.
   */
  const std::vector<CesiumUtility::IntrusivePointer<VectorOverlayTileProvider>>&
  getPlaceholderTileProviders() const;

  /**
   * @brief Finds the tile provider for a given overlay.
   *
   * If the specified Vector overlay is not part of this collection, this method
   * will return nullptr.
   *
   * If the overlay's real tile provider hasn't finished being
   * created yet, a placeholder will be returned. That is, its
   * {@link VectorOverlayTileProvider::isPlaceholder} method will return true.
   *
   * @param overlay The overlay for which to obtain the tile provider.
   * @return The tile provider, if any, corresponding to the Vector overlay.
   */
  VectorOverlayTileProvider*
  findTileProviderForOverlay(VectorOverlay& overlay) noexcept;

  /**
   * @copydoc findTileProviderForOverlay
   */
  const VectorOverlayTileProvider*
  findTileProviderForOverlay(const VectorOverlay& overlay) const noexcept;

  /**
   * @brief Finds the placeholder tile provider for a given overlay.
   *
   * If the specified Vector overlay is not part of this collection, this method
   * will return nullptr.
   *
   * This method will return the placeholder tile provider even if the real one
   * has been created. This is useful to create placeholder tiles when the
   * rectangle in the overlay's projection is not yet known.
   *
   * @param overlay The overlay for which to obtain the tile provider.
   * @return The placeholder tile provider, if any, corresponding to the Vector
   * overlay.
   */
  VectorOverlayTileProvider*
  findPlaceholderTileProviderForOverlay(VectorOverlay& overlay) noexcept;

  /**
   * @copydoc findPlaceholderTileProviderForOverlay
   */
  const VectorOverlayTileProvider* findPlaceholderTileProviderForOverlay(
      const VectorOverlay& overlay) const noexcept;

  /**
   * @brief A constant iterator for {@link VectorOverlay} instances.
   */
  typedef std::vector<CesiumUtility::IntrusivePointer<VectorOverlay>>::
      const_iterator const_iterator;

  /**
   * @brief Returns an iterator at the beginning of this collection.
   */
  const_iterator begin() const noexcept;

  /**
   * @brief Returns an iterator at the end of this collection.
   */
  const_iterator end() const noexcept;

  /**
   * @brief Gets the number of overlays in the collection.
   */
  size_t size() const noexcept;

private:
  // We store the list of overlays and tile providers in this separate class
  // so that we can separate its lifetime from the lifetime of the
  // VectorOverlayCollection. We need to do this because the async operations
  // that create tile providers from overlays need to have somewhere to write
  // the result. And we can't extend the lifetime of the entire
  // VectorOverlayCollection until the async operations complete because the
  // VectorOverlayCollection has a pointer to the tile LoadedLinkedList, which
  // is owned externally and may become invalid before the async operations
  // complete.
  struct OverlayList
      : public CesiumUtility::ReferenceCountedNonThreadSafe<OverlayList> {
    std::vector<CesiumUtility::IntrusivePointer<VectorOverlay>> overlays{};
    std::vector<CesiumUtility::IntrusivePointer<VectorOverlayTileProvider>>
        tileProviders{};
    std::vector<CesiumUtility::IntrusivePointer<VectorOverlayTileProvider>>
        placeholders{};
  };

  Tile::LoadedLinkedList* _pLoadedTiles;
  TilesetVectorExternals _externals;
  CesiumUtility::IntrusivePointer<OverlayList> _pOverlays;
  CESIUM_TRACE_DECLARE_TRACK_SET(_loadingSlots, "Vector Overlay Loading Slot");
};

} // namespace Cesium3DTilesSelection
