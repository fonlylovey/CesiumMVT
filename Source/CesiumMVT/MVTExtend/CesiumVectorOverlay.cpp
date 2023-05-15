// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumVectorOverlay.h"
#include "Async/Async.h"
#include "Cesium3DTilesSelection/RasterOverlayLoadFailureDetails.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "Cesium3DTileset.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumRuntime.h"

FCesiumVectorOverlayLoadFailure OnCesiumVectorOverlayLoadFailure{};

// Sets default values for this component's properties
UCesiumVectorOverlay::UCesiumVectorOverlay()
    : _pOverlay(nullptr), _overlaysBeingDestroyed(0) {
  this->bAutoActivate = true;

  // Set this component to be initialized when the game starts, and to be ticked
  // every frame.  You can turn these features off to improve performance if you
  // don't need them.
  PrimaryComponentTick.bCanEverTick = false;

  // ...
}

#if WITH_EDITOR
// Called when properties are changed in the editor
void UCesiumVectorOverlay::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) {
  Super::PostEditChangeProperty(PropertyChangedEvent);

  this->Refresh();
}
#endif

void UCesiumVectorOverlay::AddToTileset() {
  if (this->_pOverlay) {
    return;
  }

  Cesium3DTilesSelection::Tileset* pTileset = FindTileset();
  if (!pTileset) {
    return;
  }

  Cesium3DTilesSelection::VectorOverlayOptions options{};
  options.maximumScreenSpaceError = this->MaximumScreenSpaceError;
  options.maximumSimultaneousTileLoads = this->MaximumSimultaneousTileLoads;
  options.maximumTextureSize = this->MaximumTextureSize;
  options.subTileCacheBytes = this->SubTileCacheBytes;
  options.showCreditsOnScreen = this->ShowCreditsOnScreen;
  options.rendererOptions = &this->rendererOptions;
  options.loadErrorCallback =
      [this](const Cesium3DTilesSelection::RasterOverlayLoadFailureDetails&
                 details) {
        static_assert(
            uint8_t(ECesiumVectorOverlayLoadType::CesiumIon) ==
            uint8_t(Cesium3DTilesSelection::RasterOverlayLoadType::CesiumIon));
        static_assert(
            uint8_t(ECesiumVectorOverlayLoadType::TileProvider) ==
            uint8_t(
                Cesium3DTilesSelection::RasterOverlayLoadType::TileProvider));
        static_assert(
            uint8_t(ECesiumVectorOverlayLoadType::Unknown) ==
            uint8_t(Cesium3DTilesSelection::RasterOverlayLoadType::Unknown));

        uint8_t typeValue = uint8_t(details.type);
        assert(
            uint8_t(details.type) <=
            uint8_t(
                Cesium3DTilesSelection::RasterOverlayLoadType::TilesetJson));
        assert(this->_pTileset == details.pTileset);

        FCesiumVectorOverlayLoadFailureDetails ueDetails{};
        ueDetails.Overlay = this;
        ueDetails.Type = ECesiumVectorOverlayLoadType(typeValue);
        ueDetails.HttpStatusCode =
            details.pRequest && details.pRequest->response()
                ? details.pRequest->response()->statusCode()
                : 0;
        ueDetails.Message = UTF8_TO_TCHAR(details.message.c_str());

        // Broadcast the event from the game thread.
        // Even if we're already in the game thread, let the stack unwind.
        // Otherwise actions that destroy the Tileset will cause a deadlock.
        AsyncTask(
            ENamedThreads::GameThread,
            [ueDetails = std::move(ueDetails)]() {
                OnCesiumVectorOverlayLoadFailure.Broadcast(ueDetails);
            });
      };

  std::unique_ptr<Cesium3DTilesSelection::VectorOverlay> pOverlay =
      this->CreateOverlay(options);

  if (pOverlay) {
    this->_pOverlay = pOverlay.release();

    pTileset->getVectorOverlays().add(this->_pOverlay);

    this->OnAdd(pTileset, this->_pOverlay);
  }
}

void UCesiumVectorOverlay::RemoveFromTileset() {
  if (!this->_pOverlay) {
    return;
  }

  Cesium3DTilesSelection::Tileset* pTileset = FindTileset();
  if (!pTileset) {
    return;
  }

  // Don't allow this RasterOverlay to be fully destroyed until
  // any cesium-native RasterOverlays it created have wrapped up any async
  // operations in progress and have been fully destroyed.
  // See IsReadyForFinishDestroy.
  ++this->_overlaysBeingDestroyed;
  this->_pOverlay->getAsyncDestructionCompleteEvent(getAsyncSystem())
      .thenInMainThread([this]() { --this->_overlaysBeingDestroyed; });

  this->OnRemove(pTileset, this->_pOverlay);
  pTileset->getVectorOverlays.remove(this->_pOverlay);
  this->_pOverlay = nullptr;
}

void UCesiumVectorOverlay::Refresh() {
  this->RemoveFromTileset();
  this->AddToTileset();
}

double UCesiumVectorOverlay::GetMaximumScreenSpaceError() const {
  return this->MaximumScreenSpaceError;
}

void UCesiumVectorOverlay::SetMaximumScreenSpaceError(double Value) {
  this->MaximumScreenSpaceError = Value;
  this->Refresh();
}

int32 UCesiumVectorOverlay::GetMaximumTextureSize() const {
  return this->MaximumTextureSize;
}

void UCesiumVectorOverlay::SetMaximumTextureSize(int32 Value) {
  this->MaximumTextureSize = Value;
  this->Refresh();
}

int32 UCesiumVectorOverlay::GetMaximumSimultaneousTileLoads() const {
  return this->MaximumSimultaneousTileLoads;
}

void UCesiumVectorOverlay::SetMaximumSimultaneousTileLoads(int32 Value) {
  this->MaximumSimultaneousTileLoads = Value;

  if (this->_pOverlay) {
    this->_pOverlay->getOptions().maximumSimultaneousTileLoads = Value;
  }
}

int64 UCesiumVectorOverlay::GetSubTileCacheBytes() const {
  return this->SubTileCacheBytes;
}

void UCesiumVectorOverlay::SetSubTileCacheBytes(int64 Value) {
  this->SubTileCacheBytes = Value;

  if (this->_pOverlay) {
    this->_pOverlay->getOptions().subTileCacheBytes = Value;
  }
}

void UCesiumVectorOverlay::Activate(bool bReset) {
  Super::Activate(bReset);
  this->AddToTileset();
}

void UCesiumVectorOverlay::Deactivate() {
  Super::Deactivate();
  this->RemoveFromTileset();
}

void UCesiumVectorOverlay::OnComponentDestroyed(bool bDestroyingHierarchy) {
  this->RemoveFromTileset();
  Super::OnComponentDestroyed(bDestroyingHierarchy);
}

bool UCesiumVectorOverlay::IsReadyForFinishDestroy() {
  bool ready = Super::IsReadyForFinishDestroy();
  ready &= this->_overlaysBeingDestroyed == 0;

  if (!ready) {
    getAssetAccessor()->tick();
    getAsyncSystem().dispatchMainThreadTasks();
  }

  return ready;
}

Cesium3DTilesSelection::Tileset* UCesiumVectorOverlay::FindTileset() const {
  ACesium3DTileset* pActor = this->GetOwner<ACesium3DTileset>();
  if (!pActor) {
    return nullptr;
  }

  return pActor->GetTileset();
}
