## 一、cesium-native
#### 1、新增库VectorTileReader
##### 1.1 在cesium-native\extern目录中添加VectorTileReader库
##### 1.2 并且修改文件cesium-native\extern\CMakeLists.txt 
75行：
```
#mapmost add
add_subdirectory(VectorTileReader)
```

156行：
```
#mapmost add
set(CESIUM_NATIVE_EXTERN_DIR "${CMAKE_CURRENT_LIST_DIR}/" CACHE INTERNAL
    "Include directory extern"
)
```
#### 2、CesiumGltf模块添加文件 iclude/VectorModel.h、src/VectorModel.cpp
#### 3、CesiumGltfReader模块添加文件 iclude/VectorReader.h、src/VectorReader.cpp
#### 4、Cesium3DTilesSelection模块添加文件
```
MVTUtilities.h、MVTUtilities.cpp
VectorOverlay.h、VectorOverlay.cpp
VectorOverlayTile.h、VectorOverlayTile.cpp
VectorMappedTo3DTile.h、VectorMappedTo3DTile.cpp
VectorOverlayCollection.h、VectorOverlayCollection.cpp
QuadtreeVectorOverlayTileProvider.h、QuadtreeVectorOverlayTileProvider.cpp
VectorOverlayTileProvider.h、VectorOverlayTileProvider.cpp
WebMapTileServiceVectorOverlay.h、WebMapTileServiceVectorOverlay.cpp
TileMapServiceVectorOverlay.h、TileMapServiceVectorOverlay.cpp
WebMapServiceVectorOverlay.h、WebMapServiceVectorOverlay.cpp
```

#### 5、修改文件cesium-native\CMakeLists.txt  
200h行增加VectorTileReader的安装
```
#mapmost add
install(TARGETS VectorTileReader)
```

#### 6、修改文件CesiumGltfReader/CMakeLists.txt
在target_link_libraries()的PRIVATE中增加VectorTileReader的引用
```
target_link_libraries(CesiumGltfReader
    PUBLIC
        CesiumAsync
        CesiumGltf
        CesiumJsonReader
        modp_b64
        ${CESIUM_NATIVE_DRACO_LIBRARY}
    PRIVATE
        ktx_read
        webp
        webpdecoder
        VectorTileReader
)
```

#### 7、修改Cesium3DTilesSelection/TilesetContentManager
7.1、TilesetContentManager.h  
添加头文件：#include <Cesium3DTilesSelection/VectorOverlayCollection.h>

7.1.1、TilesetContentManager三个构造函数中在RasterOverlayCollection之后增加VectorOverlayCollection&& vectorCollection
```
TilesetContentManager(
      const TilesetExternals& externals,
      const TilesetOptions& tilesetOptions,
      RasterOverlayCollection&& overlayCollection,
      VectorOverlayCollection&& vectorCollection,
      std::vector<CesiumAsync::IAssetAccessor::THeader>&& requestHeaders,
      std::unique_ptr<TilesetContentLoader>&& pLoader,
      std::unique_ptr<Tile>&& pRootTile);

  TilesetContentManager(
      const TilesetExternals& externals,
      const TilesetOptions& tilesetOptions,
      RasterOverlayCollection&& overlayCollection,
      VectorOverlayCollection&& vectorCollection,
      const std::string& url);

  TilesetContentManager(
      const TilesetExternals& externals,
      const TilesetOptions& tilesetOptions,
      RasterOverlayCollection&& overlayCollection,
      VectorOverlayCollection&& vectorCollection,
      int64_t ionAssetID,
      const std::string& ionAccessToken,
      const std::string& ionAssetEndpointUrl = "https://api.cesium.com/");
```

7.1.2、增加成员函数：  
在getRasterOverlayCollection函数之后
```
const VectorOverlayCollection& getVectorOverlayCollection() const noexcept;

VectorOverlayCollection& getVectorOverlayCollection() noexcept;
```
7.1.3、增加成员变量：  
在_vectorCollection之后
```
VectorOverlayCollection _vectorCollection;
```

7.2、TilesetContentManager.cpp

