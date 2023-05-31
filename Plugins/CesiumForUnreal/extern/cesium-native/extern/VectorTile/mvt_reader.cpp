#include <mvt_utils.hpp>
#include <vtzero/geometry.hpp>


int main(int argc, char* argv[]) {
      mvt_pbf::mvtpbf_reader reader(argv[1]);
      mvt_pbf::mvtpbf_reader::GeomVector geoms;
      reader.getVectileData(geoms);
      for(auto item : geoms)
      {
         switch (item->type())
         {
         case vtzero::GeomType::POINT :
         {
            //SETGEOMVALUE(point,vtzero::point,item);
            //std::cout << "point value : " << point.x << " " << point.y << std::endl;
         }
            break;
         case vtzero::GeomType::LINESTRING:
         {
            //SETGEOMVALUE(line,vtzero::line,item);
            //std::cout << "line size " << line._pts.size() << std::endl;
         }
            break;
         case vtzero::GeomType::POLYGON:
         {
            //SETGEOMVALUE(polygon,vtzero::polygon,item);
            //std::cout << "polygon type " << polygon._rs.size() << std::endl;
         }
            break;
         default:
            break;
         }
      }
    return 0;
}

