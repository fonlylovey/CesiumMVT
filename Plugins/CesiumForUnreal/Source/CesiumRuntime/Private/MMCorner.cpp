#include "MMCorner.h"


/**
 * @brief 计算两直线的相交点坐标
 * @param PointA 直线的坐标位置A
 * @param PointB 直线的坐标位置B
 * @param PointC 另一直线的坐标位置C
 * @param PointD 另一直线的坐标位置D
 */
void MMCorner::CalculateIntersectionCoordinate(const FVector2d& PointA, FVector2d& PointB, FVector2d& PointC,
                                               const FVector2d& PointD)
{
	double k1 = (PointB.Y - PointA.Y) / (PointB.X - PointA.X);
	double b1 = PointA.Y - k1 * PointA.X;

	double k2 = (PointD.Y - PointC.Y) / (PointD.X - PointC.X);
	double b2 = PointC.Y - k2 * PointC.X;
	if (PointB == PointC)
	{
		return;
	}
	if (abs(PointB.X - PointA.X) < 1e-5)
	{
		PointB.Y = k2 * PointB.X + b2;
		PointC.X = PointB.X;
		PointC.Y = PointB.Y;
	}
	else if (abs(PointD.X - PointC.X) < 1e-5)
	{
		PointC.Y = k1 * PointC.X + b1;
		PointB.X = PointC.X;
		PointB.Y = PointC.Y;
	}
	else
	{
		if (abs(k1 - k2) >= 1e-5)
		{
			double x_num = (b2 - b1) / (k1 - k2);
			double y_num = k1 * x_num + b1;
			PointB.X = x_num;
			PointC.X = x_num;
			PointB.Y = y_num;
			PointC.Y = y_num;
		};
	}
}

/**
 * @brief 计算中心线边缘点集合及索引
 * @param startPoint 中心线起始点
 * @param endPoint 中心线终止点
 * @param thickness  中心线宽度
 * @param borderVertexsList 中心线边缘点集合
 * @param vectexsIndex  中心线边缘点开始索引
 * @param borderVertexsIndex 中心线边缘点索引集合
 */
void MMCorner::CalculateCenterlineBorderPoints(FVector2d startPoint, FVector2d endPoint, double thickness,
                                               TArray<FVector2d>& borderVertexsList, uint32& vectexsIndex,
                                               TArray<uint32>& borderVertexsIndex)
{
	FVector2d normalAB = FVector2d(startPoint.Y - endPoint.Y,
	                               endPoint.X - startPoint.X);
	if (normalAB.Size() > 0)
	{
		FVector2d t = normalAB.GetSafeNormal() * 0.5 * thickness;
		borderVertexsList.Add(endPoint + t);
		borderVertexsList.Add(endPoint);
		borderVertexsList.Add(endPoint - t);
		borderVertexsList.Add(startPoint - t);
		borderVertexsList.Add(startPoint);
		borderVertexsList.Add(startPoint + t);

		borderVertexsIndex.Add(vectexsIndex);
		borderVertexsIndex.Add(vectexsIndex + 1);
		borderVertexsIndex.Add(vectexsIndex + 4);
		borderVertexsIndex.Add(vectexsIndex);
		borderVertexsIndex.Add(vectexsIndex + 4);
		borderVertexsIndex.Add(vectexsIndex + 5);
		
		borderVertexsIndex.Add(vectexsIndex + 1);
		borderVertexsIndex.Add(vectexsIndex + 2);
		borderVertexsIndex.Add(vectexsIndex + 3);
		borderVertexsIndex.Add(vectexsIndex + 1);
		borderVertexsIndex.Add(vectexsIndex + 3);
		borderVertexsIndex.Add(vectexsIndex + 4);
		
		vectexsIndex += 6;
	}
}



/**
 * @brief 计算边缘点角点
 * @param MultiLineBorderPoints 多线段线相交于一点的线段集合
 * @param borderList 面带边缘点集合
 */