7.2.1 在mapOverlaysToTile之后添加函数体
```
std::vector<CesiumGeospatial::Projection> mapVectorOverlaysToTile(
    Tile& tile,
    VectorOverlayCollection& overlays,
    const TilesetOptions& tilesetOptions) 
{
  // 清空VectorMappedTo3DTile，然后再VectorMappedTo3DTile::mapOverlayToTile中重新添加
  tile.getMappedVectorTiles().clear();
  std::vector<CesiumGeospatial::Projection> projections;
  const std::vector<CesiumUtility::IntrusivePointer<VectorOverlayTileProvider>>&
      tileProviders = overlays.getTileProviders();
  const std::vector<CesiumUtility::IntrusivePointer<VectorOverlayTileProvider>>&
      placeholders = overlays.getPlaceholderTileProviders();
  assert(tileProviders.size() == placeholders.size());

  for (size_t i = 0; i < tileProviders.size() && i < placeholders.size(); ++i) {
    VectorOverlayTileProvider& tileProvider = *tileProviders[i];
    VectorOverlayTileProvider& placeholder = *placeholders[i];

    VectorMappedTo3DTile* pMapped = VectorMappedTo3DTile::mapOverlayToTile(
        tilesetOptions.maximumScreenSpaceError,
        tileProvider,
        placeholder,
        tile,
        projections);
    if (pMapped) {
      // Try to load now, but if the mapped raster tile is a placeholder this
      // won't do anything.
      pMapped->loadThrottled();
    }
  }

  return projections;
}
```
7.2.2 修改三个构造函数的函数体  
分别在三个函数的_overlayCollection之后完成成员_vectorCollection的初始化
```
TilesetContentManager::TilesetContentManager(
    const TilesetExternals& externals,
    const TilesetOptions& tilesetOptions,
    RasterOverlayCollection&& overlayCollection,
    VectorOverlayCollection&& vectorCollection,
    int64_t ionAssetID,
    const std::string& ionAccessToken,
    const std::string& ionAssetEndpointUrl)
```
初始化列表增加：
```
_vectorCollection{std::move(vectorCollection)},
```

7.2.3 修改函数 TilesetContentManager::loadTileContent  
878行return之前添加与Raster对应的循坏
```
//Mapmost VectorOverlay fengya
for (VectorMappedTo3DTile& vectorTile : tile.getMappedVectorTiles()) {
    vectorTile.loadThrottled();
}
```
在914行Raster的函数mapOverlaysToTile()调用之后增加vecotr的调用 (由于增加了代码，大概在915行)
```
//mapmost add fengya
std::vector<CesiumGeospatial::Projection> vectorProjections =
mapVectorOverlaysToTile(tile, this->_vectorCollection, tilesetOptions);

```

7.2.4 修改函数 TilesetContentManager::unloadTileContent  
在tile.getMappedRasterTiles().clear();之后增加代码
```
//mapmost add fengya
  for (VectorMappedTo3DTile& mapped : tile.getMappedVectorTiles()) {
    mapped.detachFromTile(*this->_externals.pPrepareRendererResources, tile);
  }
  tile.getMappedVectorTiles().clear();

```

7.2.5增加新添加的函数的函数体  
在getRasterOverlayCollection的实现之后
```
const VectorOverlayCollection&
TilesetContentManager::getVectorOverlayCollection() const noexcept
{
  return _vectorCollection;
}

VectorOverlayCollection&
TilesetContentManager::getVectorOverlayCollection() noexcept
{
  return _vectorCollection;
}

```

7.2.6 修改函数 TilesetContentManager::setTileContent  
在if (result.state == TileLoadResultState::Failed)和else if (result.state == TileLoadResultState::RetryLater) 当中的tile.getMappedRasterTiles().clear();语句之后分别增加
 // mapmost add
tile.getMappedVectorTiles().clear();
```
if (result.state == TileLoadResultState::Failed) {
    tile.getMappedRasterTiles().clear();
    // mapmost add
    tile.getMappedVectorTiles().clear();
    tile.setState(TileLoadState::Failed);
  } else if (result.state == TileLoadResultState::RetryLater) {
    tile.getMappedRasterTiles().clear();
    // mapmost add
    tile.getMappedVectorTiles().clear();
    tile.setState(TileLoadState::FailedTemporarily);
  }
``` 

