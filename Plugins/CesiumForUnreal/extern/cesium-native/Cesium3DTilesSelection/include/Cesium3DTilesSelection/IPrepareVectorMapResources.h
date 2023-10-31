#pragma once

#include "Library.h"
#include "TileLoadResult.h"

#include <CesiumAsync/Future.h>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <gsl/span>

#include <any>

namespace CesiumAsync {
class AsyncSystem;
}

namespace CesiumGeometry {
struct Rectangle;
}

namespace CesiumGltf {
struct VectorModel;
} // namespace CesiumGltf

namespace Cesium3DTilesSelection {

class Tile;
class VectorOverlayTile;
struct TileLoadResultAndRenderResources;

/**
 * @brief When implemented for a rendering engine, allows renderer resources to
 * be created and destroyed under the control of a {@link Tileset}.
 *
 * It is not supposed to be used directly by clients. It is implemented
 * for specific rendering engines to provide an infrastructure for preparing the
 * data of a {@link Tile} so that it can be used for rendering.
 *
 * Instances of this class are associated with a {@link Tileset}, in the
 * {@link TilesetExternals} structure that can be obtained
 * via {@link Tileset::getExternals}.
 */
class CESIUM3DTILESSELECTION_API IPrepareVectorMapResources {
public:
  virtual ~IPrepareVectorMapResources() = default;
  

    /**
       * @brief Prepares a Vector overlay tile.
       *
       * This method is invoked in the load thread and may modify the image.
       *
       * @param image The Vector tile image to prepare.
       * @param rendererOptions Renderer options associated with the Vector overlay tile from {@link VectorOverlayOptions::rendererOptions}.
       * @returns Arbitrary data representing the result of the load process. This
       * data is passed to {@link prepareVectorInMainThread} as the
       * `pLoadThreadResult` parameter.
       */
      virtual void* prepareVectorInLoadThread(
          CesiumGltf::VectorModel* model,
          const std::any& rendererOptions) = 0;

    /**
    * @brief Further preprares a Vector overlay tile.
    *
    * This is called after {@link prepareVectorInLoadThread}, and unlike that
    * method, this one is called from the same thread that called
    * {@link Tileset::updateView}.
    *
    * @param VectorTile The Vector tile to prepare.
    * @param pLoadThreadResult The value returned from
    * {@link prepareVectorInLoadThread}.
    * @returns Arbitrary data representing the result of the load process. Note
    * that the value returned by {@link prepareVectorInLoadThread} will _not_ be
    * automatically preserved and passed to {@link free}. If you need to free
    * that value, do it in this method before returning. If you need that value
    * later, add it to the object returned from this method.
    */
    virtual void* prepareVectorInMainThread(
        VectorOverlayTile& vectorTile,
        void* pLoadThreadResult) = 0;

    virtual void freeVector(
	    const Cesium3DTilesSelection::VectorOverlayTile& vectorTile,
	    void* pLoadThreadResult,
	    void* pMainThreadResult) noexcept = 0;

    /**
    * @brief Attaches a Vector overlay tile to a geometry tile.
    *
    * @param tile The geometry tile.
    * @param overlayTextureCoordinateID The ID of the overlay texture coordinate
    * set to use.
    * @param VectorTile The Vector overlay tile to add. The Vector tile will have
    * been previously prepared with a call to {@link prepareVectorInLoadThread}
    * followed by {@link prepareVectorInMainThread}.
    * @param pMainThreadRendererResources The renderer resources for this Vector
    * tile, as created and returned by {@link prepareVectorInMainThread}.
    * @param translation The translation to apply to the texture coordinates
    * identified by `overlayTextureCoordinateID`. The texture coordinates to use
    * to sample the Vector image are computed as `overlayTextureCoordinates *
    * scale + translation`.
    * @param scale The scale to apply to the texture coordinates identified by
    * `overlayTextureCoordinateID`. The texture coordinates to use to sample the
    * Vector image are computed as `overlayTextureCoordinates * scale +
    * translation`.
    */
    virtual void attachVectorInMainThread(
        const Tile& tile,
        int32_t overlayTextureCoordinateID,
        const VectorOverlayTile& VectorTile,
        void* pMainThreadRendererResources,
        const glm::dvec2& translation,
        const glm::dvec2& scale) = 0;

    /**
    * @brief Detaches a Vector overlay tile from a geometry tile.
    *
    * @param tile The geometry tile.
    * @param overlayTextureCoordinateID The ID of the overlay texture coordinate
    * set to which the Vector tile was previously attached.
    * @param VectorTile The Vector overlay tile to remove.
    * @param pMainThreadRendererResources The renderer resources for this Vector
    * tile, as created and returned by {@link prepareVectorInMainThread}.
    */
    virtual void detachVectorInMainThread(
    const Tile& tile,
    int32_t overlayTextureCoordinateID,
    const VectorOverlayTile& VectorTile,
    void* pMainThreadRendererResources) noexcept = 0;
};

} // namespace Cesium3DTilesSelection
