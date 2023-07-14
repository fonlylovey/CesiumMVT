// Copyright Peter Leontev

#pragma once

#include "CoreMinimal.h"

#include "StaticMeshResources.h"
#include "RawIndexBuffer.h"
#include "LocalVertexFactory.h"
#include "RHIDefinitions.h"
#include "PrimitiveViewRelevance.h"
#include "PrimitiveSceneProxy.h"
#include "SceneManagement.h"
#include "SceneView.h"
#include "Materials/MaterialRelevance.h"



class FLineMeshProxySection;
class ULineMeshComponent;
class FLineMeshSectionUpdateData;
struct FLineMeshSection;
class UMaterialInterface;


/** Procedural mesh scene proxy */
class FLineMeshSceneProxy final : public FPrimitiveSceneProxy
{
public:
	FLineMeshSceneProxy(ULineMeshComponent* InComponent);
	virtual ~FLineMeshSceneProxy();

	SIZE_T GetTypeHash() const override;

	void AddNewSection_GameThread(TSharedPtr<FLineMeshSection> NewSection);

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const;

	virtual bool CanBeOccluded() const;
	virtual uint32 GetMemoryFootprint() const;
	uint32 GetAllocatedSize() const;

public: 
    // Accessors for ULineMeshComponent
	int32 GetNumSections() const;
	int32 GetNumPointsInSection(int32 SectionIndex) const;
    void ClearMeshSection(int32 SectionIndex);
    void ClearAllMeshSections();
    void SetMeshSectionVisible(int32 SectionIndex, bool bNewVisibility);
	void SetAllSectionVisible( bool bNewVisibility);
    bool IsMeshSectionVisible(int32 SectionIndex) const;

private:
	ULineMeshComponent* Component;
	FMaterialRelevance MaterialRelevance;
	FBoxSphereBounds3f LocalBounds;
	FCriticalSection* Mutex;
	TMap<int32, TSharedPtr<FLineMeshProxySection>> Sections;
};