
#include "VectorTileReader.hpp"
#include <future>

void getVectileData(std::vector<VTR::GeometryHandler*>& geoms);

std::future<std::vector<VTR::GeometryHandler*>> asyncGetVectileData(const VTR::VectorTileReader& reader)
{
    return std::async([reader]()
	{
            std::vector<VTR::GeometryHandler*> geoms = reader.getGeoData();
            return geoms;
	});
}

int main(int argc, char* argv[])
{
	//VTR::mvtpbf_reader reader(argv[1]);
	VTR::VectorTileReader reader(argv[1], VTR::DataType::File);

	std::future<std::vector<VTR::GeometryHandler*>> asyncGeoms = asyncGetVectileData(reader);
	//asyncGetVectileData(reader, geoms);
	asyncGeoms.wait();
	for(VTR::GeometryHandler* item : asyncGeoms.get())
	{
	 switch (item->type)
	 {
	 case vtzero::GeomType::POINT :
	 {
		VTR::MVTPointHandler* pointGeom = static_cast<VTR::MVTPointHandler*>(item);
	    auto data = pointGeom->data();
	 }
	    break;
	 case vtzero::GeomType::LINESTRING:
	 {
	     VTR::MVTLineHandler* pointGeom = static_cast<VTR::MVTLineHandler*>(item);
	     auto data = pointGeom->data();
	 }
	    break;
	 case vtzero::GeomType::POLYGON:
	 {
	     VTR::MVTPolygonHandler* pointGeom = static_cast<VTR::MVTPolygonHandler*>(item);
	     auto data = pointGeom->data();
	 }
	    break;
	 default:
	    break;
	 }
	}
	return 0;
}

