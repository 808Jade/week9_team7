#pragma once

#include "CameraModifier.h"

class UCameraModifier_Zoom : public UCameraModifier
{
	DECLARE_CLASS(UCameraModifier_Zoom, UCameraModifier)

public:
	UCameraModifier_Zoom();
	virtual ~UCameraModifier_Zoom() override = default;

	virtual void ModifyCamera(float DeltaTime, struct FViewInfo& InOutPOV) override;
};
