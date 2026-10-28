#pragma once

#include "CameraModifier.h"

class UCameraModifier_CameraShake : public UCameraModifier
{
	DECLARE_CLASS(UCameraModifier_CameraShake, UCameraModifier)

public:
	UCameraModifier_CameraShake();
	virtual ~UCameraModifier_CameraShake() override = default;

	void StartShake(float InIntensity, float InFrequency, float InDuration)
	{
		Intensity = InIntensity;
		Frequency = InFrequency;
		Duration = InDuration;
		ElapsedTime = 0.0f;
		EnableModifier();
	}

	virtual void ModifyCamera(float DeltaTime, struct FViewInfo& InOutPOV) override;

private:
	float Intensity = 0.0f;
	float Frequency = 25.0f;
	float Duration = 0.0f;
	float ElapsedTime = 0.0f;
};
