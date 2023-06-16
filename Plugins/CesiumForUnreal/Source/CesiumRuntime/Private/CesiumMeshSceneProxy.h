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

//ֻ����Ⱦ�߳�ʹ�õĽṹ�壬Ҫ��GetDynamicMeshElements�����з��ظ���Ⱦ��ʹ��
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
	 * ������Ⱦ��Section ��GameThread
	 * ����Ժ���Ҫʵʱ�������ݵĻ�������ʵ��һ����Render�̵߳ĺ���
	 */
	void SetSection_GameThread(TSharedPtr<FCesiumMeshSection> NewSection);

public:
	/**
	 * ��д�ĸ��෽��������
	 */
	//��������Ŀ�ĵ����ͣ��������ͣ��ض���ϣ
	SIZE_T GetTypeHash() const override;

	//�ṩ����Ⱦ����Ҫ�Ķ�̬Ԫ�ص���Ⱦ����
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views,
										const FSceneViewFamily& ViewFamily,
										uint32 VisibilityMap,
										FMeshElementCollector& Collector) const override;

	//�ṩ����Ⱦ����ǰ���Ƶ�������Ҫ������ЩЧ��
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const;

	//�Ƿ��ڵ��޳�
	virtual bool CanBeOccluded() const;

	//���������������ڸ���ͷ�ļ�˵�˱�����д
	virtual uint32 GetMemoryFootprint() const;
	uint32 GetAllocatedSize() const;

private:
	UCesiumVectorComponent* Component;
	FMaterialRelevance MaterialRelevance;
	FBoxSphereBounds3f LocalBounds;

	TMap<int32, TSharedPtr<FMeshProxySection>> Sections;
};