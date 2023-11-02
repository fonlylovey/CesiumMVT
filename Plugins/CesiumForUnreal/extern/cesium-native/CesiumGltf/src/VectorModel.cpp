#include "CesiumGltf/VectorModel.h"

#include "CesiumGltf/AccessorView.h"
#include "CesiumGltf/ExtensionKhrDracoMeshCompression.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/vec3.hpp>
#include <gsl/span>
#include <sstream>  

namespace CesiumGltf {


    void MapSourceData::setType(const std::string& strType)
    {
        if(strType == "vector")
        {
            type = SoureType::Vector;
        }
        else if (strType == "geojson")
        {
            type = SoureType::Geojson;
        }
        else if (strType == "raster")
        {
            type = SoureType::Raster;
        }
        else if (strType == "raster-dem")
        {
            type = SoureType::RasterDEM;
        }
        else if (strType == "image")
        {
            type = SoureType::Image;
        }
        else if (strType == "video")
        {
            type = SoureType::Video;
        }
    }

    void MapLayerData::setType(const std::string& strType)
    {

        if (strType == "background")
        {
            type = LayerType::Background;
        }
        else if (strType == "circle")
        {
            type = LayerType::Circle;
        }
        else if (strType == "line")
        {
            type = LayerType::Line;
        }
        else if (strType == "fill")
        {
            type = LayerType::Fill;
        }
        else if (strType == "fill-extrusion")
        {
            type = LayerType::FillExtrusion;
        }
        else if (strType == "symbol")
        {
            type = LayerType::Symbol;
        }
        else if (strType == "raster")
        {
            type = LayerType::Raster;
        }
        else if (strType == "heatmap")
        {
            type = LayerType::Heatmap;
        }
        else if (strType == "hillshade")
        {
            type = LayerType::Hillshade;
        }
    }

    void StyleData::parseStringColor(std::string strColor)
    {
        if (strColor.find("rgba") != std::string::npos)
        {
            strColor = strColor.replace(0, 5, "");
            strColor = strColor.replace(strColor.find(")"), 1, "");
            std::string strR, strG, strB, strA;
            std::stringstream ss(strColor);
            ss >> std::ws >> strR >> strG >> strB >> strA;
            opacity = std::stof(strA) * 255;
            color = glm::ivec4(std::stoi(strR), std::stoi(strG), std::stoi(strB), opacity);
        }
        else if(strColor.find("#") != std::string::npos)
        {
            color.x = std::stoi(strColor.substr(1, 2), nullptr, 16);
            color.y = std::stoi(strColor.substr(3, 2), nullptr, 16);
            color.z = std::stoi(strColor.substr(5, 2), nullptr, 16);
        }
    }

} // namespace CesiumGltf
