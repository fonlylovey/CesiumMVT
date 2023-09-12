﻿#include "CesiumVectorComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Cesium3DTilesSelection/VectorOverlayTile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "CesiumMeshSection.h"
#include "Cesium3DTilesSelection/MVTUtilities.h"
#include "Cesium3DTilesSelection/VectorOverlayTileProvider.h"
#include "CesiumGeoreference.h"
#include "MMCorner.h"

namespace
{
    //根据pbf协议，将像素坐标转换成WGS84经纬度坐标
	glm::dvec3 PixelToWGS84(glm::ivec3& pbfPos,
							int Row,
							int Col,
                            Cesium3DTilesSelection::BoxExtent tileExtent,
							Cesium3DTilesSelection::VectorOverlayTileProvider* provider)
	{
        int level = pbfPos.z;
		Cesium3DTilesSelection::TileMatrixSet tileMatrixSet = provider->_TileMatrixSetMap.at(level);
		//计算地图的变化范围
		double LonDelta = tileExtent.UpperCornerLon - tileExtent.LowerCornerLon;
		double latDelta = tileExtent.UpperCornerLat - tileExtent.LowerCornerLat;

		//当前等级的行/列的瓦片数
		int ColTileCount = tileMatrixSet.MaxTileCol - tileMatrixSet.MinTileCol + 1;
		int RowTileCount = tileMatrixSet.MaxTileRow - tileMatrixSet.MinTileRow + 1;

		//当前瓦片是第几行/第几列
		int tileRowIndex = Row - tileMatrixSet.MinTileRow;
		int tileColIndex = Col - tileMatrixSet.MinTileCol;

		//数据像素坐标
		double mapPixelX = pbfPos.x + tileColIndex * 4096.0;
		double mapPixelY = pbfPos.y + tileRowIndex * 4096.0;

		//计算瓦片坐标在地图中的比例
		//double tileXPercent = mapPixelX / (ColTileCount * 4096.0);
		//double tileYPercent = 1 - mapPixelY / (RowTileCount * 4096.0);
        double tileXPercent = pbfPos.x /  4096.0;
		double tileYPercent = 1 - pbfPos.y /  4096.0;;
		//计算像素坐标在地图中的比例 row = y, col = x ,level z
		
		glm::dvec3 pbfWorld = {
		tileExtent.LowerCornerLon + tileXPercent * LonDelta,
		tileExtent.LowerCornerLat + tileYPercent * latDelta, 0};
		return pbfWorld;
	}