7.2.7 修改函数TilesetContentManager::updateDoneState  
在ifif (pRenderContent) {}语句之中for (size_t i = 0; i < rasterTiles.size(); ++i) 循坏之后增加vewctor对应的循坏
```
//mapmost add fengya
    std::vector<VectorMappedTo3DTile>& vectorTiles =
        tile.getMappedVectorTiles();
    for (size_t i = 0; i < vectorTiles.size(); ++i) {
      VectorMappedTo3DTile& mappedVectorTile = vectorTiles[i];

      VectorOverlayTile* pLoadingTile = mappedVectorTile.getLoadingTile();
      if (pLoadingTile && pLoadingTile->getState() ==
                              VectorOverlayTile::LoadState::Placeholder) {
        VectorOverlayTileProvider* pProvider =
            this->_vectorCollection.findTileProviderForOverlay(
                pLoadingTile->getOverlay());
        VectorOverlayTileProvider* pPlaceholder =
            this->_vectorCollection.findPlaceholderTileProviderForOverlay(
                pLoadingTile->getOverlay());

        // Try to replace this placeholder with real tiles.
        if (pProvider && pPlaceholder && !pProvider->isPlaceholder()) {
          // Remove the existing placeholder mapping
          vectorTiles.erase(
              vectorTiles.begin() +
              static_cast<std::vector<VectorMappedTo3DTile>::difference_type>(
                  i));
          --i;

          // Add a new mapping.
          std::vector<CesiumGeospatial::Projection> missingProjections;
          VectorMappedTo3DTile::mapOverlayToTile(
              tilesetOptions.maximumScreenSpaceError,
              *pProvider,
              *pPlaceholder,
              tile,
              missingProjections);

          if (!missingProjections.empty()) {
            // The mesh doesn't have the right texture coordinates for this
            // overlay's projection, so we need to kick it back to the unloaded
            // state to fix that.
            // In the future, we could add the ability to add the required
            // texture coordinates without starting over from scratch.
            unloadTileContent(tile);
            return;
          }
        }

        continue;
      }

      mappedVectorTile.update(*this->_externals.pPrepareRendererResources, tile);
    }
```
在tile.getMappedRasterTiles().clear();语句之后增加
```
//mapmost add fengya
tile.getMappedVectorTiles().clear();
```
7.2.7 修改函数TilesetContentManager::unloadContentLoadedState
pRenderContent->setRenderResources(nullptr);语句之后添加
```
//mapmost add fengya
pRenderContent->setVectorResources(nullptr);
```


#### 8、修改Cesium3DTilesSelection/Tile
8.1 Tile.h
8.1.1 添加头文件 #include "VectorMappedTo3DTile.h"
8.1.2 添加成员函数getMappedVectorTiles  
在getMappedRasterTiles函数之后添加
```
    /**
   * @brief Returns the verctor overlay tiles that have been mapped to this tile.
   */
  std::vector<VectorMappedTo3DTile>& getMappedVectorTiles() noexcept {
    return this->_vectorTiles;
  }

  /** @copydoc Tile::getMappedVectorTiles() */
  const std::vector<VectorMappedTo3DTile>& getMappedVectorTiles() const noexcept {
    return this->_vectorTiles;
  }

```
8.1.3 添加成员变量  
_rasterTiles之后添加成员
```
 // mapped vector overlay
  std::vector<VectorMappedTo3DTile> _vectorTiles;
```

#### 9、修改Cesium3DTilesSelection/TileContent
9.1 TileContent.h  
9.1.1 在setRenderResources()函数之后添加成员函数
```
   /**
   * @brief Get the render resources created for the glTF model of the content
   *
   * @return The render resources that is created for the glTF model
   */
  void* getVectorResources() const noexcept;

  /**
   * @brief Set the render resources created for the glTF model of the content
   *
   * @param pRenderResources The render resources that is created for the glTF
   * model
   */
  void setVectorResources(void* pVectorResources) noexcept;
```
9.1.2 添加成员变量  
在_pRenderResources之后添加成员变量  
`void* _pVectorResource;`

9.2 TileContent.cpp  
9.2.1 修改构造函数的初始化列表
```
TileRenderContent::TileRenderContent(CesiumGltf::Model&& model)
    : _model{std::move(model)},
      _pRenderResources{nullptr},
      _pVectorResource{nullptr},
      _rasterOverlayDetails{},
      _credits{},
      _lodTransitionFadePercentage{0.0f} {}
```
9.2.2 增加函数实现  
在函数setRenderResources之后增加
```
void* TileRenderContent::getVectorResources() const noexcept {
  return _pVectorResource;
}

void TileRenderContent::setVectorResources(void* pVectorResources) noexcept {
  _pVectorResource = pVectorResources;
}

```

#### 10、修改Cesium3DTilesSelection/Tileset
10.1 Tileset.h  
10.1.1 增加头文件`#include "VectorOverlayCollection.h"`
10.1.2 添加成员函数  
在getOverlays()函数之后添加成员函数
```
 /**
   * @brief Returns the {@link VectorOverlayCollection} of this tileset.
   */
  VectorOverlayCollection& getVectorOverlays() noexcept;

  /** @copydoc Tileset::getOverlays() */
  const VectorOverlayCollection& getVectorOverlays() const noexcept;

```

10.1 Tileset.cpp 
10.1.1 修改构造函数
三个个构造函数的重载中在_pTilesetContentManager初始化参数中传入VectorOverlayCollection
代码展示了一个函数的示例
```
Tileset::Tileset(
    const TilesetExternals& externals,
    std::unique_ptr<TilesetContentLoader>&& pCustomLoader,
    std::unique_ptr<Tile>&& pRootTile,
    const TilesetOptions& options)
    : _externals(externals),
      _asyncSystem(externals.asyncSystem),
      _options(options),
      _previousFrameNumber(0),
      _distances(),
      _childOcclusionProxies(),
      _pTilesetContentManager{new TilesetContentManager(
          _externals,
          _options,
          RasterOverlayCollection{_loadedTiles, externals},
          VectorOverlayCollection{_loadedTiles, externals},
          std::vector<CesiumAsync::IAssetAccessor::THeader>{},
          std::move(pCustomLoader),
          std::move(pRootTile))} {}
```
10.1.2 添加函数实现
getRasterOverlayCollection() 函数之后添加函数体
```
VectorOverlayCollection& Tileset::getVectorOverlays() noexcept {
  return _pTilesetContentManager->getVectorOverlayCollection();
}

const VectorOverlayCollection& Tileset::getVectorOverlays() const noexcept {
  return _pTilesetContentManager->getVectorOverlayCollection();
}

```

