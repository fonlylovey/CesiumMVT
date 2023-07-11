#include "VectorResourceWorker.h"
#include "CesiumGltfComponent.h"
#include "CesiumVectorComponent.h"
#include "CreateGltfOptions.h"
#include "CesiumVectorOverlay.h"
#include "Cesium3DTilesSelection/VectorOverlayTile.h"
#include "Cesium3DTilesSelection/VectorOverlayTileProvider.h"
#include "math.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumMeshSection.h"
#include "GeoTransforms.h"
#include "Components/SplineComponent.h"
#include "CesiumLifetime.h"
#include "MMCorner.h"

namespace
{
	//����pbfЭ�飬����������ת����WGS84��γ������
	glm::dvec3 PixelToWGS84(glm::ivec3& pbfPos,
							int Row,
							int Col,
							const Cesium3DTilesSelection::TileMatrix& tileMatrix,
							const Cesium3DTilesSelection::TileMatrixSet& tileMatrixSet,
							Cesium3DTilesSelection::BoxExtent extent)
	{
		pbfPos.x < 0 ? pbfPos.x = 0 : pbfPos.x;
		pbfPos.x > 4096 ? pbfPos.x = 4096 : pbfPos.x;
		pbfPos.y < 0 ? pbfPos.y = 0 : pbfPos.y;
		pbfPos.y > 4096 ? pbfPos.y = 4096 : pbfPos.y;

		//�����ͼ�ı仯��Χ
		double LonDelta = extent.UpperCornerLon - extent.LowerCornerLon;
		double latDelta = extent.UpperCornerLat - extent.LowerCornerLat;

		//��ǰ�ȼ�����/�е���Ƭ��
		int ColTileCount = tileMatrixSet.MaxTileCol - tileMatrixSet.MinTileCol + 1;
		int RowTileCount = tileMatrixSet.MaxTileRow - tileMatrixSet.MinTileRow + 1;

		//��ǰ��Ƭ�ǵڼ���/�ڼ���
		int tileRowIndex = Row - tileMatrixSet.MinTileRow;
		int tileColIndex = Col - tileMatrixSet.MinTileCol;

		//������������
		double mapPixelX = pbfPos.x + tileColIndex * 4096.0;
		double mapPixelY = pbfPos.y + tileRowIndex * 4096.0;

		//������Ƭ�����ڵ�ͼ�еı���
		float tileXPercent = mapPixelX / (ColTileCount * 4096.0);
		float tileYPercent = 1 - mapPixelY / (RowTileCount * 4096.0);
		//�������������ڵ�ͼ�еı��� row = y, col = x ,level z
		
		//double mapPixelX = tilePixelX * (Col) / tileMatrix.MatrixWidth;
		//double mapPixelY = tilePixelY * (Row) / tileMatrix.MatrixHeight;

		glm::dvec3 pbfWorld = {
		extent.LowerCornerLon + tileXPercent * LonDelta,
		extent.LowerCornerLat + tileYPercent * latDelta, 0};
		return pbfWorld;
	}
}

void* VectorResourceWorker::prepareVectorInLoadThread(CesiumGltf::VectorModel& model, const std::any& rendererOptions)
{
    auto ppOptions = std::any_cast<FVectorOverlayRendererOptions*>(&rendererOptions);
    check(ppOptions != nullptr && *ppOptions != nullptr);
    if (ppOptions == nullptr || *ppOptions == nullptr) 
	{
        return nullptr;
    }
    auto pOptions = *ppOptions;
	//�����������ջ֮�� ����model�ᱻ���������Ұָ�룬������Ҫnewһ���µ�ָ��������ݺ���ʹ��
	CesiumGltf::VectorModel* theModel = new CesiumGltf::VectorModel;
	
	
	//����������첽�̣߳���������������������궼ת���ã�Ȼ��InMainThread���̺߳���ֱ�Ӵ������ 
	if(model.layers.size() > 0)
	{
		theModel->layers = model.layers;
		theModel->level = model.level;
		theModel->Row = model.Row;
		theModel->Col = model.Col;
	}
	
	//return ������������ں����߼��д���prepareVectorInMainThreadʹ��
    return theModel;
}

