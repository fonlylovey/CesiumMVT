# CesiumMVT
### 2023.10.20变更记录
1、Cesium3DTileset.cpp中attachVectorInMainThread函数整个函数体修改为
```
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
              pGltfContent->AttachVectorTile(
                  tile,
                  VectorTile,
                  texture,
                  translation,
                  scale,
                  overlayTextureCoordinateID);
          }
      }
```
2、detachVectorInMainThread函数中if (IsValid(pGltfContent) && texture != nullptr)修改为if (IsValid(pGltfContent))

3、CesiumGltfComponent.cpp中AttachVectorTile函数整个函数体替换
```
    
#if CESIUM_UNREAL_ENGINE_DOUBLE
  FVector4 translationAndScale(translation.x, translation.y, scale.x, scale.y);
#else
  FLinearColor translationAndScale(
      translation.x,
      translation.y,
      scale.x,
      scale.y);
#endif
  auto fun = [this, &vectorTile, pTexture, &translationAndScale, textureCoordinateID](
      UCesiumGltfPrimitiveComponent* pPrimitive,
      UMaterialInstanceDynamic* pMaterial,
      UCesiumMaterialUserData* pCesiumData)
  {
      if (pCesiumData) 
      {
          FString name = UTF8_TO_TCHAR(vectorTile.getOverlay().getName().c_str());

          for (int32 i = 0; i < pCesiumData->LayerNames.Num(); ++i) 
          {
            if (pCesiumData->LayerNames[i] != name) 
            {
              continue;
            }
           pMaterial->SetTextureParameterValueByInfo(
                FMaterialParameterInfo(
                    "Texture",
                    EMaterialParameterAssociation::LayerParameter,
                    i),
                pTexture);
           
            pMaterial->SetVectorParameterValueByInfo(
                FMaterialParameterInfo(
                    "TranslationScale",
                    EMaterialParameterAssociation::LayerParameter,
                    i),
                translationAndScale);
            pMaterial->SetScalarParameterValueByInfo(
                FMaterialParameterInfo(
                    "TextureCoordinateIndex",
                    EMaterialParameterAssociation::LayerParameter,
                    i),
                static_cast<float>(
                    pPrimitive->overlayTextureCoordinateIDToUVIndex
                        [textureCoordinateID]));
          }
      } 
      else
      {
          pMaterial->SetTextureParameterValue(
              createSafeName(vectorTile.getOverlay().getName(), "_Texture"), pTexture);
          pMaterial->SetVectorParameterValue(
              createSafeName(
                  vectorTile.getOverlay().getName(),
                  "_TranslationScale"),
              translationAndScale);
          pMaterial->SetScalarParameterValue(
              createSafeName(
                  vectorTile.getOverlay().getName(),
                  "_TextureCoordinateIndex"),
              static_cast<float>(pPrimitive->overlayTextureCoordinateIDToUVIndex
                                     [textureCoordinateID]));
      }
  };
  forEachPrimitiveComponent(this, fun);
```