#### 11、修改Cesium3DTilesSelection/IPrepareRendererResources.h
11.1 添加前置声明 
11.1.1 CesiumGltf命名空间中添加VectorModel
```
struct ImageCesium;
struct Model;
struct VectorModel;
```
命名空间Cesium3DTilesSelection中添加
```
class Tile;
class RasterOverlayTile;
class VectorOverlayTile;
```
11.1.2 添加函数
在detachRasterInMainThread函数之后添加5个函数
```
 /**
       * @brief Prepares a Vector overlay tile.
       *
       * This method is invoked in the load thread and may modify the image.
       *
       * @param image The Vector tile image to prepare.
       * @param rendererOptions Renderer options associated with the Vector overlay tile from {@link VectorOverlayOptions::rendererOptions}.
       * @returns Arbitrary data representing the result of the load process. This
       * data is passed to {@link prepareVectorInMainThread} as the
       * `pLoadThreadResult` parameter.
       */
      virtual void* prepareVectorInLoadThread(
          CesiumGltf::VectorModel* model,
          const std::any& rendererOptions) = 0;

    /**
    * @brief Further preprares a Vector overlay tile.
    *
    * This is called after {@link prepareVectorInLoadThread}, and unlike that
    * method, this one is called from the same thread that called
    * {@link Tileset::updateView}.
    *
    * @param VectorTile The Vector tile to prepare.
    * @param pLoadThreadResult The value returned from
    * {@link prepareVectorInLoadThread}.
    * @returns Arbitrary data representing the result of the load process. Note
    * that the value returned by {@link prepareVectorInLoadThread} will _not_ be
    * automatically preserved and passed to {@link free}. If you need to free
    * that value, do it in this method before returning. If you need that value
    * later, add it to the object returned from this method.
    */
    virtual void* prepareVectorInMainThread(
        VectorOverlayTile& vectorTile,
        void* pLoadThreadResult) = 0;

    virtual void freeVector(
	    const Cesium3DTilesSelection::VectorOverlayTile& vectorTile,
	    void* pLoadThreadResult,
	    void* pMainThreadResult) noexcept = 0;

    /**
    * @brief Attaches a Vector overlay tile to a geometry tile.
    *
    * @param tile The geometry tile.
    * @param overlayTextureCoordinateID The ID of the overlay texture coordinate
    * set to use.
    * @param VectorTile The Vector overlay tile to add. The Vector tile will have
    * been previously prepared with a call to {@link prepareVectorInLoadThread}
    * followed by {@link prepareVectorInMainThread}.
    * @param pMainThreadRendererResources The renderer resources for this Vector
    * tile, as created and returned by {@link prepareVectorInMainThread}.
    * @param translation The translation to apply to the texture coordinates
    * identified by `overlayTextureCoordinateID`. The texture coordinates to use
    * to sample the Vector image are computed as `overlayTextureCoordinates *
    * scale + translation`.
    * @param scale The scale to apply to the texture coordinates identified by
    * `overlayTextureCoordinateID`. The texture coordinates to use to sample the
    * Vector image are computed as `overlayTextureCoordinates * scale +
    * translation`.
    */
    virtual void attachVectorInMainThread(
        const Tile& tile,
        int32_t overlayTextureCoordinateID,
        const VectorOverlayTile& VectorTile,
        void* pMainThreadRendererResources,
        const glm::dvec2& translation,
        const glm::dvec2& scale) = 0;

    /**
    * @brief Detaches a Vector overlay tile from a geometry tile.
    *
    * @param tile The geometry tile.
    * @param overlayTextureCoordinateID The ID of the overlay texture coordinate
    * set to which the Vector tile was previously attached.
    * @param VectorTile The Vector overlay tile to remove.
    * @param pMainThreadRendererResources The renderer resources for this Vector
    * tile, as created and returned by {@link prepareVectorInMainThread}.
    */
    virtual void detachVectorInMainThread(
    const Tile& tile,
    int32_t overlayTextureCoordinateID,
    const VectorOverlayTile& VectorTile,
    void* pMainThreadRendererResources) noexcept = 0;

```
## 二、CesiumForUnreal
#### 1.1 在CesiumRuntime/Public中新增文件
```
CesiumTileMapServiceVectorOverlay.h
CesiumVectorComponent.h
CesiumVectorOverlay.h
CesiumVectorOverlayLoadFailureDetails.h
CesiumWebMapServiceVectorOverlay.h
CesiumWebMapTileServiceVectorOverlay.h
LineMeshComponent.h
```
#### 1.2 在CesiumRuntime/Private中新增文件
```
CesiumMeshSection.h
CesiumTileMapServiceVectorOverlay.cpp
CesiumVectorComponent.cpp
CesiumVectorOverlay.cpp
CesiumWebMapServiceVectorOverlay.cpp
CesiumWebMapTileServiceVectorOverlay.cpp
LineMeshComponent.cpp
LineMeshSceneProxy.cpp
LineMeshSceneProxy.h
LineMeshSection.h
MMCorner.cpp
MMCorner.h
```
#### 1.3 修改Cesium3DTileset.cpp
1.3.1 添加头文件 
```
#include "CesiumVectorOverlay.h"
#include "Cesium3DTilesSelection/MVTUtilities.h"
#include "CesiumVectorComponent.h"
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileID.h>
```
1.3.2 修改函数ACesium3DTileset::OnConstruction()  
在for (UCesiumGltfComponent* pGltf : gltfComponents)循坏之后添加代码
```
//mapmost add fengya
    
    TArray<UCesiumVectorComponent*> vectorComponents;
    this->GetComponents<UCesiumVectorComponent>(vectorComponents);

    for (UCesiumVectorComponent* pVector : vectorComponents) 
    {
      if (IsValid(pVector)) 
      {
        pVector->SetVisibility(false, true);
      }
    }
```

