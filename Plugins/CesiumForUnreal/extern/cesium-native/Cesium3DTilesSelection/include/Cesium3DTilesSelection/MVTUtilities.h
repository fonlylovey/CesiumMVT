#pragma once

#include "Library.h"

#include <glm/glm.hpp>
#include <array>
#include <vector>

namespace Cesium3DTilesSelection 
{
	/**
	 * A collection of utility functions that are used to process and transform a
	 * vector model
	 */
	struct CESIUM3DTILESSELECTION_API MVTUtilities 
	{
		/*
		* https://github.com/mapbox/earcut.hpp
		* ����LineString�������飬���ǻ���Ϊmesh������
		*/
		static std::vector<uint32_t> TriangulateLineString(const std::vector<glm::dvec2>& lineString, const std::vector<std::vector<glm::dvec2>>& holes);
	};
 } // namespace Cesium3DTilesSelection
