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

namespace
{
	//����pbfЭ�飬����������ת����WGS84��γ������
	glm::dvec3 PixelToWGS84(const glm::ivec3& pbfPos,
							int Row,
							int Col,
							const Cesium3DTilesSelection::TileMatrix& tileMatrix,
							Cesium3DTilesSelection::BoxExtent extent)
	{
		//������Ƭ�����������
		auto tilePixelX = pbfPos.x / 4096.0;
		auto tilePixelY = pbfPos.y / 4096.0;

		//�������������ڵ�ͼ�еı��� row = y, col = x ,level z
		double mapPixelX = tilePixelX * (Col + 1) / tileMatrix.MatrixWidth;
		double mapPixelY = tilePixelY * (Row + 1) / tileMatrix.MatrixHeight;

		//�����ͼ��Χ
		double LonExtent = extent.UpperCornerLon - extent.LowerCornerLon;
		double latExtent = extent.UpperCornerLat - extent.LowerCornerLat;

		glm::dvec3 pbfWorld = {
		extent.LowerCornerLon + mapPixelX * LonExtent,
		extent.LowerCornerLat + mapPixelY * latExtent, 0};
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
	Cesium3DTilesSelection::BoxExtent extent = vectorTile.getTileProvider()._boxExtent;
	int Row = pModelData->Row;
	int Col = pModelData->Col;
	int Level = pModelData->level;
	auto geoReference = ACesiumGeoreference::GetDefaultGeoreference(_pActor);
	
	//���������������Ҫ��������ת��
	TArray<FVector> posXYZArray;
	if(pModelData->layers.size() > 0)
	{
		
		FString strMessagr = "Level: " + FString::FormatAsNumber(Level) + 
							"  Row: " + FString::FormatAsNumber(Row) + 
							"  Col: " + FString::FormatAsNumber(Col);
		GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, strMessagr);

		UE_LOG(LogTemp, Error, TEXT("%s"), *strMessagr);
		
		for (const CesiumGltf::VectorLayer& layer : pModelData->layers)
		{
			for (const CesiumGltf::VectorFeature& feature : layer.features)
			{
				FBatchedLine line;
				for (int i = 0; i < feature.points.size() - 1; i++ )
				{
					glm::ivec3 pixelPosS = feature.points.at(i);
					pixelPosS.z = Level;
					//glm::ivec3 pixelPosE = feature.points.at(i + 1);
					//pixelPosE.z = Level;
					glm::dvec3 llhPosS = PixelToWGS84(pixelPosS, Row, Col, tileMatrix, extent);
					//glm::dvec3 llhPosE = PixelToWGS84(pixelPosE, Row, Col, tileMatrix, extent);
					FVector uPosS = {llhPosS.x, llhPosS.y, 100};
					//FVector uPosE = {llhPosE.x, llhPosE.y, 100};
					FVector worldPosS = geoReference->TransformLongitudeLatitudeHeightToUnreal(uPosS);
					//FVector worldPosE = geoReference->TransformLongitudeLatitudeHeightToUnreal(uPosE);
					posXYZArray.Add(worldPosS);
					
				}
			}
		}
		
		return &posXYZArray;
	}

	return &posXYZArray;;
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
		CesiumGltf::VectorModel vectorModel = VectorTile.getVectorModel();
		CesiumGltf::VectorModel* ptrModel = static_cast<CesiumGltf::VectorModel*>(pMainThreadRendererResources);
		
		if(pMainThreadRendererResources != nullptr && ptrModel->layers.size() > 0)
		{
			glm::dmat4x4 transform;
			transform;
			//ULineBatchComponent* lineBatch = UCesiumVectorComponent::CreateTest(vectorModel, _pActor, transform);
		}
		/*
		 CesiumGltf::VectorModel* vectorModel = static_cast<CesiumGltf::VectorModel*>(pMainThreadRendererResources);
		 auto ptrLoaded = static_cast<Cesium3DTilesSelection::LoadedVectorOverlayData*>(pMainThreadRendererResources);
		 TUniquePtr<Cesium3DTilesSelection::LoadedVectorOverlayData> pLoadedVector;
		 pLoadedVector.Reset(ptrLoaded);
		 if(vectorModel->layers.size() > 0)
		 {
			 glm::dmat4x4 transform;
			 //ULineBatchComponent* lineBatch = UCesiumVectorComponent::CreateTest(*vectorModel, _pActor, transform);
		 }
		 */
	}

}

void VectorResourceWorker::detachVectorInMainThread(const Cesium3DTilesSelection::Tile& tile,
	int32_t overlayTextureCoordinateID, const Cesium3DTilesSelection::VectorOverlayTile& VectorTile,
	void* pMainThreadRendererResources) noexcept
{
}

void VectorResourceWorker::free(Cesium3DTilesSelection::Tile& tile, void* pLoadThreadResult, void* pMainThreadResult) noexcept
{
	if (pLoadThreadResult != nullptr)
	{
		CesiumGltf::VectorModel* theModel = static_cast<CesiumGltf::VectorModel*>(pLoadThreadResult);
		delete theModel;
	}
	if(pMainThreadResult != nullptr)
	{
		UCesiumGltfComponent* pGltfContent = static_cast<UCesiumGltfComponent*>(pMainThreadResult);
		 CesiumLifetime::destroyComponentRecursively(pGltfContent);
	}
}

void VectorResourceWorker::freeVector(const Cesium3DTilesSelection::VectorOverlayTile& vectorTile,
	void* pLoadThreadResult, void* pMainThreadResult) noexcept
{
}
