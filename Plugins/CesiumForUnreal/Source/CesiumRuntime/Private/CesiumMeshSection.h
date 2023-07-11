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
		//����ֻ��Ҫǳ����������
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

	//��ǰ��Ƭ������
	FString TileName;
};