1.3.3 Cesium3DTileset.cpp中的私有类UnrealResourcePreparer类中添加函数  
在detachRasterInMainThread()函数之后添加
```
  virtual void* prepareVectorInLoadThread(
          CesiumGltf::VectorModel* pModel,
          const std::any& rendererOptions)
  {
    auto ppOptions = std::any_cast<FVectorOverlayRendererOptions*>(&rendererOptions);
    check(ppOptions != nullptr && *ppOptions != nullptr);
    if (ppOptions == nullptr || *ppOptions == nullptr) 
    {
        return nullptr;
    }
    auto pOptions = *ppOptions;
    /*
    //在这个函数出栈之后 参数model会被析构，变成野指针，所以需要new一个新的指针接收数据后续使用
    CesiumGltf::VectorModel* theModel = new CesiumGltf::VectorModel;
	
	
    //这个函数是异步线程，可以在这里面把所有坐标都转换好，然后到InMainThread主线程函数直接创建组件 
    if(pModel->layers.size() > 0)
    {
	    theModel->layers = pModel->layers;
	    theModel->level = model.level;
	    theModel->Row = model.Row;
	    theModel->Col = model.Col;
    }*/
	
    //return 的这个参数会在后续逻辑中传入prepareVectorInMainThread使用
    return pModel;
  }

  virtual void* prepareVectorInMainThread(
        Cesium3DTilesSelection::VectorOverlayTile& vectorTile,
        void* pLoadThreadResult)
  {
        //pLoadThreadResult就是prepareVectorInLoadThread()函数的返回值
        CesiumGltf::VectorModel* pModelData = static_cast<CesiumGltf::VectorModel*>(pLoadThreadResult);
        UCesiumVectorComponent* pVectorContent = UCesiumVectorComponent::CreateOnGameThread(pModelData, vectorTile, _pActor);
       if (pModelData->level != pVectorContent->Level ||
           pModelData->row != pVectorContent->Row ||
           pModelData->col != pVectorContent->Col)
       {
           int aa = 0;
       }
       FString strName = FString::FormatAsNumber(pModelData->level) + "_" +
       FString::FormatAsNumber(pModelData->row) + "_" + FString::FormatAsNumber(pModelData->col);
       UE_LOG(LogTemp, Error, TEXT("create vector %s"), *strName);
       return pVectorContent;
  }

  virtual void freeVector(
	    const Cesium3DTilesSelection::VectorOverlayTile& vectorTile,
	    void* pLoadThreadResult,
	    void* pMainThreadResult) noexcept
  {


  }

  virtual void attachVectorInMainThread(
        const Cesium3DTilesSelection::Tile& tile,
        int32_t overlayTextureCoordinateID,
        const Cesium3DTilesSelection::VectorOverlayTile& VectorTile,
        void* pMainThreadRendererResources,
        const glm::dvec2& translation,
        const glm::dvec2& scale)
   {
      const Cesium3DTilesSelection::TileContent& content = tile.getContent();
      Cesium3DTilesSelection::TileRenderContent* pRenderContent = const_cast<Cesium3DTilesSelection::TileRenderContent*>(content.getRenderContent());
      
      auto tileID = Cesium3DTilesSelection::MVTUtilities::GetTileID(tile.getTileID());
      double wmtsY = glm::pow(2, tileID.level) - 1.f - (double)tileID.y;
      FString strName = FString::FormatAsNumber(tileID.level) + "_" + FString::FormatAsNumber(wmtsY) + "_" + FString::FormatAsNumber(tileID.x);
      //UE_LOG(LogTemp, Error, TEXT("attach tiles %s"), *strName);
      
      UCesiumGltfComponent* pGltfContent = reinterpret_cast<UCesiumGltfComponent*>(pRenderContent->getRenderResources());
      UCesiumVectorComponent* pVectorComponent = static_cast<UCesiumVectorComponent*>(pMainThreadRendererResources);
      if (IsValid(pVectorComponent))
      {
         
           if (tileID.level != pVectorComponent->Level &&
               wmtsY != pVectorComponent->Row &&
               tileID.x != pVectorComponent->Col)
           {
               auto thePtr = (void*)pVectorComponent;
               pRenderContent->setVectorResources(thePtr);
           }
      }
     
   }

  virtual void detachVectorInMainThread(
    const Cesium3DTilesSelection::Tile& tile,
    int32_t overlayTextureCoordinateID,
    const Cesium3DTilesSelection::VectorOverlayTile& VectorTile,
    void* pMainThreadRendererResources) noexcept
  {
      const Cesium3DTilesSelection::TileContent& content = tile.getContent();
      const Cesium3DTilesSelection::TileRenderContent* pRenderContent = content.getRenderContent();
      auto tileID = Cesium3DTilesSelection::MVTUtilities::GetTileID(tile.getTileID());
      double wmtsY = glm::pow(2, tileID.level) - 1.f - (double)tileID.y;
      FString strName = FString::FormatAsNumber(tileID.level) + "_" + FString::FormatAsNumber(wmtsY) + "_" + FString::FormatAsNumber(tileID.x);
      //UE_LOG(LogTemp, Error, TEXT("dettach tiles %s"), *strName);
       UCesiumVectorComponent* pVectorComponent = static_cast<UCesiumVectorComponent*>(pMainThreadRendererResources);
      if (IsValid(pVectorComponent))
      {
          //pVectorComponent->DetachFromParent();
          //pVectorComponent->SetVisibility(false, true);
      }
  }

```
1.3.4 修改函数ACesium3DTileset::LoadTileset()  
在for (UCesiumRasterOverlay* pOverlay : rasterOverlays)之后添加代码
```
//增加VectorOverlay的处理代码
  TArray<UCesiumVectorOverlay*> vectorOverlays;
  this->GetComponents<UCesiumVectorOverlay>(vectorOverlays);
  for (UCesiumVectorOverlay* pOverlay : vectorOverlays)
  {
      if (pOverlay->IsActive()) 
      {
          pOverlay->AddToTileset();
      }
  }

```

