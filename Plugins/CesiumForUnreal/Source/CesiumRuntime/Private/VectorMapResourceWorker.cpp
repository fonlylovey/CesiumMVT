#include "VectorMapResourceWorker.h"
#include "CesiumGltfComponent.h"
#include "CreateGltfOptions.h"
#include "CesiumVectorOverlay.h"
#include "VectorRasterizer.h"

void* VectorMapResourceWorker::prepareVectorInLoadThread(CesiumGltf::VectorModel* pModel, const std::any& rendererOptions)
{
    auto ppOptions = std::any_cast<FVectorOverlayRendererOptions*>(&rendererOptions);
    check(ppOptions != nullptr && *ppOptions != nullptr);
    if (ppOptions == nullptr || *ppOptions == nullptr) 
    {
        return nullptr;
    }
    auto pOptions = *ppOptions;
    FVectorRasterizer rasterizer;
    GeometryTile* tileData = rasterizer.LoadTileModel(pModel);
    return tileData;
}

void* VectorMapResourceWorker::prepareVectorInMainThread(Cesium3DTilesSelection::VectorOverlayTile& vectorTile, void* pLoadThreadResult)
{
    if(pLoadThreadResult != nullptr)
    {
        //pLoadThreadResult就是prepareVectorInLoadThread()函数的返回值
        GeometryTile* pModelData = static_cast<GeometryTile*>(pLoadThreadResult);
        if (pModelData->GeometryList.empty())
        {
        }
        FVectorRasterizer rasterizer;
        UTexture2D* pTexture = rasterizer.Rasterizer(pModelData);
        pTexture->AddressX = TA_MAX;
        pTexture->AddressY = TA_MAX;
        delete pModelData;
        return pTexture;
    }
    return nullptr;
}

void VectorMapResourceWorker::attachVectorInMainThread(const Cesium3DTilesSelection::Tile& tile, int32_t overlayTextureCoordinateID, const Cesium3DTilesSelection::VectorOverlayTile& VectorTile, void* pMainThreadRendererResources, const glm::dvec2& translation, const glm::dvec2& scale)
{
    const Cesium3DTilesSelection::TileContent& content = tile.getContent();
      Cesium3DTilesSelection::TileRenderContent* pRenderContent = const_cast<Cesium3DTilesSelection::TileRenderContent*>(content.getRenderContent());
      UCesiumGltfComponent* pGltfContent = reinterpret_cast<UCesiumGltfComponent*>(pRenderContent->getRenderResources());
      UTexture2D* texture = static_cast<UTexture2D*>(pMainThreadRendererResources);
      auto vectorContent = pRenderContent->getVectorResources();
      auto model = VectorTile.getVectorModel();
      if(model != nullptr)
      {
          if (!model->layers.empty())
          {
              //auto texObj = StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Script/Engine.Texture2D'/Game/123.123'"));
              //texture = static_cast<UTexture2D*>(texObj);
              pGltfContent->AttachVectorTile(
                  tile,
                  VectorTile,
                  texture,
                  translation,
                  scale,
                  overlayTextureCoordinateID);
          }
      }
}

void VectorMapResourceWorker::detachVectorInMainThread(const Cesium3DTilesSelection::Tile& tile, int32_t overlayTextureCoordinateID, const Cesium3DTilesSelection::VectorOverlayTile& VectorTile, void* pMainThreadRendererResources) noexcept
{
    const Cesium3DTilesSelection::TileContent& content = tile.getContent();
    const Cesium3DTilesSelection::TileRenderContent* pRenderContent = content.getRenderContent();
    auto vectorContent = pRenderContent->getVectorResources();
    UCesiumGltfComponent* pGltfContent = reinterpret_cast<UCesiumGltfComponent*>(pRenderContent->getRenderResources());
    UTexture2D* texture = static_cast<UTexture2D*>(pMainThreadRendererResources);

    if (IsValid(pGltfContent))
    {
        pGltfContent->DetachVectorTile(
            tile,
            VectorTile,
            texture);
    }
}

void VectorMapResourceWorker::freeVector(const Cesium3DTilesSelection::VectorOverlayTile& vectorTile, void* pLoadThreadResult, void* pMainThreadResult) noexcept
{
     if (pMainThreadResult) 
       {
           UTexture* pTexture = static_cast<UTexture*>(pMainThreadResult);
           pTexture->RemoveFromRoot();
           CesiumTextureUtility::destroyTexture(pTexture);
       }
}