#include "EnginePCH.h"
#include "CameraModifier_Zoom.h"
#include "Camera/PlayerCameraManager.h"
#include <cmath>
#include <algorithm>

UCameraModifier_Zoom::UCameraModifier_Zoom()
{
	bDisabled = false;
}

void UCameraModifier_Zoom::ModifyCamera(float DeltaTime, FViewInfo& InOutPOV)
{
	Super::ModifyCamera(DeltaTime, InOutPOV);

	TargetZoomOffset = bIsZooming ? ZoomAmount : 0.0f;

	float ExpFactor = 1.0f - std::exp(-ZoomSpeed * DeltaTime);
	CurrentZoomOffset += (TargetZoomOffset - CurrentZoomOffset) * ExpFactor;

	InOutPOV.FOV = FMath::Clamp(InOutPOV.FOV - CurrentZoomOffset, 5.0f, 170.0f);
}
