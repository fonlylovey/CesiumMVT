#pragma once

#include "CoreMinimal.h"
#include "MMCorner.generated.h"

UCLASS()
class UMMCorner: public UObject
{
	GENERATED_BODY()
public:
	void CalculateIntersectionCoordinate(FVector2d& PointA, FVector2d& PointB, FVector2d& PointC, FVector2d& PointD);
	void corner(TArray<FVector2d>& retangleAB, TArray<FVector2d>& retangleCD);
	void corner(FVector2d& retangleAB_0, FVector2d& retangleAB_1, FVector2d& retangleAB_2, FVector2d& retangleAB_3,
			FVector2d& retangleCD_0, FVector2d& retangleCD_1, FVector2d& retangleCD_2, FVector2d& retangleCD_3);
	void CalculatePolylineBorderPoint(TArray<FVector2d> PolyLineArray, double thickness,
								  TArray<FVector2d>& borderVertexsList, TArray<uint32>& borderVertexsIndex);
	void PolylineCorner(TArray<FVector2d> polylineArray, double width, TArray<FVector2d>& borderVertexsList,
					TArray<uint32>& borderVertexsIndex);
};
