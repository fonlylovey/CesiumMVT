
#ifndef MVT_UTILS_H
#define MVT_UTILS_H

#include <vtzero/vector_tile.hpp>
#include <detail/geometry.hpp>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <types.hpp>

//Vector Tile Reader
namespace VTR
{
    /**
     * Read complete contents of a file into a string.
     *
     * The file is read in binary mode.
     *
     * @param filename The file name. Can be empty or "-" to read from STDIN.
     * @returns a string with the contents of the file.
     * @throws various exceptions if there is an error
     */
    static std::string read_file(const std::string& filename)
	{
        if (filename.empty() || (filename.size() == 1 && filename[0] == '-')) 
        {
            return std::string{std::istreambuf_iterator<char>(std::cin.rdbuf()),
                            std::istreambuf_iterator<char>()};
        }

        std::ifstream stream{filename, std::ios_base::in | std::ios_base::binary};
        if (!stream) {
            throw std::runtime_error{std::string{"Can not open file '"} + filename + "'"};
        }

        stream.exceptions(std::ifstream::failbit);

        std::string buffer{std::istreambuf_iterator<char>(stream.rdbuf()),
                        std::istreambuf_iterator<char>()};
        stream.close();

        return buffer;
    }

    /**
     * Write contents of a buffer into a file.
     *
     * The file is written in binary mode.
     *
     * @param buffer The data to be written.
     * @param filename The file name.
     * @throws various exceptions if there is an error
     */
    static void write_data_to_file(const std::string& buffer, const std::string& filename)
	{
        std::ofstream stream{filename, std::ios_base::out | std::ios_base::binary};
        if (!stream) 
        {
            throw std::runtime_error{std::string{"Can not open file '"} + filename + "'"};
        }

        stream.exceptions(std::ifstream::failbit);

        stream.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));

        stream.close();
    }

    class Result
	{

        int resultCode = 0;

    public:

        void hasWarning() noexcept
    	{
            if (resultCode < 1) 
            {
                resultCode = 1;
            }
        }

        void hasError() noexcept
    	{
            if (resultCode < 2)
            {
                resultCode = 2;
            }
        }

        void hasFatalError() noexcept
    	{
            if (resultCode < 3) {
                resultCode = 3;
            }
        }

        int return_code() const noexcept
    	{
            return resultCode;
        }

    };


    class GeometryHandler
    {
    public:
        int layerID;
        std::string featureID;
        int geoCount;
        Result result;
        vtzero::GeomType type;
        constexpr static const int dimensions = 3;
        constexpr static const unsigned int max_geometric_attributes = 0;
    };
    /*
     *  point geometry storage
     */
    class MVTPointHandler : public GeometryHandler
    {
    public:

        virtual void points_begin(uint32_t count)
        {
            
        }

        virtual void points_point(const vtzero::point_3d &pt)
        {
            mpt = pt;
            geoCount++;
        }

        virtual void points_end(vtzero::ring_type type = vtzero::ring_type::invalid)
        {

        }

        virtual const void* data()const
        {
            return &mpt;
        }
    protected:
        vtzero::point_3d mpt;
    };
    /*
     *  line geometry storage
     */
    struct MVTRing
    {
        std::vector<vtzero::point_3d> line;
        vtzero::ring_type type;
    };

    class MVTLineHandler : public GeometryHandler
    {
    public:
        virtual void linestring_begin(uint32_t count)
        {
            lineString.line.reserve(count);
        }

        virtual void linestring_point(const vtzero::point_3d&pt)
        {
            lineString.line.push_back(pt);
            geoCount++;
        }

        virtual void linestring_end(vtzero::ring_type type = vtzero::ring_type::invalid)
        {
            lineString.type = type;
        }
        virtual const void* data()const
        {
            return &lineString;
        }
    protected:
        MVTRing lineString;
    };

    /*
     *  polygon geometry storage
     */
    class MVTPolygonHandler : public GeometryHandler
    {
    public:
        virtual void ring_begin(uint32_t count)
        {
            MVTRing ring;
            ring.line.reserve(count);
            polygon.push_back(ring);
        }

        virtual void ring_point(const vtzero::point_3d &pt)
        {
            polygon.rbegin()->line.push_back(pt);
            geoCount++;
        }

        virtual void ring_end(vtzero::ring_type type = vtzero::ring_type::invalid)
        {
            polygon.rbegin()->type = type;
        }
        virtual const void* data()const
        {
            return &polygon;
        }
    protected:
        std::vector<MVTRing> polygon;

    };

    /*
     *  point geometry display
     */
    class display_point : public GeometryHandler
    {
    public:

        virtual void points_begin(uint32_t count)
        {

        }

        virtual void points_point(const vtzero::point_3d&pt)
        {
            std::cout << "      POINT(" << pt.x << ',' << pt.y << ")\n";
        }

        virtual void points_end(vtzero::ring_type type = vtzero::ring_type::invalid)
        {

        }
    };

    /*
     *  line geometry display
     */
    class display_line : public GeometryHandler
    {
        std::string output{};
    public:
        virtual void linestring_begin(uint32_t count)
        {
            output = "      LINESTRING[count=";
            output += std::to_string(count);
            output += "](";
        }

        virtual void linestring_point(const vtzero::point_3d&pt)
        {
            output += std::to_string(pt.x);
            output += ' ';
            output += std::to_string(pt.y);
            output += ',';
        }

        virtual void linestring_end(vtzero::ring_type type = vtzero::ring_type::invalid)
        {
            if (output.empty()) {
                return;
            }
            if (output.back() == ',') {
                output.resize(output.size() - 1);
            }
            output += ")\n";
            std::cout << output;
        }
    };

    /*
     *  polygon geometry display
     */
    class display_polygon : public GeometryHandler
    {
        std::string output{};
    public:
        virtual void ring_begin(uint32_t count)
        {
            output = "      RING[count=";
            output += std::to_string(count);
            output += "](";
        }

        virtual void ring_point(const vtzero::point_3d&pt)
        {
            output += std::to_string(pt.x);
            output += ' ';
            output += std::to_string(pt.y);
            output += ',';
        }

        virtual void ring_end(vtzero::ring_type type = vtzero::ring_type::invalid)
        {
            if (output.empty()) {
                return;
            }
            if (output.back() == ',') {
                output.back() = ')';
            }
            switch (type) {
                case vtzero::ring_type::outer:
                    output += "[OUTER]\n";
                    break;
                case vtzero::ring_type::inner:
                    output += "[INNER]\n";
                    break;
                default:
                    output += "[INVALID]\n";
            }
            std::cout << output;
        }

    };

    /*
     *   generate a geometry data storage object
     *
     */
    inline GeometryHandler* MakeGeometry(vtzero::GeomType type)
    {
        switch (type)
        {
        case vtzero::GeomType::POINT:
            return new MVTPointHandler;
        case vtzero::GeomType::LINESTRING:
            return new MVTLineHandler;
        case vtzero::GeomType::POLYGON:
            return new MVTPolygonHandler;
        case vtzero::GeomType::SPLINE:
            return nullptr;
        default:
            assert(NULL);
            return nullptr;
            break;
        }
    }

    inline GeometryHandler* MakeDisplayGeometry(vtzero::GeomType type)
    {
        switch (type)
        {
        case vtzero::GeomType::POINT:
            return new MVTPointHandler;
        case vtzero::GeomType::LINESTRING:
            return new MVTLineHandler;
        case vtzero::GeomType::POLYGON:
            return new MVTPolygonHandler;
        case vtzero::GeomType::SPLINE:
            return nullptr;
        default:
            assert(NULL);
            return nullptr;
            break;
        }
    }

    class mvtpbf_reader
    {
    public:
        enum class ePathType{ eFile,eData};
        mvtpbf_reader(const std::string &path,ePathType type = ePathType::eFile)
    	:mpath(path)
    	,mtype(type)
        {
        }
        /*
         * get vector tile geo datas
         */
        void getVectileData(std::vector<GeometryHandler*> &geoms)
        {
            geoms.clear();
            const auto data = (mtype == ePathType::eFile) ? read_file(mpath) : mpath;
            vtzero::vector_tile tile(data);
            bool isTile = vtzero::is_vector_tile(data);
            size_t layerCount = tile.count_layers();
            for (const auto layer : tile)
            {
                for (const auto& feature : layer)
                {
                    MVTPolygonHandler* handler = new MVTPolygonHandler;
                    handler->layerID = (int)layer.layer_num();
                    handler->featureID = feature.has_integer_id()
                            ? std::to_string(feature.integer_id()) 
                                             : feature.string_id().to_string();
                    handler->type = feature.geometry_type();
                    feature.decode_polygon_geometry(*handler);
                    geoms.push_back(handler);
                }
            }
            
            std::cout << " load geometries successfully! " << geoms.size() << " gemos have been loaded." << std::endl;
        }
    protected:
        std::string mpath;
        ePathType   mtype;
    };
}


#endif
