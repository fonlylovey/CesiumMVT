
#include <Cesium3DTilesSelection/MVTUtilities.h>
#include <mapbox/earcut.hpp>
#include <vector>

namespace Cesium3DTilesSelection 
{

CesiumGeometry::QuadtreeTileID MVTUtilities::GetTileID(const TileID& tileId) 
{

    struct Operation 
    {

        CesiumGeometry::QuadtreeTileID operator()(const std::string& ) 
        {
             return CesiumGeometry::QuadtreeTileID(0,0,0);
        }

        CesiumGeometry::QuadtreeTileID operator()(const CesiumGeometry::QuadtreeTileID& quadtreeTileId) 
        {
            return quadtreeTileId;
        }

        CesiumGeometry::QuadtreeTileID operator()(const CesiumGeometry::OctreeTileID& octreeTileId) 
        {
            return CesiumGeometry::QuadtreeTileID(octreeTileId.level, octreeTileId.x, octreeTileId.y);
        }

        CesiumGeometry::QuadtreeTileID operator()(const CesiumGeometry::UpsampledQuadtreeNode& upsampledQuadtreeNode) 
        {
            return CesiumGeometry::QuadtreeTileID(upsampledQuadtreeNode.tileID);
        }
      };
    return std::visit(Operation{}, tileId);
}

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