void* VectorResourceWorker::prepareVectorInMainThread(Cesium3DTilesSelection::VectorOverlayTile& vectorTile,
    void* pLoadThreadResult)
{
	//pLoadThreadResult����prepareVectorInLoadThread()�����ķ���ֵ
	CesiumGltf::VectorModel* pModelData = static_cast<CesiumGltf::VectorModel*>(pLoadThreadResult);

	const CesiumGeospatial::Projection& projection = vectorTile.getTileProvider().getProjection();
	Cesium3DTilesSelection::TileMatrix tileMatrix = vectorTile.getTileProvider()._TileMatrixMap[pModelData->level];
	Cesium3DTilesSelection::TileMatrixSet tileMatrixSet = vectorTile.getTileProvider()._TileMatrixSetMap[pModelData->level];
	Cesium3DTilesSelection::BoxExtent extent = vectorTile.getTileProvider()._boxExtent;
	int Row = pModelData->Row;
	int Col = pModelData->Col;
	int Level = pModelData->level;
	auto geoReference = ACesiumGeoreference::GetDefaultGeoreference(_pActor);

	//���������������Ҫ��������ת��
	FTileModel* pTileModelData = new FTileModel;
	if(pModelData->layers.size() > 0)
	{
		if (Level == 13)
		{
			FString strMessagr = "Level: " + FString::FormatAsNumber(Level) +
			"  Row: " + FString::FormatAsNumber(Row) +
			"  Col: " + FString::FormatAsNumber(Col);
			GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, strMessagr);

			UE_LOG(LogTemp, Error, TEXT("%s"), *strMessagr);
			//�߳�������ָ��
			float height = 2000;
			for (const CesiumGltf::VectorLayer& layer : pModelData->layers)
			{
				int index = 0;
				for (const CesiumGltf::VectorFeature& feature : layer.features)
				{
					TArray<FVector2D> UE2Array;
					TArray<FVector> UEArray;
					for (int i = 0; i < feature.points.size() - 1; i++ )
					{
						glm::ivec3 pixelPosS = feature.points.at(i);
						pixelPosS.z = Level;
						glm::dvec3 llPosS = PixelToWGS84(pixelPosS, Row, Col, tileMatrix, tileMatrixSet, extent);
						FVector llhPos = {llPosS.x, llPosS.y, height};
						UE2Array.Add(FVector2D(llhPos.X, llhPos.Y));

						FVector uePos = geoReference->TransformLongitudeLatitudeHeightToUnreal(llhPos);
						UEArray.Add(uePos);
					}

					//���߶���չ����
					TArray<FVector2d> polyVertex;
					TArray<uint32> polyIndex;
					//UMMCorner::PolylineCorner(UE2Array, 5.0, polyVertex, polyIndex);
					//�涥������ת����UE����
					FCesiumMeshSection section;
					for (const FVector& uePos : UEArray)
					{
						int inx = section.VertexBuffer.Add(FVector3f(uePos.X, uePos.Y, 0));
						polyIndex.Add(inx);
					}
					section.IndexBuffer = polyIndex;
					section.SectionIndex = index;
					pTileModelData->Sections.Add(section);
					index++;
				}
			}
		
			FString strName = FString::FormatAsNumber(Level) + "_" + FString::FormatAsNumber(Row) + "_" + FString::FormatAsNumber(Col);
			pTileModelData->TileName = strName;

			UCesiumVectorComponent* pVectorContent = UCesiumVectorComponent::CreateOnGameThread(pTileModelData, _pActor->GetRootComponent());
			pVectorContent->AttachToComponent(_pActor->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
			return pVectorContent;
		}
	}
	
	return nullptr;
}

void VectorResourceWorker::attachVectorInMainThread(const Cesium3DTilesSelection::Tile& tile,
	int32_t overlayTextureCoordinateID, const Cesium3DTilesSelection::VectorOverlayTile& VectorTile,
	void* pMainThreadRendererResources, const glm::dvec2& translation, const glm::dvec2& scale)
{
	const Cesium3DTilesSelection::TileContent& content = tile.getContent();
    const Cesium3DTilesSelection::TileRenderContent* pRenderContent = content.getRenderContent();
	if(pRenderContent != nullptr)
	{

		 UCesiumGltfComponent* pGltfContent = reinterpret_cast<UCesiumGltfComponent*>(
              pRenderContent->getRenderResources());
		UCesiumVectorComponent* pVectorContent = reinterpret_cast<UCesiumVectorComponent*>(pMainThreadRendererResources);
		if(pVectorContent != nullptr)
		{
		//pVectorContent->AttachToComponent(pGltfContent, FAttachmentTransformRules::KeepWorldTransform);
			//UE_LOG(LogTemp, Error, TEXT("Attach %s"), *pVectorContent->GetName());

		}
		
	}

}

