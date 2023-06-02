#include <VectorTileReader.hpp>

int main(int argc, char* argv[]) {
    VTR::mvtpbf_reader reader(argv[1]);
      std::vector<VTR::GeometryHandler*> geoms;
      reader.getVectileData(geoms);
      for(VTR::GeometryHandler* item : geoms)
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

