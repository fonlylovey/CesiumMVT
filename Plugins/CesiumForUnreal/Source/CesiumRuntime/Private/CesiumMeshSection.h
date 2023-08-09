// Copyright Peter Leontev

#pragma once

#include "CoreMinimal.h"
#include "CesiumMeshSection.generated.h"

class UMaterialInterface;
/**
/** Line section description */

USTRUCT()
struct FCesiumMeshSection
{
GENERATED_BODY()
public:
    FCesiumMeshSection()
        : SectionLocalBox(ForceInit)
        , bSectionVisible(true)
        , SectionIndex(-1)
    {}

	FCesiumMeshSection(const FCesiumMeshSection& section)
	{
		VertexBuffer = section.VertexBuffer;
		IndexBuffer = section.IndexBuffer;
		SectionLocalBox = section.SectionLocalBox;
		SectionIndex = section.SectionIndex;
		bSectionVisible = section.bSectionVisible;
		//材质只需要浅拷贝就行了
		Material = section.Material;
	}

    void Reset()
    {
        VertexBuffer.Empty();
        SectionLocalBox.Init();
        bSectionVisible = true;
        SectionIndex = -1;
    }

public:
	UPROPERTY()
    TArray<FVector3f> VertexBuffer;
	UPROPERTY()
    TArray<uint32> IndexBuffer;

	UPROPERTY()
    FBox3f SectionLocalBox;
	UPROPERTY()
    bool bSectionVisible;
	UPROPERTY()
    uint32 SectionIndex;
	UPROPERTY()
    UMaterialInterface* Material;
};

struct FTileModel
{
	FTileModel() = default;
	~FTileModel()
	{
		Sections.Empty();
	}

	TArray<FCesiumMeshSection> Sections;

    TArray<FCesiumMeshSection> Lines;

    bool Fill = true;

    bool Outline = false;
    
    float LineWidth = 5.0;

    FLinearColor FillColor = FLinearColor::Yellow;

    FLinearColor OutlineColor = FLinearColor::Red;

	//当前瓦片的名称
	FString TileName;
};
