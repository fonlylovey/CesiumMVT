#include "CesiumGltfReader/VectorReader.h"
#include "registerExtensions.h"

#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>

#include <CesiumUtility/Uri.h>
#include <VectorTileReader.hpp>


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
    CesiumGltf::VectorLayer layer;
    virtual void points_begin(uint32_t count)
    {
      geoCount = count;
	  VectorFeature feature;
      layer.features.emplace_back(feature);
    }

    virtual void points_point(const vtzero::point_3d &pt)
    {
      layer.features.rbegin()->points.emplace_back(pt.x, pt.y, pt.z);
    }

    virtual void points_end(vtzero::ring_type type = vtzero::ring_type::invalid)
    {
      layer.features.rbegin()->ringType = static_cast<int>(type);
    }

    virtual void linestring_begin(uint32_t count)
    {
		geoCount = count;
		VectorFeature feature;
		layer.features.emplace_back(feature);
    }

    virtual void linestring_point(const vtzero::point_3d&pt)
    {
       layer.features.rbegin()->points.emplace_back(pt.x, pt.y, pt.z);
    }

    virtual void linestring_end(vtzero::ring_type type = vtzero::ring_type::invalid)
    {
        layer.features.rbegin()->ringType = static_cast<int>(type);
    }

    virtual void ring_begin(uint32_t count)
    {
        geoCount = count;
		VectorFeature feature;
		layer.features.emplace_back(feature);
    }

    virtual void ring_point(const vtzero::point_3d &pt)
    {
         layer.features.rbegin()->points.emplace_back(pt.x, pt.y, pt.z);
    }

    virtual void ring_end(vtzero::ring_type type = vtzero::ring_type::invalid)
    {
         layer.features.rbegin()->ringType = static_cast<int>(type);
    }

	void controlpoints_begin(const uint32_t count) 
	{
         geoCount = count;
         VectorFeature feature;
         layer.features.emplace_back(feature);
    }

    void controlpoints_point(const vtzero::point_3d pt) 
	{
        layer.features.rbegin()->points.emplace_back(pt.x, pt.y, pt.z);
    }

    void controlpoints_end() 
	{
        
    }

	void knots_begin(const uint32_t count, vtzero::index_value /*scaling*/) 
	{
        geoCount = count;
		VectorFeature feature;
		layer.features.emplace_back(feature);
    }

    void knots_value(const int64_t /*value*/) 
	{
    }

    void knots_end() 
	{
    }
  };

  //解析二进制的矢量数据
  std::vector<VectorGeomHandler*> resolvingGeoData(const std::string& strData) 
  {
		std::vector<VectorGeomHandler*> geoms;
		vtzero::vector_tile tile(strData);
		for (const auto layer : tile)
		{
			for (const auto feature : layer)
			{
				VectorGeomHandler* handler = new VectorGeomHandler;
				handler->layerID = (int)layer.layer_num();
				handler->featureID = feature.has_integer_id()
					? std::to_string(feature.integer_id())
					: feature.string_id().to_string();
				handler->featureType = feature.geometry_type();
				feature.decode_geometry(*handler);
				geoms.push_back(handler);
			}
		}
        return geoms;
  }


  VectorReaderResult readBinaryVector(
      const CesiumJsonReader::ExtensionReaderContext& context,
    const std::string& data)
  {
    context;
    //VTR::VectorTileReader mvtReader;
    //std::string strData = mvtReader.readData(data, da);
    VectorReaderResult result;
    try
    {
      std::vector<VectorGeomHandler*> geoms = resolvingGeoData(data);
      
      CesiumGltf::VectorModel model;
      for (auto geom : geoms)
      {
        result.model.layers.emplace_back(geom->layer);
      }
    }
    catch (std::runtime_error ex)
    {
      result.errors.push_back("Failed to process data!");
    }
   
    return result;
  }

  // 解码数据
  void decodeData(VectorReaderResult& readVector) {
    readVector;
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
    const CesiumJsonReader::ExtensionReaderContext& context =
        this->getExtensions();
    VectorReaderResult result = readBinaryVector(context, data);
    if (options.decodeDraco) {
      //decodeData(result);
    }

    return result;
}