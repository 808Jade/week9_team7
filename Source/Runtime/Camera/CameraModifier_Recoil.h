#pragma once

#include "CameraModifier.h"

class UCameraModifier_Recoil : public UCameraModifier
{
	DECLARE_CLASS(UCameraModifier_Recoil, UCameraModifier)

public:
	UCameraModifier_Recoil();
	virtual ~UCameraModifier_Recoil() override = default;

	void AddRecoil(float InRecoilPitch = 2.0f, float InRecoilDistance = 0.08f, float InRecoilFOV = 1.5f);

	virtual void ModifyCamera(float DeltaTime, struct FViewInfo& InOutPOV) override;

private:
	float CurrentRecoilPitch = 0.0f;
	float CurrentRecoilDist = 0.0f;
	float CurrentRecoilRoll = 0.0f;
	float CurrentRecoilFOV = 0.0f;

	float TargetRecoilPitch = 0.0f;
	float TargetRecoilDist = 0.0f;
	float TargetRecoilRoll = 0.0f;
	float TargetRecoilFOV = 0.0f;

	float KickSpeed = 45.0f;
	float RecoverySpeed = 8.0f;
};
