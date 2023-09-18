
#include <Cesium3DTilesSelection/MVTUtilities.h>
#include <mapbox/earcut.hpp>
#include <vector>
#include "geos.h"
#include <geos/operation/buffer/BufferParameters.h>
#include <geos/operation/buffer/BufferBuilder.h>
#include <geos/operation/buffer/BufferOp.h>
#include <geos/geom/LineString.h>
#include <geos/geom/Polygon.h>
#include <geos/geom/CoordinateSequence.h>
#include <geos/geom/Coordinate.h>

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

    std::vector<geos::geom::Coordinate> MVTUtilities::GlmToGoes(const std::vector<glm::dvec2>& lineString) 
    {
        std::vector<geos::geom::Coordinate> geosArray;
        for (std::vector<glm::dvec2>::const_iterator iter = lineString.begin(); iter != lineString.end(); iter++)
        {
            geosArray.emplace_back(geos::geom::Coordinate(iter->x, iter->y));
        }
        return geosArray;
    }

    std::vector<glm::dvec2> MVTUtilities::CreateGoesBuffer(
            const std::vector<glm::dvec2>& lineString,
            float width) 
    {
        std::vector<glm::dvec2> bufferArray;
        const std::vector<geos::geom::Coordinate> geosArray = GlmToGoes(lineString);
        geos::geom::CoordinateSequence::Ptr coords = std::make_unique< geos::geom::CoordinateSequence>();
        coords->setPoints(geosArray);

        geos::geom::GeometryFactory::Ptr geomFactory = geos::geom::GeometryFactory::create();
       
        geos::geom::LineString::Ptr line = geomFactory->createLineString(*coords);
        // 创建一个BufferParameters对象，设置面带的参数 
        geos::operation::buffer::BufferParameters params;
        params.setEndCapStyle(geos::operation::buffer::BufferParameters::CAP_FLAT); // 设置端点样式为平头
        params.setJoinStyle(geos::operation::buffer::BufferParameters::JOIN_MITRE); // 设置连接样式为斜接
        params.setMitreLimit(2.0); // 设置斜接限制为2.0
        
        // 创建一个BufferBuilder对象，用于构建面带
        geos::operation::buffer::BufferBuilder* builder = new geos::operation::buffer::BufferBuilder(params);
        // 调用buffer方法，传入一个distance参数，表示面带的宽度，得到一个Polygon对象，表示面带builder->buffer(line.get(),width);

        geos::geom::Geometry::Ptr geomBuffer = builder->buffer(line.get(), width);
        geos::geom::Polygon* polygon = dynamic_cast<geos::geom::Polygon*>(geomBuffer.get());
        if(polygon == nullptr) 
        {
            return bufferArray;
        }
        // 调用Polygon对象的exterior属性，得到一个LinearRing对象，表示面带的外边界
        const geos::geom::LinearRing* ring = polygon->getExteriorRing();
        
        // 调用LinearRing对象的coords属性，得到一个CoordinateSequence对象，表示坐标序列
        geos::geom::CoordinateSequence::Ptr vertices = ring->getCoordinates();
        
        for (size_t i = 0; i < vertices->size(); i++) 
        {
            // 调用Coordinate对象的x和y属性，得到顶点的坐标
            double x = vertices->getX(i);
            double y = vertices->getY(i);
            bufferArray.emplace_back(glm::dvec2(x, y));
        }
        return bufferArray;
    }

} // namespace Cesium3DTilesSelection
