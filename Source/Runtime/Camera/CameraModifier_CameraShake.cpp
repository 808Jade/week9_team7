#include "EnginePCH.h"
#include "CameraModifier_CameraShake.h"
#include "PlayerCameraManager.h"

UCameraModifier_CameraShake::UCameraModifier_CameraShake()
{
}

void UCameraModifier_CameraShake::ModifyCamera(float DeltaTime, FViewInfo& InOutPOV)
{
	Super::ModifyCamera(DeltaTime, InOutPOV);

	if (IsDisabled() || Duration <= 0.0f)
	{
		return;
	}

	ElapsedTime += DeltaTime;
	if (ElapsedTime >= Duration)
	{
		DisableModifier();
		return;
	}

	const float NormalizedTime = ElapsedTime / Duration;
	const float Fade = 1.0f - NormalizedTime;
	const float Damping = Fade * Fade;
	const float Strength = FMath::Max(Intensity / 5.0f, 0.0f);
	const float Oscillation = sinf(ElapsedTime * Frequency);

	const float WavePitch = -Oscillation * 1.4f * Strength * Damping;
	const float WaveYaw = sinf(ElapsedTime * Frequency * 0.85f) * 0.35f * Strength * Damping;
	const float WaveZ = -sinf(ElapsedTime * Frequency * 1.2f) * 0.015f * Strength * Damping;

	InOutPOV.Rotation.Pitch += WavePitch;
	InOutPOV.Rotation.Yaw += WaveYaw;
	InOutPOV.Location.Z += WaveZ;
}
