#pragma once
#include "Core/Types.h"
#include "Math/Vector.h"

class FContactPoint
{
public:
	// Contact points on the two bodies
	FVector ContactPoints[2];
	
	FVector ContactNormal;

	float ContactDepth;

	FContactPoint() : ContactDepth(FLT_MAX) {}

	FContactPoint(const FContactPoint& Other)
	{
		ContactPoints[0] = Other.ContactPoints[0];
		ContactPoints[1] = Other.ContactPoints[1];
		ContactNormal = Other.ContactNormal;
		ContactDepth = Other.ContactDepth;
	}

	bool IsSet() const
	{
		return ContactDepth != FLT_MAX;
	}
};