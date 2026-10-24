#pragma once

#include "Math/EngineMath.h"
#include "Math/Rotator.h"

struct FTransform
{
	FTransform() = default;
	FTransform(FVector InL, FVector InS, FRotator InR);
	FTransform(const FRotator& InR, const FVector& InL, const FVector& InS = FVector::OneVector)
		: Location(InL), Rotation(InR), Scale(InS) {}

	static FTransform Identity;

    FMatrix GetLocalMatrix() const;

	FVector GetForward() const;
	FVector GetUp() const;
	FVector GetRight() const;
	FQuat GetOrientation() const;	

	FVector Location = FVector(0, 0, 0);
	FVector Scale = FVector(1, 1, 1);
	FRotator Rotation = FRotator(0, 0, 0);

	FTransform GetRelativeTransformNoScale(const FTransform& Other) const;

	FVector TransformPosition(const FVector& V) const;

	FVector TransformVectorNoScale(const FVector& V) const;
	
	FVector InverseTransformVector(const FVector& V) const;

	FVector InverseTransformPosition(const FVector& V) const;
};
