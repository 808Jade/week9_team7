#pragma once

#include "Vector.h"
#include "Transform.h"
#include <algorithm>
#include <cmath>

class FCapsule
{
public:
	FCapsule() : Center(FVector::ZeroVector), Radius(0.0f), HalfHeight(0.0f) {}
	FCapsule(const FVector& InCenter, float InRadius, float InHalfHeight)
		: Center(InCenter), Radius(InRadius), HalfHeight(InHalfHeight) {}
	FCapsule(const FCapsule& Other)
		: Center(Other.GetCenter()), Radius(Other.GetRadius()), HalfHeight(Other.GetHalfHeight()) {}

	const FVector& GetCenter() const
	{
		return Center;
	}

	void SetCenter(const FVector& InCenter)
	{
		Center = InCenter;
	}

	float GetRadius() const
	{
		return Radius;
	}

	void SetRadius(float InRadius)
	{
		Radius = InRadius;
	}

	float GetHalfHeight() const
	{
		return HalfHeight;
	}

	void SetHalfHeight(float InHalfHeight)
	{
		HalfHeight = InHalfHeight;
	}

	void GetSegment(const FTransform& Transform, FVector& OutP0, FVector& OutP1) const
	{
		const float SegmentHalfLength = std::max(0.0f, HalfHeight - Radius);
		const FVector WorldCenter = Transform.TransformPosition(Center);
		const FVector UpAxis = Transform.GetUp();
		OutP0 = WorldCenter - UpAxis * SegmentHalfLength;
		OutP1 = WorldCenter + UpAxis * SegmentHalfLength;
	}

private:
	FVector Center;
	float Radius;
	float HalfHeight;
};
