#pragma once

#include "CameraModifier.h"

class UCameraModifier_Zoom : public UCameraModifier
{
	DECLARE_CLASS(UCameraModifier_Zoom, UCameraModifier)

public:
	UCameraModifier_Zoom();
	virtual ~UCameraModifier_Zoom() override = default;

	void ToggleZoom() { SetZoom(!bIsZooming); }
	void SetZoom(bool bInZoom) { bIsZooming = bInZoom; EnableModifier(); }
	bool IsZooming() const { return bIsZooming; }

	virtual void ModifyCamera(float DeltaTime, struct FViewInfo& InOutPOV) override;

private:
	bool bIsZooming = false;
	float CurrentZoomOffset = 0.0f;
	float TargetZoomOffset = 0.0f;
	float ZoomAmount = 35.0f;
	float ZoomSpeed = 14.0f;
};
