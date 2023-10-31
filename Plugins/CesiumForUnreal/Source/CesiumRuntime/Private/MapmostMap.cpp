// Fill out your copyright notice in the Description page of Project Settings.


#include "MapmostMap.h"
#include "CesiumRuntime.h"


UMapmostMap::UMapmostMap(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
    _pMapmostMap = nullptr;
    this->bAutoActivate = true;
}

UMapmostMap::~UMapmostMap()
{
   
}

void UMapmostMap::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Refresh();
}

void UMapmostMap::OnAttachmentChanged()
{

}

void UMapmostMap::BeginDestroy()
{
    Super::BeginDestroy();
}

void UMapmostMap::LoadMap()
{
    UE_LOG(LogCesium, Warning, TEXT("Create Mapmost Map..."));
    if(_pMapmostMap == nullptr)
    {
        _pMapmostMap = MakeUnique<Cesium3DTilesSelection::MapmostVectorMap>();
    }
    if (!MapStyleUrl.IsEmpty())
    {
        
        _pMapmostMap->CreateMap(TCHAR_TO_UTF8(*MapStyleUrl), getAssetAccessor(), getAsyncSystem());
    }
}

void UMapmostMap::Refresh()
{
    this->LoadMap();
}

void UMapmostMap::Activate(bool bReset)
{
    Super::Activate(bReset);
    this->LoadMap();
}

void UMapmostMap::Deactivate()
{
    Super::Deactivate();
}

