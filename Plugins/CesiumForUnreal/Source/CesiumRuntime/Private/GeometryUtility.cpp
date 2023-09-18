#include "GeometryUtility.h"


TArray<FVector> GeometryUtility::CrteateStripMiter(const TArray<FVector>& source, float width, TArray<int32>& indexs)
{
    TArray<FVector> stripArray, lefts, rights;
    if(source.Num() < 2)
    {
        return stripArray;
    }
    FVector currtVec, nextVec, vertlVec, nextVerl,
		normalVec, curtPos, nextPos, lastPos;
    int32 size = source.Num();
   
	for (size_t i = 0; i < size; i++)
	{
		if (i == 0) //第一个点
		{
            curtPos = source[i];
			nextPos = source[i + 1];
			currtVec = nextPos - curtPos;
			currtVec.Normalize();
			vertlVec = currtVec ^ FVector(0, 0, 1);
            stripArray.Add(source[i] - vertlVec * width);
            stripArray.Add(source[i] + vertlVec * width);
            //lefts.Add(source[i] - vertlVec * width);
            //rights.Add(source[i] + vertlVec * width);
			continue;
		}
		else if (i == size - 1) //最后一个点
		{
			curtPos = source[i];
			lastPos = source[i - 1];
			currtVec = curtPos - lastPos;
			currtVec.Normalize();
			vertlVec = currtVec ^ FVector(0, 0, 1);
            //lefts.Add(source[i] - vertlVec * width);
            //rights.Add(source[i] + vertlVec * width);
            stripArray.Add(source[i] - vertlVec * width);
            stripArray.Add(source[i] + vertlVec * width);
			continue;
		}
		else
		{
			curtPos = source[i];
			lastPos = source[i - 1];
			nextPos = source[i + 1];
			currtVec = curtPos - lastPos;
			nextVec = nextPos - curtPos;
			currtVec.Normalize();
			nextVec.Normalize();
			vertlVec = currtVec ^ FVector(0, 0, 1);
			nextVerl = nextVec ^ FVector(0, 0, 1);
			FVector A1, A2, B1, B2, C1, C2, D1, D2;
            A1 = lastPos - vertlVec * width;
            A2 = curtPos - vertlVec * width;
            B1 = lastPos + vertlVec * width;
            B2 = curtPos + vertlVec * width;
            C1 = curtPos - nextVerl * width;
            C2 = nextPos - nextVerl * width;
            D1 = curtPos + nextVerl * width;
            D2 = nextPos + nextVerl * width;

			float offset = 1.0;
			FVector interA;
			bool isInter = intersectSegm(A1, A2, C1, C2, interA);
			if (isInter)
			{
				interA.Z = curtPos.Z;
                stripArray.Add(interA);
                stripArray.Add(B2*offset);
                stripArray.Add(D1*offset);
			}
			FVector interB;
			isInter = intersectSegm(B1, B2, D1, D2, interB);
			if (isInter)
			{
				interB.Z = curtPos.Z;
				stripArray.Add(interB);
				stripArray.Add(A2*offset);
				stripArray.Add(C1*offset);
			}
		}

	}
    /*
	if (source[0] == source[size - 1])
	{
		FVector interA, interB;
        bool isInter = intersectSegm(lefts[0], lefts[1], lefts[lefts.Num() - 2], lefts[lefts.Num() - 1], interA);
		if (isInter)
		{
			lefts[0] = interA;
			lefts.Pop();
			lefts.Add(interA);
			lefts.Add(interA);
		}
		else
		{
			lefts.Add(lefts[0]);
		}
		isInter = intersectSegm(rights[0], rights[1], rights[rights.Num() - 2], rights[rights.Num() - 1], interB);
		if (isInter)
		{
			rights[0] = interB;
			rights.Pop();
			rights.Add(rights[0]);
			rights.Add(rights[0]);
		}
		else
		{
			rights.Add(rights[0]);
		}
	}*/

    size = stripArray.Num();
    int32 index0 = 0, index1 = 1, index2 = 2;
    for (size_t i = 0; i < size; i +=3)
    {
       indexs.Add(index0);
       indexs.Add(index1);
       indexs.Add(index2);
       index0++;
       index1++;
       index2++;
    }
	return stripArray;
}

bool GeometryUtility::intersectSegm(FVector posA, FVector posB, FVector posC, FVector posD, FVector& posInter)
{
   double delta = (posB.Y - posA.Y)*(posD.X - posC.X)
		- (posA.X - posB.X)*(posC.Y - posD.Y);
	if (delta <= (1e-6) && delta >= -(1e-6))
	{
		return false;
	}

	// 线段所在直线的交点坐标 (x , y)      
	posInter.X = ((posB.X - posA.X) * (posD.X - posC.X) * (posC.Y - posA.Y)
		+ (posB.Y - posA.Y) * (posD.X - posC.X) * posA.X
		- (posD.Y - posC.Y) * (posB.X - posA.X) * posC.X) / delta;

	posInter.Y = -((posB.Y - posA.Y) * (posD.Y - posC.Y) * (posC.X - posA.X)
		+ (posB.X - posA.X) * (posD.Y - posC.Y) * posA.Y
		- (posD.X - posC.X) * (posB.Y - posA.Y) * posC.Y) / delta;

	if (
		// 交点在线段1上  
		(posInter.X - posA.X) * (posInter.X - posB.X) <= 0
		&& (posInter.Y - posA.Y) * (posInter.Y - posB.Y) <= 0
		// 且交点也在线段2上  
		&& (posInter.X - posC.X) * (posInter.X - posD.X) <= 0
		&& (posInter.Y - posC.Y) * (posInter.Y - posD.Y) <= 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void GeometryUtility::CreateGeosBuffer()
{

}