void MMCorner::MultiLineSegmentsCollinearCorner(TArray<uint32> MultiLineBorderPoints, TArray<FVector2d>& borderList)
{
	if (MultiLineBorderPoints.Num() < 7)
	{
		return;
	}
	int RetangleNum = MultiLineBorderPoints.Num() / 6;
	for (int i = 0; i < RetangleNum; i++)
	{
		if (i + 1 >= RetangleNum)
		{
			CalculateIntersectionCoordinate(borderList[MultiLineBorderPoints[6 * i + 2]],
			                                borderList[MultiLineBorderPoints[6 * i + 3]],
			                                borderList[MultiLineBorderPoints[5]], borderList[MultiLineBorderPoints[0]]);
		}
		else
		{
			CalculateIntersectionCoordinate(borderList[MultiLineBorderPoints[6 * i + 2]],
			                                borderList[MultiLineBorderPoints[6 * i + 3]],
			                                borderList[MultiLineBorderPoints[6 * i + 11]],
			                                borderList[MultiLineBorderPoints[6 * i + 6]]);
		}
	}
}

/**
 * @brief 查找对应点在集合中的索引
 * @param point 待查找点
 * @param wallConstructionArray 墙中心点集合
 * @return 
 */
int MMCorner::FindIndex(FVector2d point, TArray<MMCornerNode> wallConstructionArray)
{
	for (int i = 0; i < wallConstructionArray.Num(); i++)
	{
		if (wallConstructionArray[i].centerPoint == point)
		{
			return i;
		}
	}
	return wallConstructionArray.Num();
};


/**
 * @brief 计算两个向量的叉积
 * @param v1 向量1
 * @param v2 向量2
 * @return 
 */
double MMCorner::CrossProduct(const FVector2d& v1, const FVector2d& v2)
{
	return v1.X * v2.Y - v1.Y * v2.X;
}

//计算极坐标角度
double MMCorner::PolarAngle(FVector2d cengterPoint, FVector2d q2)
{
	FVector2d p2 = FVector2d(q2.X - cengterPoint.X, cengterPoint.Y - q2.Y);
	if (p2.Y > 0)
	{
		double value = sqrt(p2.X * p2.X + p2.Y * p2.Y);
		return acos(p2.X / value);
	}
	if (p2.Y == 0)
	{
		if (p2.X > 0)
		{
			return 0;
		}
		return PI;
	}

	double value = sqrt(p2.X * p2.X + p2.Y * p2.Y);
	return 2 * PI - acos(p2.X / value);
};
//插入排序
void MMCorner::insertionSort(TArray<PointStruct>& PointList)
{
	PointStruct key;
	for (int i = 0; i < PointList.Num(); ++i)
	{
		int j = i - 1;
		key.angle = PointList[i].angle;
		key.borderIDs = PointList[i].borderIDs;
		key.pointID = PointList[i].pointID;
		while (j >= 0 && PointList[j].angle > key.angle)
		{
			PointList[j + 1].borderIDs = PointList[j].borderIDs;
			PointList[j + 1].angle = PointList[j].angle;
			PointList[j + 1].pointID = PointList[j].pointID;
			--j;
		}
		PointList[j + 1].angle = key.angle;
		PointList[j + 1].borderIDs = key.borderIDs;
		PointList[j + 1].pointID = key.pointID;
	}
}