1.3.5 修改ACesium3DTileset::DestroyTileset()函数  
在for (UCesiumRasterOverlay* pOverlay : rasterOverlays)之后添加代码
```
//增加VectorOverlay的析构代码
  TArray<UCesiumVectorOverlay*> vectorOverlays;
  this->GetComponents<UCesiumVectorOverlay>(vectorOverlays);
  for (UCesiumVectorOverlay* pOverlay : vectorOverlays)
  {
      if (pOverlay->IsActive())
      {
          pOverlay->RemoveFromTileset();
      }
  }

```

1.3.6 修改hideTiles函数  
在 UCesiumGltfComponent* Gltf = static_cast<UCesiumGltfComponent*>();整个显隐控制代码段之后添加
```
//mapmost add fengya
    
    UCesiumVectorComponent* pVector = static_cast<UCesiumVectorComponent*>(
        pRenderContent->getVectorResources());
    if (IsValid(pVector) && pVector->IsVisible()) 
    {
      TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::SetVisibilityFalse)
      pVector->SetVisibility(false, true);
      FString strName = FString::FormatAsNumber(pVector->Level) + "_" + FString::FormatAsNumber(pVector->Row) +
        "_" + FString::FormatAsNumber(pVector->Col);
      UE_LOG(LogTemp, Error, TEXT("hide vector tile %s"), *strName);
    } 
    else
    {
      // TODO: why is this happening?
      UE_LOG(
          LogCesium,
          Verbose,
          TEXT("Tile to no longer render does not have a visible Vector"));
    }
    
```

