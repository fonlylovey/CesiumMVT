#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "CesiumMeshSection.h"
#include <vector>
#include <opencv2/core/types.hpp>
#include <CesiumGltf/VectorModel.h>
#include "string"

struct Geometry2D
{
    std::vector<cv::Point> Points;
    bool Inner = false;
};

struct GeometryTile
{
    std::string Name = "";
    std::vector<Geometry2D*> GeometryList;
    CesiumGltf::FeatureType Type = CesiumGltf::FeatureType::UNKNOWN;
    CesiumGltf::MapLayerData Style = {};
};

/**
* Draws vector onto an Image canvas using software path-rendering.
*/
class FVectorRasterizer
{
public:
    FVectorRasterizer() = default;

    /** dtor */
    virtual ~FVectorRasterizer();

    GeometryTile* LoadTileModel( CesiumGltf::VectorModel* pModel, const TMap<FString, CesiumGltf::MapLayerData>& layers);

    UTexture2D* Rasterizer(const GeometryTile* pTileModel);

    /** draws the geometry to the image. class UTexture2D* texture = nullptr*/
    UTexture2D* Rasterizer(const CesiumGltf::VectorModel* pTileModel, const TMap<FString, CesiumGltf::MapLayerData>& layers);

private:
    int Clamp(int value, int min, int max);

    UTexture2D* RasterizerPoint(const GeometryTile* pTileModel);

    UTexture2D* RasterizerLine(const GeometryTile* pTileModel);

    UTexture2D* RasterizerPoly(const GeometryTile* pTileModel);

    UTexture2D* RasterizerNone(const GeometryTile* pTileModel);

    //光栅化一个瓦片
    UTexture2D* RasterizerTile(const CesiumGltf::VectorModel* pTileModel);

    //ue 获取的是[0, 1]之间的颜色，转换成[0， 255]
    cv::Scalar glmColorToCVColor(glm::dvec4 color);
};