    FTileModel* CreateModel(const CesiumGltf::VectorModel* pModelData, AActor* pOwner,
                            Cesium3DTilesSelection::VectorOverlayTileProvider* provider)
    {
        auto geoReference = ACesiumGeoreference::GetDefaultGeoreference(pOwner);
        int Row = pModelData->row;
        int Col = pModelData->col;
        int Level = pModelData->level;
        FString strName = FString::FormatAsNumber(Level) + "_" + FString::FormatAsNumber(Row) + "_" + FString::FormatAsNumber(Col);
        //在这个函数里面主要是做坐标转换
	    FTileModel* pTileModelData = new FTileModel;
        pTileModelData->Fill = pModelData->style.isFill;
	    pTileModelData->FillColor = FLinearColor(pModelData->style.fillColor.x, pModelData->style.fillColor.y, 
                                                 pModelData->style.fillColor.z, pModelData->style.fillColor.w);
        pTileModelData->LineWidth = pModelData->style.lineWidth;
        pTileModelData->Outline = pModelData->style.isOutline;
        pTileModelData->OutlineColor = FLinearColor(pModelData->style.outlineColor.x, pModelData->style.outlineColor.y,
                                                 pModelData->style.outlineColor.z, pModelData->style.outlineColor.w);
	    if(pModelData->layers.size() > 0)
        {
            //if (pModelData->level == 11 && pModelData->row == 668 && pModelData->col == 3421)
		    {
                pTileModelData->TileName = strName;
                Cesium3DTilesSelection::BoxExtent tileExtent;
                tileExtent.LowerCornerLon = pModelData->extentMin.x;
                tileExtent.LowerCornerLat = pModelData->extentMin.y;
                tileExtent.UpperCornerLon = pModelData->extentMax.x;
                tileExtent.UpperCornerLat = pModelData->extentMax.y;
			    //高程先随意指定
			    float height = 50;
			    for (const CesiumGltf::VectorLayer& layer : pModelData->layers)
			    {
				    int index = 0;
				    for (const CesiumGltf::VectorFeature& feature : layer.features)
				    {
                        pTileModelData->VectorType = static_cast<VectorType>(feature.featureType);
					    TArray<FVector> UEArray;
                        TArray<FVector2d> UELinesArray;
                        //三角剖分用的数组
					    std::vector<glm::dvec2> linestrings;
					    std::vector<std::vector<glm::dvec2>> holes;
					    for (const CesiumGltf::VectorGeometry& geom : feature.geometry)
					    {
						    std::vector<glm::dvec2> hole;
						    for (int i = 0; i < geom.points.size(); i++ )
						    {
                                glm::ivec3 pixelPosS = geom.points.at(i);
							    pixelPosS.z = Level;	
                                
							    glm::dvec3 llPosS = PixelToWGS84(pixelPosS, Row, Col, tileExtent, provider);
							    FVector llhPos = {llPosS.x, llPosS.y, height};

							    FVector uePos = geoReference->TransformLongitudeLatitudeHeightToUnreal(llhPos);
							    UEArray.Add(uePos);
							    if (geom.ringType == CesiumGltf::RingType::Outer)
							    {
								    linestrings.emplace_back(glm::dvec2(uePos.X, uePos.Y));
                                    UELinesArray.Add(FVector2d(uePos.X, uePos.Y));
							    }
							    else if(geom.ringType == CesiumGltf::RingType::Inner)
							    {

								    hole.emplace_back(glm::dvec2(uePos.X, uePos.Y));
							    }
							    //else
							    {
								    //UE_LOG(LogTemp, Error, TEXT("%s"), TEXT("polygon type error!"));
							    }
						
						    }
						    if(geom.ringType == CesiumGltf::RingType::Inner)
						    {
							    holes.emplace_back(hole);
						    }
					    }
					
					   
                        if(pTileModelData->Outline)
                        {
                            //UE_LOG(LogTemp, Error, TEXT("Tile Name : %s"), *pTileModelData->TileName);
                             //折线段扩展成面
                            TArray<FVector2d> polyVertex;
                            TArray<uint32> polyIndex;
                            //for (FVector2d pos : UELinesArray)
                            {
                                //FString strPoint = FString::Printf(TEXT("%0.2f"), pos.X) + "," + FString::Printf(TEXT("%0.2f"), pos.Y);
                               // UE_LOG(LogTemp, Error, TEXT("%s"), *strPoint);
                            }
                            UMMCorner::CalculatePolylineBorderPoint(UELinesArray, pTileModelData->LineWidth, polyVertex, polyIndex);
                            //面顶点坐标转换成UE坐标
					        FCesiumMeshSection lineSection;
					        for (const FVector2d& uePos : polyVertex)
					        {
						        lineSection.VertexBuffer.Add(FVector3f(uePos.X, uePos.Y, height));
					        }
                            lineSection.IndexBuffer.SetNumUninitialized(polyIndex.Num());
                            FMemory::Memcpy(lineSection.IndexBuffer.GetData(), polyIndex.GetData(), polyIndex.Num());
					        lineSection.SectionIndex = index;
					        pTileModelData->Lines.Add(lineSection);
                        }
					    
					    //面顶点坐标转换成UE坐标
					    FCesiumMeshSection section;
					    std::vector<uint32> indexs = Cesium3DTilesSelection::MVTUtilities::TriangulateLineString(linestrings, holes);
					    size_t indexSize = indexs.size()*sizeof(uint32);
					    section.IndexBuffer.SetNumUninitialized(indexs.size());
					    FMemory::Memcpy(section.IndexBuffer.GetData(), indexs.data(), indexSize);
					    section.VertexBuffer.Append(UEArray);
					    section.SectionIndex = index;
					    pTileModelData->Sections.Add(section);
					    index++;
				    }
			    }
		
		    }
        }
        return pTileModelData;
    }

}