//排序
void MMCorner::SortInsectionPoint(TArray<MMCornerNode>& wallConstructionArray)
{
	for (int k = 0; k < wallConstructionArray.Num(); k++)
	{
		TArray<FVector2d> Points = wallConstructionArray[k].InsectionPointsID;
		TArray<PointStruct> PointList;
		if (Points.Num() > 1)
		{
			for (int i = 0; i < Points.Num(); i++)
			{
				PointStruct PointStruct;
				PointStruct.pointID = Points[i];
				PointStruct.borderIDs = {
					wallConstructionArray[k].borderIDs[6 * i],
					wallConstructionArray[k].borderIDs[6 * i + 1],
					wallConstructionArray[k].borderIDs[6 * i + 2],
					wallConstructionArray[k].borderIDs[6 * i + 3],
					wallConstructionArray[k].borderIDs[6 * i + 4],
					wallConstructionArray[k].borderIDs[6 * i + 5]
				};
				PointStruct.angle = PolarAngle(wallConstructionArray[k].centerPoint, Points[i]);
				PointList.Emplace(PointStruct);
			}

			insertionSort(PointList);

			wallConstructionArray[k].InsectionPointsID.Empty();
			wallConstructionArray[k].borderIDs.Empty();
			for (int j = 0; j < PointList.Num(); j++)
			{
				wallConstructionArray[k].InsectionPointsID.Emplace(PointList[j].pointID);
				for (auto borderID : PointList[j].borderIDs)
				{
					wallConstructionArray[k].borderIDs.Emplace(borderID);
				}
			}
		}
		Points.Empty();
		PointList.Empty();
	}
};
//墙体数据结构化
void MMCorner::WallConstruction(TArray<MMCenterLine> wallArray, TArray<MMCornerNode>& wallConstructionArray)
{
	for (int i = 0; i < wallArray.Num(); i++)
	{
		if (wallConstructionArray.Num() == 0)
		{
			MMCornerNode cornerNode_start;

			cornerNode_start.centerPoint = wallArray[i].startPoint;
			cornerNode_start.InsectionPointsID.Emplace(wallArray[i].endPoint);
			for (auto borderID : wallArray[i].borderIDs)
			{
				cornerNode_start.borderIDs.Emplace(borderID);
			}
			wallConstructionArray.Emplace(cornerNode_start);

			MMCornerNode cornerNode_end;
			cornerNode_end.centerPoint = wallArray[i].endPoint;
			cornerNode_end.InsectionPointsID.Emplace(wallArray[i].startPoint);
			cornerNode_end.borderIDs.Emplace(wallArray[i].borderIDs[3]);
			cornerNode_end.borderIDs.Emplace(wallArray[i].borderIDs[4]);
			cornerNode_end.borderIDs.Emplace(wallArray[i].borderIDs[5]);
			cornerNode_end.borderIDs.Emplace(wallArray[i].borderIDs[0]);
			cornerNode_end.borderIDs.Emplace(wallArray[i].borderIDs[1]);
			cornerNode_end.borderIDs.Emplace(wallArray[i].borderIDs[2]);
			wallConstructionArray.Emplace(cornerNode_end);
		}
		else
		{
			int startPointIndex = FindIndex(wallArray[i].startPoint, wallConstructionArray);

			if (startPointIndex != wallConstructionArray.Num())
			{
				wallConstructionArray[startPointIndex].InsectionPointsID.Emplace(wallArray[i].endPoint);
				for (auto borderID : wallArray[i].borderIDs)
				{
					wallConstructionArray[startPointIndex].borderIDs.Emplace(borderID);
				}
			}
			else
			{
				MMCornerNode cornerNode;
				cornerNode.centerPoint = wallArray[i].startPoint;
				cornerNode.InsectionPointsID.Emplace(wallArray[i].endPoint);
				for (auto borderID : wallArray[i].borderIDs)
				{
					cornerNode.borderIDs.Emplace(borderID);
				}
				wallConstructionArray.Emplace(cornerNode);
			}

			int endPointIndex = FindIndex(wallArray[i].endPoint, wallConstructionArray);
			if (endPointIndex != wallConstructionArray.Num())
			{
				wallConstructionArray[endPointIndex].InsectionPointsID.Emplace(wallArray[i].startPoint);

				wallConstructionArray[endPointIndex].borderIDs.Emplace(wallArray[i].borderIDs[3]);
				wallConstructionArray[endPointIndex].borderIDs.Emplace(wallArray[i].borderIDs[4]);
				wallConstructionArray[endPointIndex].borderIDs.Emplace(wallArray[i].borderIDs[5]);
				wallConstructionArray[endPointIndex].borderIDs.Emplace(wallArray[i].borderIDs[0]);
				wallConstructionArray[endPointIndex].borderIDs.Emplace(wallArray[i].borderIDs[1]);
				wallConstructionArray[endPointIndex].borderIDs.Emplace(wallArray[i].borderIDs[2]);
			}
			else
			{
				MMCornerNode cornerNode;

				cornerNode.centerPoint = wallArray[i].endPoint;
				cornerNode.InsectionPointsID.Emplace(wallArray[i].startPoint);

				cornerNode.borderIDs.Emplace(wallArray[i].borderIDs[3]);
				cornerNode.borderIDs.Emplace(wallArray[i].borderIDs[4]);
				cornerNode.borderIDs.Emplace(wallArray[i].borderIDs[5]);
				cornerNode.borderIDs.Emplace(wallArray[i].borderIDs[0]);
				cornerNode.borderIDs.Emplace(wallArray[i].borderIDs[1]);
				cornerNode.borderIDs.Emplace(wallArray[i].borderIDs[2]);
				wallConstructionArray.Emplace(cornerNode);
			}
		}
	}
};

