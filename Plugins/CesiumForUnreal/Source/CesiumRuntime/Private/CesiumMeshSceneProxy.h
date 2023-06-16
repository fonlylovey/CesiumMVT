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

//只在渲染线程使用的结构体，要在GetDynamicMeshElements函数中返回给渲染器使用
class FMeshProxySection;

struct FCesiumMeshSection;
class UCesiumVectorComponent;

class UMaterialInterface;


class FLineMeshSceneProxy final : public FPrimitiveSceneProxy
{
public:
	FLineMeshSceneProxy(UCesiumVectorComponent* InComponent);
	virtual ~FLineMeshSceneProxy();

	/**
	 * 设置渲染的Section 在GameThread
	 * 如果以后需要实时更新数据的话，可以实现一个在Render线程的函数
	 */
	void SetSection_GameThread(TSharedPtr<FCesiumMeshSection> NewSection);

public:
	/**
	 * 重写的父类方法，以下
	 */
	//用于排序目的的类型（或子类型）特定哈希
	SIZE_T GetTypeHash() const override;

	//提供给渲染器需要的动态元素的渲染数据
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views,
										const FSceneViewFamily& ViewFamily,
										uint32 VisibilityMap,
										FMeshElementCollector& Collector) const override;

	//提供给渲染器当前绘制的数据需要开启那些效果
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const;

	//是否遮挡剔除
	virtual bool CanBeOccluded() const;

	//下面这两个函数在父类头文件说了必须重写
	virtual uint32 GetMemoryFootprint() const;
	uint32 GetAllocatedSize() const;

private:
	UCesiumVectorComponent* Component;
	FMaterialRelevance MaterialRelevance;
	FBoxSphereBounds3f LocalBounds;

	TMap<int32, TSharedPtr<FMeshProxySection>> Sections;
};