1.3.7 修改ACesium3DTileset::showTilesToRender()函数  
在 if (!Gltf->IsVisible())语句之内的末尾
添加代码
```
    {
      TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::SetCollisionEnabled)
      Gltf->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }

    //mapmost add fengya
    
    UCesiumVectorComponent* pVector = static_cast<UCesiumVectorComponent*>(pRenderContent->getVectorResources());
    if (IsValid(pVector) && !pVector->IsVisible()) 
    {
      TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::SetVisibilityFalse)
      pVector->SetVisibility(true, true);
      FString strName = FString::FormatAsNumber(pVector->Level) + "_" + FString::FormatAsNumber(pVector->Row) + 
        "_" + FString::FormatAsNumber(pVector->Col);
      UE_LOG(LogTemp, Error, TEXT("show vector tile %s"), *strName);
    } 
    else
    {
      // TODO: why is this happening?
      UE_LOG(
          LogCesium,
          Verbose,
          TEXT("Tile to no longer render does not have a visible Vector"));
    }
    
```

<font color="#dd0000">2023.10.13 changelog<font><br/>

### 1、CesiumRuntime/Public/Cesium3DTileset.h
删除函数成员变量
```
UPROPERTY(
      Transient,
      BlueprintReadOnly,
      Category = "Cesium",
      Meta = (AllowPrivateAccess))
  UVectorSSVComponent* VectorRenderComponent = nullptr;
```

### 2、CesiumRuntime/Public/Cesium3DTileset.cpp
##### 2.1 删除头文件 #include <VectorSSVComponent.h>，添加头文件 #include "VectorRasterizer.h"
##### 2.2 删除OnConstruction(）函数中的代码

```

    //mapmost add fengya
    
    TArray<UCesiumVectorComponent*> vectorComponents;
    this->GetComponents<UCesiumVectorComponent>(vectorComponents);

    for (UCesiumVectorComponent* pVector : vectorComponents) 
    {
      if (IsValid(pVector)) 
      {
        pVector->SetVisibility(false, true);
      }
    }
    
```

##### 2.3 修改函数prepareVectorInLoadThread()全部内容为以下

```
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
```
##### 2.4 prepareVectorInMainThread()全部内容为以下
```
if(pLoadThreadResult != nullptr)
        {
                //pLoadThreadResult就是prepareVectorInLoadThread()函数的返回值
                GeometryTile* pModelData = static_cast<GeometryTile*>(pLoadThreadResult);
                if(pModelData->GeometryList.empty())
                {
                    //return nullptr;
                }
                FVectorRasterizer rasterizer;
                UTexture2D* pTexture = rasterizer.Rasterizer(pModelData);
                delete pModelData;
                return pTexture;
        }
        return nullptr;
```
##### 2.5 freeVector()全部内容为以下
```
    if (pMainThreadResult) 
    {
        UTexture* pTexture = static_cast<UTexture*>(pMainThreadResult);
        pTexture->RemoveFromRoot();
        CesiumTextureUtility::destroyTexture(pTexture);
    }
```
##### 2.6 attachVectorInMainThread()全部内容为以下
```
      const Cesium3DTilesSelection::TileContent& content = tile.getContent();
      Cesium3DTilesSelection::TileRenderContent* pRenderContent = const_cast<Cesium3DTilesSelection::TileRenderContent*>(content.getRenderContent());
      UCesiumGltfComponent* pGltfContent = reinterpret_cast<UCesiumGltfComponent*>(pRenderContent->getRenderResources());
      UTexture2D* texture = static_cast<UTexture2D*>(pMainThreadRendererResources);
      auto vectorContent = pRenderContent->getVectorResources();
      auto model = VectorTile.getVectorModel();
      auto tileID = Cesium3DTilesSelection::MVTUtilities::GetWTMSTileID(tile.getTileID());

      if(IsValid(texture) && model != nullptr)
      {
          bool isCurrent = model->row == tileID.y && model->col == tileID.x;
          if (IsValid(pGltfContent) && isCurrent)
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
      else
      {
          if (IsValid(pGltfContent))
          {
              pGltfContent->AttachVectorTile(
                  tile,
                  VectorTile,
                  nullptr,
                  translation,
                  scale,
                  overlayTextureCoordinateID);
          }
      }
```
##### 2.7 detachVectorInMainThread()全部内容为以下
```
const Cesium3DTilesSelection::TileContent& content = tile.getContent();
      const Cesium3DTilesSelection::TileRenderContent* pRenderContent = content.getRenderContent();
      auto vectorContent = pRenderContent->getVectorResources();
      UCesiumGltfComponent* pGltfContent = reinterpret_cast<UCesiumGltfComponent*>(pRenderContent->getRenderResources());
      UTexture2D* texture = static_cast<UTexture2D*>(pMainThreadRendererResources);
      if (IsValid(pGltfContent) && texture != nullptr)
      {
          pGltfContent->DetachVectorTile(
            tile,
            VectorTile,
            texture);
      }
```
##### 2.8 删除LoadTileset()内代码
```
  VectorRenderComponent = NewObject<UVectorSSVComponent>(this);
  VectorRenderComponent->RegisterComponent();
```
##### 2.8 hideTiles()内代码
```
//mapmost add fengya
    
    UCesiumVectorComponent* pVector = static_cast<UCesiumVectorComponent*>(
        pRenderContent->getVectorResources());
    if (IsValid(pVector) && pVector->IsVisible()) 
    {
      TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::SetVisibilityFalse)
      pVector->SetVisibility(false, true);
      FString strName = FString::FormatAsNumber(pVector->Level) + "_" + FString::FormatAsNumber(pVector->Row) +
        "_" + FString::FormatAsNumber(pVector->Col);
      UE_LOG(LogTemp, Error, TEXT("hide vector tile %s"), *strName);
    } 
    else
    {
      // TODO: why is this happening?
      UE_LOG(
          LogCesium,
          Verbose,
          TEXT("Tile to no longer render does not have a visible Vector"));
    }
```

