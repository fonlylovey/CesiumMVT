
#ifndef MVT_UTILS_H
#define MVT_UTILS_H

#if defined(_WIN32)
#define CESIUMVTR_API __declspec(dllexport)
#else
#define CESIUMVTR_API
#endif


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
    class GeometryHandler
    {
    public:
        virtual ~GeometryHandler() = default;
        constexpr static const int dimensions = 3;
        constexpr static const unsigned int max_geometric_attributes = 0;
    };

    /*
     *  point geometry storage
     */
    class MVTPointHandler : public GeometryHandler
    {
    public:
        ~MVTPointHandler() override = default;

        virtual void points_begin(uint32_t count)
        {
            
        }

        virtual void points_point(const vtzero::point_3d &pt)
        {
            mpt = pt;
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
        vtzero::ring_type type = vtzero::ring_type::invalid;
    };

    class MVTLineHandler : public GeometryHandler
    {
    public:
        ~MVTLineHandler() override = default;
        
        virtual void linestring_begin(uint32_t count)
        {
            lineString.line.reserve(count);
        }

        virtual void linestring_point(const vtzero::point_3d&pt)
        {
            lineString.line.push_back(pt);
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
        ~MVTPolygonHandler() override = default;

        virtual void ring_begin(uint32_t count)
        {
            MVTRing ring;
            ring.line.reserve(count);
            polygon.push_back(ring);
        }

        virtual void ring_point(const vtzero::point_3d &pt)
        {
            polygon.rbegin()->line.push_back(pt);
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

    enum class DataType
    {
        File,
        Byte
    };

    class CESIUMVTR_API VectorTileReader
    {
    public:
        VectorTileReader() = default;
        ~VectorTileReader();

		/*
		* 如果是二进制数据就什么都不做，如果是文件路径就读取出二进制数据
        * data: 可以是数据流和文件路径 
        * type: 说明data数据室文件还是数据流  
		*/
        std::string readVectorTile(const std::string& data, DataType type = DataType::Byte);
       
        std::string readFile(const std::string& filePath);

        void writeFile(const std::string& buffer, const std::string& filePath);

        std::string mvtDecode(const std::string& strData);

    private:
        std::string m_data;
        DataType m_type = DataType::Byte;
    };
    
}


#endif