/**
 * @brief 计算墙体角点
 * @param wallConstructionArray 墙体结构化存储后的集合
 * @param borderPointsList 边缘点集合
 */
void MMCorner::ComputeWallConstructionArrayConer(TArray<MMCornerNode> wallConstructionArray,
                                                 TArray<FVector2d>& borderPointsList)
{
	for (int i = 0; i < wallConstructionArray.Num(); i++)
	{
		MultiLineSegmentsCollinearCorner(wallConstructionArray[i].borderIDs, borderPointsList);
	}
}

/**
 * @brief 点到线转换及转角处理 非等宽
 * @param polyLineList 奇数为中心线线段的起始点，偶数为中心线线段的终止点
 * @param width 线段的宽(厚)度集合
 * @param borderPoints 边缘点集合
 * @param borderIndex 边缘点索引集合
 */
void MMCorner::PointToLinesCorner(TArray<FVector2d> polyLineList, TArray<double> width, TArray<FVector2d>& borderPoints,
                                  TArray<uint32>& borderIndex)
{
	TArray<MMCenterLine> wallArray;
	TArray<MMCornerNode> wallConstructionArray;
	uint32 pointIndexStart = 0;
	wallArray.Empty();
	wallConstructionArray.Empty();
	for (int i = 0; i < polyLineList.Num(); i += 2)
	{
		MMCenterLine centerLine;
		CalculateCenterlineBorderPoints(polyLineList[i], polyLineList[i + 1], width[i / 2], borderPoints,
		                                pointIndexStart, borderIndex);

		centerLine.startPoint = polyLineList[i];
		centerLine.endPoint = polyLineList[i + 1];
		centerLine.borderIDs = {
			pointIndexStart - 6,
			pointIndexStart - 5,
			pointIndexStart - 4,
			pointIndexStart - 3,
			pointIndexStart - 2,
			pointIndexStart - 1
		};
		wallArray.Emplace(centerLine);
	}
	WallConstruction(wallArray, wallConstructionArray);
	SortInsectionPoint(wallConstructionArray);
	ComputeWallConstructionArrayConer(wallConstructionArray, borderPoints);
};

void MMCorner::PointsToPolylineCorner(TArray<FVector2d> polyLineList, double width, TArray<FVector2d>& borderPoints,
						TArray<uint32>& borderIndex)
{
	TArray<MMCenterLine> wallArray;
	TArray<MMCornerNode> wallConstructionArray;
	uint32 pointIndexStart = 0;
	wallArray.Empty();
	wallConstructionArray.Empty();
	for (int i = 0; i < polyLineList.Num()-1; i ++)
	{
		MMCenterLine centerLine;
		CalculateCenterlineBorderPoints(polyLineList[i], polyLineList[i + 1], width, borderPoints,
										pointIndexStart, borderIndex);

		centerLine.startPoint = polyLineList[i];
		centerLine.endPoint = polyLineList[i + 1];
		centerLine.borderIDs = {
			pointIndexStart - 6,
			pointIndexStart - 5,
			pointIndexStart - 4,
			pointIndexStart - 3,
			pointIndexStart - 2,
			pointIndexStart - 1
		};
		wallArray.Emplace(centerLine);
	}
	WallConstruction(wallArray, wallConstructionArray);
	SortInsectionPoint(wallConstructionArray);
	ComputeWallConstructionArrayConer(wallConstructionArray, borderPoints);
}
