#pragma once

#include "CoreMinimal.h"

class GeometryUtility
{
public:
    static TArray<FVector> CrteateStripMiter(const TArray<FVector>& source, float width, TArray<int32>& indexs);


	//判断直线AB, CD是否相交，交点是参数posInter;
	static bool intersectSegm(FVector posA, FVector posB,
		FVector posC, FVector posD, FVector& posInter);

    static void CreateGeosBuffer();

};
