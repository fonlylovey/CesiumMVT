#pragma once

#include "CoreMinimal.h"

class GeometryUtility
{
public:
    static TArray<FVector> CrteateStripMiter(const TArray<FVector>& source, float width, TArray<int32>& indexs);


	//�ж�ֱ��AB, CD�Ƿ��ཻ�������ǲ���posInter;
	static bool intersectSegm(FVector posA, FVector posB,
		FVector posC, FVector posD, FVector& posInter);

    static void CreateGeosBuffer();

};
