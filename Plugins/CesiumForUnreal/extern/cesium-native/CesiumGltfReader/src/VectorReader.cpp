#include "CesiumGltfReader/VectorReader.h"
#include "registerExtensions.h"

#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>

#include <CesiumUtility/Uri.h>
#include <VectorTileReader.h>


using namespace CesiumAsync;
using namespace CesiumGltf;
using namespace CesiumGltfReader;
using namespace CesiumJsonReader;
using namespace CesiumUtility;

namespace
{
	class VectorGeomHandler : public VTR::GeometryHandler
	{
	public:
	VectorFeature feature;

	virtual void points_begin(uint32_t count)
	{
        count;
        VectorGeometry geom;
		feature.featureType = FeatureType::Point;
		feature.geometry.push_back(geom);
	}

	virtual void points_point(const vtzero::point_3d &pt)
	{
		feature.geometry.rbegin()->points.emplace_back(pt.x, pt.y, pt.z);
	}

	virtual void points_end(vtzero::ring_type type = vtzero::ring_type::invalid)
	{
		feature.geometry.rbegin()->ringType = static_cast<RingType>(type);
	}

	virtual void linestring_begin(uint32_t count)
	{
		count;
		VectorGeometry geom;
		feature.geometry.push_back(geom);
	}

	virtual void linestring_point(const vtzero::point_3d&pt)
	{
		feature.geometry.rbegin()->points.emplace_back(pt.x, pt.y, pt.z);
	}

	virtual void linestring_end(vtzero::ring_type type = vtzero::ring_type::invalid)
	{
		feature.geometry.rbegin()->ringType = static_cast<RingType>(type);
	}

	virtual void ring_begin(uint32_t count)
	{
		count;
		VectorGeometry geom;
		feature.geometry.push_back(geom);
	}

	virtual void ring_point(const vtzero::point_3d &pt)
	{
		feature.geometry.rbegin()->points.emplace_back(pt.x, pt.y, pt.z);
	}

	virtual void ring_end(vtzero::ring_type type = vtzero::ring_type::invalid)
	{
		feature.geometry.rbegin()->ringType = static_cast<RingType>(type);
	}

	//
	void controlpoints_begin(const uint32_t count) 
	{
		count;
        VectorGeometry geom;
        feature.geometry.push_back(geom);
	}

	void controlpoints_point(const vtzero::point_3d pt) 
	{
		feature.geometry.rbegin()->points.emplace_back(pt.x, pt.y, pt.z);
	}

	void controlpoints_end() 
	{
        
	}

	void knots_begin(const uint32_t count, vtzero::index_value /*scaling*/) 
	{
		count;
		VectorGeometry geom;
		feature.geometry.push_back(geom);
	}

	void knots_value(const int64_t /*value*/) 
	{
	}

	void knots_end() 
	{
	}
	};

	//解析二进制的矢量数据
	CesiumGltf::VectorTile* resolvingGeoData(const std::string& strData) 
	{
        CesiumGltf::VectorTile* tileModel = new CesiumGltf::VectorTile;

		vtzero::vector_tile tile(strData);
		for (const auto layer : tile)
		{
			CesiumGltf::VectorLayer vlayer;
            vlayer.name = layer.name().to_string();
			for (const auto feature : layer)
			{
				VectorGeomHandler* handler = new VectorGeomHandler;
				feature.decode_geometry(*handler);
                handler->feature.featureID = feature.integer_id();
                handler->feature.featureType = static_cast<FeatureType>(feature.geometry_type());
                vlayer.features.push_back(handler->feature);
                delete handler;
			}
            tileModel->layers.push_back(vlayer);
		}
		return tileModel;
	}


	VectorReaderResult readBinaryVector(
		const CesiumJsonReader::ExtensionReaderContext& context,
	const std::string& data)
	{
	    context;
	    VectorReaderResult result;
	    try
	    {
		    CesiumGltf::VectorTile* tileModel = resolvingGeoData(data);
            result.model = tileModel;
	    }
	    catch (std::runtime_error ex)
	    {
            result.model = nullptr;
		    result.errors.push_back("Failed to process data!");
	    }
   
	    return result;
	    }

} // namespace


VectorReader::VectorReader() : _context() {
    registerExtensions(this->_context);
}

  CesiumJsonReader::ExtensionReaderContext& VectorReader::getExtensions() {
    return this->_context;
}

const CesiumJsonReader::ExtensionReaderContext& VectorReader::getExtensions() const
{
    return this->_context;
}

VectorReaderResult VectorReader::readVector(
    const std::string& data,
    const VectorReaderOptions& options) const
{
    const CesiumJsonReader::ExtensionReaderContext& context = this->getExtensions();
    std::string strData = data;
    if (options.decodeDraco)
    {
        VTR::VectorTileReader reader;
        strData = reader.mvtDecode(data);
    }
    VectorReaderResult result = readBinaryVector(context, strData);

    return result;
}