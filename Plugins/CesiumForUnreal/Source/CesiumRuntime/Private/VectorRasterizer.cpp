
#include "VectorRasterizer.h"
#if PLATFORM_WINDOWS

#undef check
#include "opencv2/core.hpp"
#include <opencv2/core/types.hpp>
#include <opencv2/imgproc.hpp> 
#endif//PLATFORM_WINDOWS
#include "OpenCVHelper.h"
#include "Engine/Texture2D.h"
#include <CesiumGltf/VectorModel.h>


FVectorRasterizer::~FVectorRasterizer()
{

}


GeometryTile* FVectorRasterizer::LoadTileModel(CesiumGltf::VectorModel* pModel)
{
    int tileWidth = 4096;
    GeometryTile* tileData = new GeometryTile;
    tileData->Name = std::to_string(pModel->level) + "_" + std::to_string(pModel->col) + "_" + std::to_string(pModel->row);
    if(pModel->layers.size() > 0)
    {
        tileData->Style = pModel->style;
       
        for (const CesiumGltf::VectorLayer& layer : pModel->layers)
		{
			for (const CesiumGltf::VectorFeature& feature : layer.features)
			{
                tileData->Type = feature.featureType;
                if(feature.featureType == CesiumGltf::FeatureType::Point)
                {
                    for (const CesiumGltf::VectorGeometry& geom : feature.geometry)
					{
                        Geometry2D* geometry = new Geometry2D;
						for (int i = 0; i < geom.points.size(); i++ )
						{
                            glm::ivec3 pixelPosS = geom.points.at(i);
                            int pixX = Clamp(pixelPosS.x, 0, tileWidth) / 16;
                            int pixY = Clamp(pixelPosS.y, 0, tileWidth) / 16;
                            geometry->Points.emplace_back(cv::Point(pixX, pixY));
						}
                        tileData->GeometryList.push_back(geometry);
                    }
                }
                else if (feature.featureType == CesiumGltf::FeatureType::LineString)
                {
                    for (const CesiumGltf::VectorGeometry& geom : feature.geometry)
					{
                        Geometry2D* geometry = new Geometry2D;
						for (int i = 0; i < geom.points.size(); i++ )
						{
                            glm::ivec3 pixelPosS = geom.points.at(i);
                            int pixX = Clamp(pixelPosS.x, 0, tileWidth) / 16;
                            int pixY = Clamp(pixelPosS.y, 0, tileWidth) / 16;
                            geometry->Points.emplace_back(cv::Point(pixX, pixY));
						}
                        tileData->GeometryList.push_back(geometry);
                    }
                } 
                else  if (feature.featureType == CesiumGltf::FeatureType::Polygon)
                {
					for (const CesiumGltf::VectorGeometry& geom : feature.geometry)
					{
                        Geometry2D* geometry = new Geometry2D;
                        geometry->Inner = geom.ringType == CesiumGltf::RingType::Inner ? true : false;
                        for (int i = 0; i < geom.points.size(); i++ )
						{
                            glm::ivec3 pixelPosS = geom.points.at(i);
                            int pixX = Clamp(pixelPosS.x, 0, tileWidth) / 16;
                            int pixY = Clamp(pixelPosS.y, 0, tileWidth) / 16;
                            geometry->Points.emplace_back(cv::Point(pixX, pixY));
						}
                        tileData->GeometryList.push_back(geometry);
					}
                }
			}
		}
    }
    return tileData;
}

UTexture2D* FVectorRasterizer::Rasterizer(const GeometryTile* pTileModel)
{
    UTexture2D* texture = nullptr;
    switch (pTileModel->Type)
    {
        case CesiumGltf::FeatureType::Point :
            texture = RasterizerPoint(pTileModel);
            break;
        case CesiumGltf::FeatureType::LineString :
            texture = RasterizerLine(pTileModel);
            break;
        case CesiumGltf::FeatureType::Polygon :
            texture = RasterizerPoly(pTileModel);
            break;
        default :
            texture = RasterizerNone(pTileModel);
            break;
    }
    return texture;
}