UCesiumVectorComponent* UCesiumVectorComponent::CreateOnGameThread(
	const CesiumGltf::VectorModel* pModelData,
    Cesium3DTilesSelection::VectorOverlayTile& vectorTile,
	AActor* pOwner)
{
    auto provider = &vectorTile.getTileProvider();
    if (provider == nullptr)
    {
        return nullptr;
    }
    FTileModel* pTileModel = CreateModel(pModelData, pOwner, provider);
    if (!pTileModel->Sections.IsEmpty())
    {
        UCesiumVectorComponent* mvtComponent = NewObject<UCesiumVectorComponent>(pOwner, *pTileModel->TileName);
        mvtComponent->Level = pModelData->level;
        mvtComponent->Row = pModelData->row;
        mvtComponent->Col = pModelData->col;

	    mvtComponent->SetUsingAbsoluteLocation(true);
	    mvtComponent->SetMobility(EComponentMobility::Static);
	    mvtComponent->SetFlags(RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
    
	    mvtComponent->BuildMesh(pTileModel, pTileModel->TileName);
	    mvtComponent->SetVisibility(false, true);
        mvtComponent->RegisterComponent();
	    mvtComponent->AttachToComponent(pOwner->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);

	    return mvtComponent;
    }
	return nullptr;
}

// 设置默认值
UCesiumVectorComponent::UCesiumVectorComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
  FString str = TEXT("/CesiumForUnreal/Materials/MVT/M_VectorBase.M_VectorBase");
  BaseMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, *str));
}

UCesiumVectorComponent::~UCesiumVectorComponent()
{

}

void UCesiumVectorComponent::BuildMesh(const FTileModel* tileModel, FString strName)
{
    if(tileModel->VectorType == VectorType::Point)
    {
        buildPoints(tileModel, strName);
    }
    //绘制线矢量
    else if(tileModel->VectorType == VectorType::LineString)
    {
        buildLines(tileModel, strName);
    }
    else if (tileModel->VectorType == VectorType::Polygon)
    {
        buildPolygons(tileModel, strName);
    }
    
}

void UCesiumVectorComponent::BeginDestroy()
{
	 Super::BeginDestroy();
	 if(outlineMeshComponent != nullptr)
	 {
		outlineMeshComponent->SetVisibility(false, true);
        outlineMeshComponent->ConditionalBeginDestroy();
	 }
     if (IsValid(polygonMeshComponent))
     {
         polygonMeshComponent->SetVisibility(false, true);
         polygonMeshComponent->ConditionalBeginDestroy();
     }
     if(lineMeshComponent != nullptr)
	 {
		lineMeshComponent->SetVisibility(false, true);
        lineMeshComponent->ConditionalBeginDestroy();
	 }
}

UMaterialInterface* UCesiumVectorComponent::createMaterial(const FLinearColor& color)
{
    UMaterialInstanceDynamic* mat = UMaterialInstanceDynamic::Create(BaseMaterial, this);
    mat->SetVectorParameterValue(TEXT("FillColor"), color);
	mat->TwoSided = true;
	return mat;
}

void UCesiumVectorComponent::buildPoints(const FTileModel* tileModel, FString strName)
{

}

