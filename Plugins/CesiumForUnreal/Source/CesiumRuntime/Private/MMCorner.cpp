#include "MMCorner.h"

void UMMCorner::CalculateIntersectionCoordinate(FVector2d& PointA, FVector2d& PointB, FVector2d& PointC, FVector2d& PointD)
{
	double k1 = (PointB.Y - PointA.Y) / (PointB.X - PointA.X);
	double b1 = PointA.Y - k1 * PointA.X;

	double k2 = (PointD.Y - PointC.Y) / (PointD.X - PointC.X);
	double b2 = PointC.Y - k2 * PointC.X;
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

void UMMCorner::corner(FVector2d& retangleAB_0, FVector2d& retangleAB_1, FVector2d& retangleAB_2, FVector2d& retangleAB_3,
            FVector2d& retangleCD_0, FVector2d& retangleCD_1, FVector2d& retangleCD_2, FVector2d& retangleCD_3)
{
	CalculateIntersectionCoordinate(retangleAB_0, retangleAB_3, retangleCD_0, retangleCD_3);
	CalculateIntersectionCoordinate(retangleAB_1, retangleAB_2, retangleCD_1, retangleCD_2);
}

void UMMCorner::CalculatePolylineBorderPoint(TArray<FVector2d> PolyLineArray, double thickness,
								  TArray<FVector2d>& borderVertexsList, TArray<uint32>& borderVertexsIndex)
{
	borderVertexsList.Empty();
	borderVertexsIndex.Empty();
	int vectexsIndex = 0;
	if (PolyLineArray.Num() < 2)
	{
		return;
	}
	for (int i = 1; i < PolyLineArray.Num(); i++)
	{
		FVector2d normalAB = FVector2d(PolyLineArray[i].Y - PolyLineArray[i - 1].Y,
									   PolyLineArray[i - 1].X - PolyLineArray[i].X);
		if (normalAB.Size() > 0)
		{
			FVector2d t = normalAB.GetSafeNormal() * 0.5 * thickness;
			borderVertexsList.Add(PolyLineArray[i - 1] + t);
			borderVertexsList.Add(PolyLineArray[i - 1] - t);
			borderVertexsList.Add(PolyLineArray[i] - t);
			borderVertexsList.Add(PolyLineArray[i] + t);

			borderVertexsIndex.Add(vectexsIndex);
			borderVertexsIndex.Add(vectexsIndex + 1);
			borderVertexsIndex.Add(vectexsIndex + 2);
			borderVertexsIndex.Add(vectexsIndex);
			borderVertexsIndex.Add(vectexsIndex + 2);
			borderVertexsIndex.Add(vectexsIndex + 3);
			vectexsIndex += 4;
		}
	}
}

void UMMCorner::PolylineCorner(TArray<FVector2d> polylineArray, double width, TArray<FVector2d>& borderVertexsList,
					TArray<uint32>& borderVertexsIndex)
{
	if (polylineArray.Num() < 3)
	{
		return;
	}
	CalculatePolylineBorderPoint(polylineArray, width, borderVertexsList, borderVertexsIndex);

	for (int i = 0; i < borderVertexsList.Num()-4; i+=4)
	{
		corner(borderVertexsList[i],borderVertexsList[i+1],borderVertexsList[i+2],
			borderVertexsList[i+3],borderVertexsList[i+4],borderVertexsList[i+5],
			borderVertexsList[i+6],borderVertexsList[i+7]);
		
	}
}
