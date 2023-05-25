#pragma once

#include "CreditSystem.h"
#include "Library.h"
#include "RasterOverlayDetails.h"
#include  "TileContent.h"

#include <CesiumGeospatial/Projection.h>
#include <CesiumGltf/VectorModel.h>

#include <memory>
#include <variant>

namespace Cesium3DTilesSelection {

using VectorOverlayDetails = RasterOverlayDetails;
/**
 * @brief A content tag that indicates a tile has a glTF model content and
 * render resources for the model
 */
class CESIUM3DTILESSELECTION_API VectorTileRenderContent {
public:
  /**
   * @brief Construct the content with a glTF model
   *
   * @param model A glTF model that will be owned by this content
   */
  VectorTileRenderContent(CesiumGltf::VectorModel&& model);

  /**
   * @brief Retrieve a glTF model that is owned by this content
   *
   * @return A glTF model that is owned by this content
   */
  const CesiumGltf::VectorModel& getModel() const noexcept;

  /**
   * @brief Retrieve a glTF model that is owned by this content
   *
   * @return A glTF model that is owned by this content
   */
  CesiumGltf::VectorModel& getModel() noexcept;

  /**
   * @brief Set the glTF model for this content
   *
   * @param model A glTF model that will be owned by this content
   */
  void setModel(const CesiumGltf::VectorModel& model);

  /**
   * @brief Set the glTF model for this content
   *
   * @param model A glTF model that will be owned by this content
   */
  void setModel(CesiumGltf::VectorModel&& model);

  /**
   * @brief Get the {@link VectorOverlayDetails} which is the result of generating raster overlay UVs for the glTF model
   *
   * @return The {@link VectorOverlayDetails} that is owned by this content
   */
  const VectorOverlayDetails& getVectorOverlayDetails() const noexcept;

  /**
   * @brief Get the {@link VectorOverlayDetails} which is the result of generating raster overlay UVs for the glTF model
   *
   * @return The {@link VectorOverlayDetails} that is owned by this content
   */
  VectorOverlayDetails& getVectorOverlayDetails() noexcept;

  /**
   * @brief Set the {@link VectorOverlayDetails} which is the result of generating raster overlay UVs for the glTF model
   *
   * @param vectorOverlayDetails The {@link VectorOverlayDetails} that will be owned by this content
   */
  void
  setVectorOverlayDetails(const VectorOverlayDetails& vectorOverlayDetails);

  /**
   * @brief Set the {@link VectorOverlayDetails} which is the result of generating raster overlay UVs for the glTF model
   *
   * @param vectorOverlayDetails The {@link VectorOverlayDetails} that will be owned by this content
   */
  void setVectorOverlayDetails(VectorOverlayDetails&& vectorOverlayDetails);

  /**
   * @brief Get the list of {@link Credit} of the content
   *
   * @return The list of {@link Credit} of the content
   */
  const std::vector<Credit>& getCredits() const noexcept;

  /**
   * @brief Get the list of {@link Credit} of the content
   *
   * @return The list of {@link Credit} of the content
   */
  std::vector<Credit>& getCredits() noexcept;

  /**
   * @brief Set the list of {@link Credit} for the content
   *
   * @param credits The list of {@link Credit} to be owned by the content
   */
  void setCredits(std::vector<Credit>&& credits);

  /**
   * @brief Set the list of {@link Credit} for the content
   *
   * @param credits The list of {@link Credit} to be owned by the content
   */
  void setCredits(const std::vector<Credit>& credits);

  /**
   * @brief Get the render resources created for the glTF model of the content
   *
   * @return The render resources that is created for the glTF model
   */
  void* getRenderResources() const noexcept;

  /**
   * @brief Set the render resources created for the glTF model of the content
   *
   * @param pRenderResources The render resources that is created for the glTF
   * model
   */
  void setRenderResources(void* pRenderResources) noexcept;

  /**
   * @brief Get the fade percentage that this tile during an LOD transition.
   *
   * This will be used when {@link TilesetOptions::enableLodTransitionPeriod}
   * is true. Tile fades can be used to make LOD transitions appear less abrupt
   * and jarring. It is up to client implementations how to render the fade
   * percentage, but dithered fading is recommended.
   *
   * @return The fade percentage.
   */
  float getLodTransitionFadePercentage() const noexcept;

  /**
   * @brief Set the fade percentage of this tile during an LOD transition with.
   * Not to be used by clients.
   *
   * @param percentage The new fade percentage.
   */
  void setLodTransitionFadePercentage(float percentage) noexcept;

private:
  CesiumGltf::VectorModel _model;
  void* _pRenderResources;
  VectorOverlayDetails _vectorOverlayDetails;
  std::vector<Credit> _credits;
  float _lodTransitionFadePercentage;
};

/**
 * @brief A tile content container that can store and query the content type
 * that is currently being owned by the tile
 */
class CESIUM3DTILESSELECTION_API VectorTileContent
{
  using VectorTileContentKindImpl = std::variant<
      TileUnknownContent,
      TileEmptyContent,
      TileExternalContent,
      std::unique_ptr<VectorTileRenderContent>>;

public:
  /**
   * @brief Construct an unknown content for a tile. This constructor
   * is useful when the tile content is known after its content is downloaded by
   * {@link TilesetContentLoader}
   */
  VectorTileContent();

  /**
   * @brief Construct an empty content for a tile
   */
  VectorTileContent(TileEmptyContent content);

  /**
   * @brief Construct an external content for a tile whose content
   * points to an external tileset
   */
  VectorTileContent(TileExternalContent content);

  /**
   * @brief Set an unknown content tag for a tile. This constructor
   * is useful when the tile content is known after its content is downloaded by
   * {@link TilesetContentLoader}
   */
  void setContentKind(TileUnknownContent content);

  /**
   * @brief Construct an empty content tag for a tile
   */
  void setContentKind(TileEmptyContent content);

  /**
   * @brief Set an external content for a tile whose content
   * points to an external tileset
   */
  void setContentKind(TileExternalContent content);

  /**
   * @brief Set a glTF model content for a tile
   */
  void setContentKind(std::unique_ptr<VectorTileRenderContent>&& content);

  /**
   * @brief Query if a tile has an unknown content
   */
  bool isUnknownContent() const noexcept;

  /**
   * @brief Query if a tile has an empty content
   */
  bool isEmptyContent() const noexcept;

  /**
   * @brief Query if a tile has an external content which points to
   * an external tileset
   */
  bool isExternalContent() const noexcept;

  /**
   * @brief Query if a tile has an glTF model content
   */
  bool isRenderContent() const noexcept;

  /**
   * @brief Get the {@link VectorTileRenderContent} which stores the glTF model
   * and render resources of the tile
   *
   * @return The {@link VectorTileRenderContent} which stores the glTF model
   * and render resources of the tile
   */
  const VectorTileRenderContent* getRenderContent() const noexcept;

  /**
   * @brief Get the {@link VectorTileRenderContent} which stores the glTF model
   * and render resources of the tile
   *
   * @return The {@link VectorTileRenderContent} which stores the glTF model
   * and render resources of the tile
   */
  VectorTileRenderContent* getRenderContent() noexcept;

private:
  VectorTileContentKindImpl _contentKind;
};
} // namespace Cesium3DTilesSelection
