#include "EnginePCH.h"
#include "CameraModifier_Recoil.h"
#include "Camera/PlayerCameraManager.h"
#include "Math/EngineMath.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

UCameraModifier_Recoil::UCameraModifier_Recoil()
{
	bDisabled = true;
}

void UCameraModifier_Recoil::AddRecoil(float InRecoilPitch, float InRecoilDistance, float InRecoilFOV)
{
	//TargetRecoilPitch = std::min(TargetRecoilPitch + InRecoilPitch, 8.0f);
	//TargetRecoilDist = std::min(TargetRecoilDist + InRecoilDistance, 0.20f);
	//TargetRecoilFOV = std::min(TargetRecoilFOV + InRecoilFOV, 8.0f);
	TargetRecoilPitch = TargetRecoilPitch + InRecoilPitch;
	TargetRecoilDist = TargetRecoilDist + InRecoilDistance;
	TargetRecoilFOV = TargetRecoilFOV + InRecoilFOV;


	float RandomRoll = ((std::rand() % 100) / 100.0f - 0.5f) * 0.8f;
	TargetRecoilRoll += RandomRoll;

	EnableModifier();
}

void UCameraModifier_Recoil::ModifyCamera(float DeltaTime, FViewInfo& InOutPOV)
{
	Super::ModifyCamera(DeltaTime, InOutPOV);

	if (IsDisabled())
	{
		return;
	}

	float KickFactor = 1.0f - std::exp(-KickSpeed * DeltaTime);
	CurrentRecoilPitch += (TargetRecoilPitch - CurrentRecoilPitch) * KickFactor;
	CurrentRecoilDist += (TargetRecoilDist - CurrentRecoilDist) * KickFactor;
	CurrentRecoilRoll += (TargetRecoilRoll - CurrentRecoilRoll) * KickFactor;
	CurrentRecoilFOV += (TargetRecoilFOV - CurrentRecoilFOV) * KickFactor;

	float RecovFactor = 1.0f - std::exp(-RecoverySpeed * DeltaTime);
	TargetRecoilPitch -= TargetRecoilPitch * RecovFactor;
	TargetRecoilDist -= TargetRecoilDist * RecovFactor;
	TargetRecoilRoll -= TargetRecoilRoll * RecovFactor;
	TargetRecoilFOV -= TargetRecoilFOV * RecovFactor;

	FVector ForwardVec = InOutPOV.Rotation.Quaternion().GetForwardVector();
	InOutPOV.Rotation.Pitch -= CurrentRecoilPitch;
	InOutPOV.Rotation.Roll += CurrentRecoilRoll;
	InOutPOV.Location -= ForwardVec * CurrentRecoilDist;
	InOutPOV.FOV += CurrentRecoilFOV;

	if (std::abs(CurrentRecoilPitch) < 0.01f && CurrentRecoilDist < 0.001f && CurrentRecoilFOV < 0.01f && std::abs(TargetRecoilPitch) < 0.01f)
	{
		CurrentRecoilPitch = TargetRecoilPitch = 0.0f;
		CurrentRecoilDist = TargetRecoilDist = 0.0f;
		CurrentRecoilRoll = TargetRecoilRoll = 0.0f;
		CurrentRecoilFOV = TargetRecoilFOV = 0.0f;
		DisableModifier();
	}
}
