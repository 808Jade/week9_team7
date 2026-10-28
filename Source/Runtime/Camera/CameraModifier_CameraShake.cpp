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

	float Damping = 1.0f - (ElapsedTime / Duration);

	float WavePitch = sinf(ElapsedTime * Frequency) * Intensity * Damping;
	float WaveYaw = cosf(ElapsedTime * Frequency * 0.9f) * (Intensity * 0.8f) * Damping;
	float WaveZ = sinf(ElapsedTime * Frequency * 1.2f) * (Intensity * 5.0f) * Damping;

	InOutPOV.Rotation.Pitch += WavePitch;
	InOutPOV.Rotation.Yaw += WaveYaw;
	InOutPOV.Location.Z += WaveZ;
}
