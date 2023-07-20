
#include <Cesium3DTilesSelection/MVTUtilities.h>
#include <mapbox/earcut.hpp>
#include <vector>

namespace Cesium3DTilesSelection 
{

	std::vector<uint32_t> MVTUtilities::TriangulateLineString(const std::vector<glm::dvec2>& lineString, const std::vector<std::vector<glm::dvec2>>& holes) 
	{
		std::vector<uint32_t> indices;
		const size_t vertexCount = lineString.size();

		if (vertexCount < 3) {
		return indices;
		}

		using Point = std::array<double, 2>;

		//polygon 数组
		std::vector<Point> localPolygon;
		std::vector<std::vector<Point>> localPolygons;
		for (glm::dvec2 pos : lineString) 
		{
			localPolygon.push_back({pos.x, pos.y});
		}
		localPolygons.emplace_back(localPolygon);

		//hole 数组
		for (std::vector<glm::dvec2> hole : holes) 
		{
			std::vector<Point> holePolygon;
			for (glm::dvec2 pos : hole) 
			{
                holePolygon.push_back({pos.x, pos.y});
            }
			localPolygons.emplace_back(holePolygon);
		}

		indices = mapbox::earcut<uint32_t>(localPolygons);
		return indices;
    }

} // namespace Cesium3DTilesSelection
