// Copyright Peter Leontev

#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;

/**
/** Line section description */
struct FCesiumMeshSection
{
public:
    FCesiumMeshSection()
        : SectionLocalBox(ForceInit)
        , bSectionVisible(true)
        , SectionIndex(-1)
    {}

    void Reset()
    {
        ProcVertexBuffer.Empty();
        SectionLocalBox.Init();
        bSectionVisible = true;
        SectionIndex = -1;
    }

public:
    TArray<FVector3f> ProcVertexBuffer;
    TArray<uint32> ProcIndexBuffer;

    FBox3f SectionLocalBox;
    bool bSectionVisible;
    uint32 SectionIndex;

    UMaterialInterface* Material;
};