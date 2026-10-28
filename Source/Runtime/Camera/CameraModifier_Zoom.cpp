#include "EnginePCH.h"
#include "CameraModifier_Zoom.h"

UCameraModifier_Zoom::UCameraModifier_Zoom()
{
}

void UCameraModifier_Zoom::ModifyCamera(float DeltaTime, FViewInfo& InOutPOV)
{
	Super::ModifyCamera(DeltaTime, InOutPOV);
}