### 3、CesiumRuntime/Public/CesiumRuntime.Build.cs
文件 189行左右添加代码
```
  PrivateDependencyModuleNames.Add("OpenCV");
  PrivateDependencyModuleNames.Add("OpenCVHelper");
```
### CesiumForUnreal/CesiumForUnreal.uplugin
"Plugins":增加插件opencv
```
    {
        "Name": "OpenCV",
        "Enabled": true
    }
```

### 4、CesiumRuntime/Private/CesiumGltfComponent.h
##### 4.1 Cesium3DTilesSelection命名空间增加前置声明
class VectorOverlayTile;

##### 4.2 DetachRasterTile() 函数后面增加两个函数声明
```
  //mapmost add by fengya
  void AttachVectorTile(
      const Cesium3DTilesSelection::Tile& Tile,
      const Cesium3DTilesSelection::VectorOverlayTile& VectorTile,
      UTexture2D* Texture,
      const glm::dvec2& Translation,
      const glm::dvec2& Scale,
      int32_t TextureCoordinateID);

  void DetachVectorTile(
      const Cesium3DTilesSelection::Tile& Tile,
      const Cesium3DTilesSelection::VectorOverlayTile& VectorTile,
      UTexture2D* Texture);

```

### 5、CesiumRuntime/Private/CesiumGltfComponent.cpp
##### 5.1 增加头文件
#include <Cesium3DTilesSelection/VectorOverlayTile.h>
#include <Cesium3DTilesSelection/VectorOverlay.h>

##### 5.2 DetachRasterTile()函数后面增加函数实现
```
void UCesiumGltfComponent::AttachVectorTile(const Cesium3DTilesSelection::Tile& tile, const Cesium3DTilesSelection::VectorOverlayTile& vectorTile, UTexture2D* pTexture, const glm::dvec2& translation, const glm::dvec2& scale, int32_t textureCoordinateID)
{
    
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
            if(pTexture == nullptr)
            {
                pMaterial->SetTextureParameterValueByInfo(
                FMaterialParameterInfo(
                    "Texture",
                    EMaterialParameterAssociation::LayerParameter,
                    i),
                this->Transparent1x1);
            }
            else
            {
                 pMaterial->SetTextureParameterValueByInfo(
                FMaterialParameterInfo(
                    "Texture",
                    EMaterialParameterAssociation::LayerParameter,
                    i),
                pTexture);
            }
           
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
          if (pTexture == nullptr)
          {
              pMaterial->SetTextureParameterValue(createSafeName(vectorTile.getOverlay().getName(), "_Texture"),
                                                  this->Transparent1x1);
          }
          else
          {
              pMaterial->SetTextureParameterValue(createSafeName(vectorTile.getOverlay().getName(), "_Texture"),
                                                  pTexture);
          }
          
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
}

void UCesiumGltfComponent::DetachVectorTile(const Cesium3DTilesSelection::Tile& tile, const Cesium3DTilesSelection::VectorOverlayTile& vectorTile, UTexture2D* pTexture)
{
    auto fun = [this, &vectorTile, pTexture](
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
                    this->Transparent1x1);
            }
        }
        else
        {
            pMaterial->SetTextureParameterValue(
                createSafeName(vectorTile.getOverlay().getName(), "_Texture"),
                this->Transparent1x1);
        }
    };

    forEachPrimitiveComponent(this, fun);
}

```


## 6 删除文件 PCesiumRuntime/Private/GeometryUtility.h和GeometryUtility.cpp

## 7 增加文件
Plugins/CesiumForUnreal/Source/CesiumRuntime/Public/CesiumVectorGeometryComponent.h
Plugins/CesiumForUnreal/Source/CesiumRuntime/Public/CesiumVectorGeometryComponent.cpp
Plugins/CesiumForUnreal/Source/CesiumRuntime/Public/CesiumVectorRasterizerComponent.h
Plugins/CesiumForUnreal/Source/CesiumRuntime/Public/CesiumVectorRasterizerComponent.cpp


