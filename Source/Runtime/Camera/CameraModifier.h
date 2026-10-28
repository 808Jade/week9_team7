#pragma once

#include "UObject/Object.h"
#include "UObject/Class.h"

class APlayerCameraManager;
struct FViewInfo;

class UCameraModifier : public UObject
{
	DECLARE_CLASS(UCameraModifier, UObject)

public:
	UCameraModifier();
	virtual ~UCameraModifier() = default;

	// [가상 함수] 자식 클래스(CameraShake 등)가 오버라이드하여 카메라 정보를 직접 수정
	virtual void ModifyCamera(float DeltaTime, struct FViewInfo& InOutPOV);

	virtual void Init(APlayerCameraManager* InCameraOwner) { CameraOwner = InCameraOwner; }

	void EnableModifier() { bDisabled = false; }
	void DisableModifier() { bDisabled = true; }
	bool IsDisabled() const { return bDisabled; }

	uint8 GetPriority() const { return Priority; }
	void SetPriority(uint8 InPriority) { Priority = InPriority; }

	float GetAlpha() const { return Alpha; }
	void SetAlpha(float InAlpha) { Alpha = InAlpha; }

protected:
	APlayerCameraManager* CameraOwner = nullptr;
	float Alpha = 1.0f;
	uint8 Priority = 127;
	bool bDisabled = false;
};