void UCesiumVectorComponent::buildLines(const FTileModel* tileModel, FString strName)
{
    lineMeshComponent = NewObject<ULineMeshComponent>(this, *FString("Line_" + strName));
	lineMeshComponent->Material = BaseMaterial;

	int index = 0;
	int sectionIndex = 0;
    TArray<FVector>worldPosArray;
	for (const FCesiumMeshSection& section : tileModel->Sections)
	{
		int32 count = section.IndexBuffer.Num();
		worldPosArray.Empty();
		for (int32 i = 0; i < section.VertexBuffer.Num(); i++)
		{
			//int32 theIndex = section.IndexBuffer[i];
			FVector3f pos = section.VertexBuffer[i];
			worldPosArray.Add(FVector(pos));
		}
		lineMeshComponent->CreateLine(sectionIndex, worldPosArray, tileModel->FillColor);
		sectionIndex++;
		++index;
	}
    lineMeshComponent->SetupAttachment(this);
	lineMeshComponent->RegisterComponent();
}

void UCesiumVectorComponent::buildPolygons(const FTileModel* tileModel, FString strName)
{
     //绘制面
    {
        polygonMeshComponent = NewObject<UStaticMeshComponent>(this, *FString("Poly_" + strName));
        UStaticMesh* pStaticMesh = NewObject<UStaticMesh>(polygonMeshComponent);
        polygonMeshComponent->SetStaticMesh(pStaticMesh);
	    pStaticMesh->NeverStream = true;
	
	    pStaticMesh->SetFlags(RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
	    TUniquePtr<FStaticMeshRenderData> RenderData = MakeUnique<FStaticMeshRenderData>();
	    RenderData->AllocateLODResources(1);
	
	    FStaticMeshLODResources& LODResources = RenderData->LODResources[0];
	    LODResources.bHasColorVertexData = false;
	    LODResources.bHasDepthOnlyIndices = false;
	    LODResources.bHasReversedIndices = false;
	    LODResources.bHasReversedDepthOnlyIndices = false;
	
	    FStaticMeshSectionArray& Sections = LODResources.Sections;
	    TArray<FStaticMeshBuildVertex> vertexData;
	    TArray<uint32> indexs;
	
	    int sectionCount = 0;
	    FBox3d box;
	    box.Init();
	
	    for (const FCesiumMeshSection& section : tileModel->Sections)
	    {
		    int32 count = section.IndexBuffer.Num();
		    FStaticMeshSection& staticSection = Sections.AddDefaulted_GetRef();
		    staticSection.bEnableCollision = false;
		    staticSection.MaterialIndex = 0;
		    staticSection.NumTriangles =  count / 3;
		    staticSection.FirstIndex = sectionCount;
		    staticSection.MinVertexIndex = 0;
		    staticSection.MaxVertexIndex = sectionCount + count - 1;

		    for (int32 i = 0; i < section.VertexBuffer.Num(); i++)
		    {
			
			    FStaticMeshBuildVertex vertex;
			    vertex.Position = section.VertexBuffer[i];
			    box += FVector(vertex.Position);
			    vertex.Color = FColor::Red;
			    vertex.UVs[0] = FVector2f(0, 0);
			    vertex.TangentZ = FVector3f(0, 0, -1);;
			    vertexData.Add(vertex);
		    }

		    for (int i = 0; i < count; i++)
		    {
			    int32 theIndex = section.IndexBuffer[i];
			    indexs.Add(sectionCount + theIndex);
		    }
		    sectionCount = sectionCount + count;
	    }

	    //设置包围盒
	
	    FBoxSphereBounds BoundingBoxAndSphere;
	    BoundingBoxAndSphere.Origin = FVector(box.GetCenter());
	    BoundingBoxAndSphere.BoxExtent = FVector(box.GetExtent());
	    BoundingBoxAndSphere.SphereRadius = BoundingBoxAndSphere.BoxExtent.Size();
	    RenderData->Bounds = BoundingBoxAndSphere;
	    LODResources.IndexBuffer.SetIndices(indexs, EIndexBufferStride::AutoDetect);
	    LODResources.VertexBuffers.PositionVertexBuffer.Init(vertexData);
	    LODResources.VertexBuffers.StaticMeshVertexBuffer.Init(vertexData, 1);
	
	
	    pStaticMesh->SetRenderData(MoveTemp(RenderData));
		
	    UMaterialInterface* pMaterial = createMaterial(tileModel->FillColor);
	    pStaticMesh->AddMaterial(pMaterial);
	    pStaticMesh->InitResources();
        pStaticMesh->CalculateExtendedBounds();
	    polygonMeshComponent->SetupAttachment(this);
        polygonMeshComponent->RegisterComponent();
        
    }

    //绘制轮廓线

    if (tileModel->Outline)
    {
        /*
        outlineMeshComponent = NewObject<UStaticMeshComponent>(this, *FString("Line_" + strName));
	    UStaticMesh* pStaticMesh = NewObject<UStaticMesh>(outlineMeshComponent);
        outlineMeshComponent->SetStaticMesh(pStaticMesh);
	    pStaticMesh->NeverStream = true;
	
	    pStaticMesh->SetFlags(RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
	    TUniquePtr<FStaticMeshRenderData> RenderData = MakeUnique<FStaticMeshRenderData>();
	    RenderData->AllocateLODResources(1);
	
	    FStaticMeshLODResources& LODResources = RenderData->LODResources[0];
	    LODResources.bHasColorVertexData = false;
	    LODResources.bHasDepthOnlyIndices = false;
	    LODResources.bHasReversedIndices = false;
	    LODResources.bHasReversedDepthOnlyIndices = false;
	
	    FStaticMeshSectionArray& Sections = LODResources.Sections;
	    TArray<FStaticMeshBuildVertex> vertexData;
	    TArray<uint32> indexs;
	
	    int sectionCount = 0;
	    FBox3d box;
	    box.Init();
	
	    for (const FCesiumMeshSection& section : tileModel->Lines)
	    {
		    int32 count = section.IndexBuffer.Num();
		    FStaticMeshSection& staticSection = Sections.AddDefaulted_GetRef();
		    staticSection.bEnableCollision = false;
		    staticSection.MaterialIndex = 0;
		    staticSection.NumTriangles =  count / 3;
		    staticSection.FirstIndex = sectionCount;
		    staticSection.MinVertexIndex = 0;
		    staticSection.MaxVertexIndex = sectionCount + count - 1;

		    for (int32 i = 0; i < section.VertexBuffer.Num(); i++)
		    {
			    FStaticMeshBuildVertex vertex;
			    vertex.Position = section.VertexBuffer[i];
			    box += FVector(vertex.Position);
			    vertex.Color = FColor::Red;
			    vertex.UVs[0] = FVector2f(0, 0);
			    vertex.TangentZ = FVector3f(0, 0, 1);;
			    vertexData.Add(vertex);
		    }

		    for (int i = 0; i < count; i++)
		    {
			    int32 theIndex = section.IndexBuffer[i];
			    indexs.Add(sectionCount + theIndex);
		    }
		    sectionCount = sectionCount + count;
	    }

	    //设置包围盒
	
	    FBoxSphereBounds BoundingBoxAndSphere;
	    BoundingBoxAndSphere.Origin = FVector(box.GetCenter());
	    BoundingBoxAndSphere.BoxExtent = FVector(box.GetExtent());
	    BoundingBoxAndSphere.SphereRadius = BoundingBoxAndSphere.BoxExtent.Size();
	    RenderData->Bounds = BoundingBoxAndSphere;
	    LODResources.IndexBuffer.SetIndices(indexs, EIndexBufferStride::AutoDetect);
	    LODResources.VertexBuffers.PositionVertexBuffer.Init(vertexData);
	    LODResources.VertexBuffers.StaticMeshVertexBuffer.Init(vertexData, 1);
	
	
	    pStaticMesh->SetRenderData(MoveTemp(RenderData));
		
	    UMaterialInterface* pMaterial = createMaterial(tileModel->OutlineColor);
	    pStaticMesh->AddMaterial(pMaterial);
	    pStaticMesh->InitResources();
        pStaticMesh->CalculateExtendedBounds();
        outlineMeshComponent->SetupAttachment(this);
	    outlineMeshComponent->RegisterComponent();
        */
    }
}