void VectorResourceWorker::detachVectorInMainThread(const Cesium3DTilesSelection::Tile& tile,
	int32_t overlayTextureCoordinateID, const Cesium3DTilesSelection::VectorOverlayTile& VectorTile,
	void* pMainThreadRendererResources) noexcept
{
	const Cesium3DTilesSelection::TileContent& content = tile.getContent();
	const Cesium3DTilesSelection::TileRenderContent* pRenderContent = content.getRenderContent();
	if (pRenderContent != nullptr)
	{
		UCesiumGltfComponent* pGltfContent = reinterpret_cast<UCesiumGltfComponent*>(
			pRenderContent->getRenderResources());

		FTileModel* pTileModelData = reinterpret_cast<FTileModel*>(pMainThreadRendererResources);

		auto children = pGltfContent->GetAttachChildren();
		for (auto vectorCom : children)
		{
			UCesiumVectorComponent* pVectorContent = Cast<UCesiumVectorComponent>(vectorCom);
			if(pVectorContent)
			{
				UE_LOG(LogTemp, Error, TEXT("Detach %s"), *pVectorContent->GetName());
				//vectorCom->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
				//CesiumLifetime::destroyComponentRecursively(vectorCom);
			}
			
		}
		
	}

}

void VectorResourceWorker::free(Cesium3DTilesSelection::Tile& tile, void* pLoadThreadResult, void* pMainThreadResult) noexcept
{
	if (pLoadThreadResult != nullptr)
	{
		FTileModel* pTileModelData = reinterpret_cast<FTileModel*>(pLoadThreadResult);
		delete pTileModelData;
	}
	if(pMainThreadResult != nullptr)
	{
		FTileModel* pTileModelData = reinterpret_cast<FTileModel*>(pMainThreadResult);
		delete pTileModelData;
		//pVectorContent->RemoveFromRoot();
		//CesiumLifetime::destroyComponentRecursively(pVectorContent);
	}
	const Cesium3DTilesSelection::TileContent& content = tile.getContent();
	const Cesium3DTilesSelection::TileRenderContent* pRenderContent = content.getRenderContent();
	if (pRenderContent != nullptr)
	{
		UCesiumGltfComponent* pGltfContent = reinterpret_cast<UCesiumGltfComponent*>(
			pRenderContent->getRenderResources());
		auto children = pGltfContent->GetAttachChildren();
	}
}

void VectorResourceWorker::freeVector(const Cesium3DTilesSelection::VectorOverlayTile& vectorTile,
	void* pLoadThreadResult, void* pMainThreadResult) noexcept
{
	if (pLoadThreadResult != nullptr)
	{
		FTileModel* pTileModelData = reinterpret_cast<FTileModel*>(pMainThreadResult);
		delete pTileModelData;
	}
	if(pMainThreadResult != nullptr)
	{
		//UCesiumVectorComponent* pVectorContent = static_cast<UCesiumVectorComponent*>(pMainThreadResult);
		//auto children = pVectorContent->GetAttachChildren();
		//FTileModel* pTileModelData = reinterpret_cast<FTileModel*>(pMainThreadResult);
		//delete pTileModelData;
	}
	if(vectorTile.getRendererResources() != nullptr)
	{
		//UCesiumGltfComponent* pGltfContent = reinterpret_cast<UCesiumGltfComponent*>(
		//	vectorTile.getRendererResources());
		//UCesiumVectorComponent* pVectorContent = reinterpret_cast<UCesiumVectorComponent*>(
		//	vectorTile.getRendererResources());
		//auto children = pGltfContent->GetAttachChildren();
		//CesiumLifetime::destroyComponentRecursively(pVectorContent);
	}
} 