int FVectorRasterizer::Clamp(int value, int min, int max)
{
    value = value > max ? max : value;
    value = value < min ? min : value;
    return value;
}

UTexture2D* FVectorRasterizer::RasterizerPoint(const GeometryTile* pTileModel)
{
    //必须使用8字节4通道的图像
    cv::Mat image =cv:: Mat(256, 256, CV_8UC4, cv::Scalar(0, 0, 0, 0));
    
    UTexture2D* tex = FOpenCVHelper::TextureFromCvMat(image);
    tex->AddToRoot();
    return tex;
}

UTexture2D* FVectorRasterizer::RasterizerLine(const GeometryTile* pTileModel)
{
    cv::Mat image =cv:: Mat(256, 256, CV_8UC4, cv::Scalar(0, 0, 0, 0));
    //所有Geometry的顶点数组
    std::vector<std::vector<cv::Point>> geomList;
    for (Geometry2D* geomPtr : pTileModel->GeometryList)
    {
        geomList.emplace_back(geomPtr->Points);
    }
    cv::Scalar color = glmColorToCVColor(pTileModel->Style.fillColor);
    cv::polylines(image, geomList, false, color, pTileModel->Style.lineWidth);
    UTexture2D* tex = FOpenCVHelper::TextureFromCvMat(image);
    tex->AddToRoot();
    return tex;
}

UTexture2D* FVectorRasterizer::RasterizerPoly(const GeometryTile* pTileModel)
{
    cv::Mat image =cv:: Mat(256, 256, CV_8UC4, cv::Scalar(0, 0, 0, 0));

    std::vector<cv::Vec4i> hierarchy;
    std::vector<std::vector<cv::Point>> geomList;
    
    for (Geometry2D* geomPtr : pTileModel->GeometryList)
    {
        geomList.emplace_back(geomPtr->Points);
        if(geomPtr->Inner)
        {
            // 内轮廓，没有父轮廓，没有下一个轮廓，没有第一个子轮
            hierarchy.push_back(cv::Vec4i(-1, -1, -1, 0));
        }
        else
        {
            // 外轮廓，没有父轮廓，没有下一个轮廓，有第一个子轮廓，没有前一个轮廓
            hierarchy.push_back(cv::Vec4i(-1, -1, 1, -1));
        }
    }

    cv::Scalar fillColor = glmColorToCVColor(pTileModel->Style.fillColor);
    cv::Scalar outlineColor = glmColorToCVColor(pTileModel->Style.outlineColor);
    cv::fillPoly(image, geomList, fillColor, cv::LineTypes::LINE_8, 0);
    cv::drawContours(image, geomList, -1, fillColor, 0, cv::LineTypes::LINE_AA, hierarchy, 0);
    cv::putText(image, pTileModel->Name, cv::Point(20, 125), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0, 255), 2);
    if(pTileModel->Style.isOutline)
    {
        cv::polylines(image, geomList, true, outlineColor, pTileModel->Style.lineWidth, cv::LineTypes::LINE_AA);
    }
   
    UTexture2D* tex = FOpenCVHelper::TextureFromCvMat(image);
    tex->AddToRoot();
    return tex;
}

UTexture2D* FVectorRasterizer::RasterizerNone(const GeometryTile* pTileModel)
{
    //必须使用8字节4通道的图像
    cv::Mat image =cv:: Mat(256, 256, CV_8UC4, cv::Scalar(0, 0, 0, 0));
    
    UTexture2D* tex = FOpenCVHelper::TextureFromCvMat(image);
    cv::putText(image, pTileModel->Name, cv::Point(20, 125), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0, 255), 2);
    tex->AddToRoot();
    return tex;
}

cv::Scalar FVectorRasterizer::glmColorToCVColor(glm::dvec4 color)
{
    int r = color.x * 255;
    int g = color.y * 255;
    int b = color.z * 255;
    int a = color.w * 255;
    cv::Scalar cvColor(b, g, r, a);
    return cvColor;
}
