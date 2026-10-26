#include "EnginePCH.h"
#include "Transform.h"

FTransform::FTransform(FVector InL, FVector InS, FRotator InR)
{
	Location = InL;
	Scale = InS;
	Rotation = InR;
}

FMatrix FTransform::GetLocalMatrix() const
{
    FMatrix RotationMatrix = Rotation.Quaternion().ToFMatrix();

    FMatrix WorldMatrix = FMatrix(
        RotationMatrix[0][0] * Scale.X, RotationMatrix[0][1] * Scale.X, RotationMatrix[0][2] * Scale.X, 0.0f,
        RotationMatrix[1][0] * Scale.Y, RotationMatrix[1][1] * Scale.Y, RotationMatrix[1][2] * Scale.Y, 0.0f,
        RotationMatrix[2][0] * Scale.Z, RotationMatrix[2][1] * Scale.Z, RotationMatrix[2][2] * Scale.Z, 0.0f,
        Location.X, Location.Y, Location.Z, 1.0f
    );

    return WorldMatrix;
}

FVector FTransform::GetForward() const
{
    FQuat Quat = GetOrientation();
    return Quat.RotateVector(FVector(1.0f, 0.0f, 0.0f));
}

FVector FTransform::GetUp() const
{
    FQuat Quat = GetOrientation();
    return Quat.RotateVector(FVector(0.0f, 0.0f, 1.0f));
}

FVector FTransform::GetRight() const
{
    return GetOrientation().RotateVector(FVector(0.0f, 1.0f, 0.0f));
}

FQuat FTransform::GetOrientation() const
{
    return FQuat::MakeFromEuler(FMath::DegreesToRadians(Rotation.Roll), FMath::DegreesToRadians(Rotation.Pitch), FMath::DegreesToRadians(Rotation.Yaw));
}

FTransform FTransform::GetRelativeTransformNoScale(const FTransform& Other) const
{
    FVector DeltaLocation = Location - Other.Location;
    FQuat InvOtherQuat = Other.Rotation.Quaternion().Inverse();

    FVector RelativeLocation = InvOtherQuat.RotateVector(DeltaLocation);
    FQuat RelativeQuat = InvOtherQuat * Rotation.Quaternion();
       
    return FTransform(RelativeQuat.ToFRotator(), RelativeLocation);
}

FVector FTransform::TransformPosition(const FVector& V) const
{
    const FVector ScaleVec = Scale * V;
    const FVector RotatedVec = Rotation.Quaternion().RotateVector(ScaleVec);
    const FVector TranslatedVec = RotatedVec + Location;

    return TranslatedVec;
}

FVector FTransform::TransformVectorNoScale(const FVector& V) const
{
    const FVector RotatedVec = Rotation.Quaternion().RotateVector(V);

    return RotatedVec;
}

FVector FTransform::InverseTransformVector(const FVector& V) const
{
    FVector QuatInverseRotateVector = Rotation.Quaternion().Inverse().RotateVector(V);

    return QuatInverseRotateVector / Scale;
}

FVector FTransform::InverseTransformPosition(const FVector& V) const
{
    FVector TranslatedVec = V - Location;
    FVector QuatInverseRotateVector = Rotation.Quaternion().Inverse().RotateVector(TranslatedVec);

    return QuatInverseRotateVector / Scale;
}

FTransform FTransform::Identity = FTransform(FVector(0, 0, 0), FVector(1, 1, 1), FRotator(0, 0, 0));