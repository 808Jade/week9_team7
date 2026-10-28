#include "EnginePCH.h"
#include "Rotator.h"
#include "Math/EngineMath.h"

FRotator::FRotator()
{
	Pitch = 0; Yaw = 0; Roll = 0;
}

FRotator::FRotator(float P, float Y, float R)
{
	Pitch = P; Yaw = Y; Roll = R;
}

FRotator::FRotator(FVector V)
{
	Pitch = V.X; Yaw = V.Y; Roll = V.Z;
}

FRotator::FRotator(float Angle)
{
	Pitch = Angle; Yaw = Angle; Roll = Angle;
}


FQuat FRotator::Quaternion() const
{
	// Degree를 Radian으로 변환
	float PRad = Pitch * (PI / 180.0f);
	float YRad = Yaw * (PI / 180.0f);
	float RRad = Roll * (PI / 180.0f);
	
	return FQuat::MakeFromEuler(RRad, PRad, YRad);
}

FVector FRotator::ToVector() const
{
	return FVector(Pitch, Yaw, Roll);
}

FRotator FRotator::Identitiy = FRotator(0, 0, 0);

FRotator operator+(FRotator Rot, const FVector& Vec)
{
	Rot.Roll += Vec.X;
	Rot.Pitch += Vec.Y;
	Rot.Yaw += Vec.Z;

	return Rot;
}

const FRotator FRotator::ZeroRotator(0.0f, 0.0f, 0.0f);