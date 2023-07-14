#pragma once
#include "CoreMinimal.h"

namespace MMCorner
{
	//转角节点
	struct MMCornerNode
	{
		FVector2d centerPoint;
		TArray<FVector2d> InsectionPointsID;
		TArray<uint32> borderIDs;
	};

	struct MMCenterLine
	{
		FVector2d startPoint;
		FVector2d endPoint;
		TArray<uint32> borderIDs;
	};

	struct CenterLine
	{
		uint32 index;
		double width;
		FVector2d startPoint;
		FVector2d endPoint;
		TArray<FVector2d> borderPoints;
		TArray<uint32> borderIndex;
	};

	struct PointStruct
	{
		TArray<uint32> borderIDs;
		double angle;
		FVector2d pointID;
	};
	
	/**
	 * @brief 计算两直线的相交点坐标
	 * @param PointA 直线的坐标位置A
	 * @param PointB 直线的坐标位置B
	 * @param PointC 另一直线的坐标位置C
	 * @param PointD 另一直线的坐标位置D
	 */
	void CalculateIntersectionCoordinate(const FVector2d& pointA, FVector2d& pointB, FVector2d& pointC,
	                                     const FVector2d& pointD);
	/**
	 * @brief 计算中心线边缘点集合及索引
	 * @param startPoint 中心线起始点
	 * @param endPoint 中心线终止点
	 * @param thickness  中心线宽度
	 * @param borderVertexsList 中心线边缘点集合
	 * @param vectexsIndex  中心线边缘点开始索引
	 * @param borderVertexsIndex 中心线边缘点索引集合
	 */
	void CalculateCenterlineBorderPoints(FVector2d startPoint, FVector2d endPoint, double thickness,
	                                     TArray<FVector2d>& borderVertexsList, uint32& vectexsIndex,
	                                     TArray<uint32>& borderVertexsIndex);
	/**
	 * @brief 查找对应点在集合中的索引
	 * @param point 待查找点
	 * @param wallConstructionArray 墙中心点集合
	 * @return 
	 */
	int FindIndex(FVector2d point, TArray<MMCornerNode> wallConstructionArray);

	/**
	 * @brief 计算两个向量的叉积
	 * @param v1 向量1
	 * @param v2 向量2
	 * @return 
	 */
	double CrossProduct(const FVector2d& v1, const FVector2d& v2);
	//计算极坐标角度
	double PolarAngle(FVector2d cengterPoint, FVector2d q2);
	//插入排序
	void insertionSort(TArray<PointStruct>& PointList);
	//排序
	void SortInsectionPoint(TArray<MMCornerNode>& wallConstructionArray);

	//墙体数据结构化
	void WallConstruction(TArray<MMCenterLine> wallArray, TArray<MMCornerNode>& wallConstructionArray);

	/**
	 * @brief 计算边缘点角点
	 * @param MultiLineBorderPoints 多线段线相交于一点的线段集合
	 * @param borderList 面带边缘点集合
	 */
	void MultiLineSegmentsCollinearCorner(TArray<uint32> MultiLineBorderPoints, TArray<FVector2d>& borderList);

	/**
	* @brief 计算墙体角点
	* @param wallConstructionArray 墙体结构化存储后的集合
	* @param borderPointsList 边缘点集合
	*/
	void ComputeWallConstructionArrayConer(TArray<MMCornerNode> wallConstructionArray,
						  TArray<FVector2d>& borderPointsList);
	/**
	 * @brief 点到线转换及转角处理 非等宽
	 * @param polyLineList 奇数为中心线线段的起始点，偶数为中心线线段的终止点
	 * @param width 线段的宽(厚)度集合
	 * @param borderPoints 边缘点集合
	 * @param borderIndex 边缘点索引集合
	 */
	void PointToLinesCorner(TArray<FVector2d> polyLineList, TArray<double> width, TArray<FVector2d>& borderPoints,
	                        TArray<uint32>& borderIndex);
	/**
 * @brief 点到多段线转换及转角处理 非等宽
 * @param polyLineList 点集合中前一个点为起始点，后一个点为终止点；
 * @param width 线段的宽(厚)度
 * @param borderPoints 边缘点集合
 * @param borderIndex 边缘点索引集合
 */
	void PointsToPolylineCorner(TArray<FVector2d> polyLineList,double width, TArray<FVector2d>& borderPoints,
							TArray<uint32>& borderIndex);

};
