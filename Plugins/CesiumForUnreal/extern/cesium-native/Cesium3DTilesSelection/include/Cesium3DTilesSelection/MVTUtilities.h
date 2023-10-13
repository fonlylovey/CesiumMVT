#pragma once

#include "Library.h"

#include <glm/glm.hpp>
#include <array>
#include <vector>
#include "TileID.h"

namespace Cesium3DTilesSelection 
{
	/**
	 * A collection of utility functions that are used to process and transform a
	 * vector model
	 */

    struct CESIUM3DTILESSELECTION_API Geometry
    {
        std::vector<glm::dvec3> vertexs;
    };

	struct CESIUM3DTILESSELECTION_API MVTUtilities 
	{
        static CesiumGeometry::QuadtreeTileID GetTileID(const TileID& tileId);

        static CesiumGeometry::QuadtreeTileID GetWTMSTileID(const TileID& tileId);

		/*
		* https://github.com/mapbox/earcut.hpp
		* 根据LineString顶点数组，三角化的为mesh三角形
		*/
		static std::vector<uint32_t> TriangulateLineString(const std::vector<glm::dvec2>& lineString, const std::vector<std::vector<glm::dvec2>>& holes);
	};
 } // namespace Cesium3DTilesSelection
