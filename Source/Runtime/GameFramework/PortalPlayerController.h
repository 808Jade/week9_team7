#pragma once

#include "GameFramework/PlayerController.h"

class APortalPawn;

class APortalPlayerController : public APlayerController
{
	DECLARE_CLASS(APortalPlayerController, APlayerController)

	REFLECT_START(ClassName)
		PROPERTY(bAlwaysLookWithMouse)
		REFLECT_END()

public:
	APortalPlayerController();
	virtual ~APortalPlayerController();

	virtual void BeginPlay() override;
	virtual void EndPlay() override;
	virtual void ProcessPlayerInput(float DeltaTime) override;
	virtual void InputKey(int32 Key, bool bDown) override;

	APortalPawn* GetPortalPawn() const;

	void SetMouseCapture(bool bEnable);
	bool IsMouseCaptured() const { return bMouseCaptured; }

private:
	// 마우스 시선 회전 방식(true: 항상, false: 우클릭 시)
	bool bAlwaysLookWithMouse = true;

	// 마우스 잠금 및 커서 숨김 상태
	bool bMouseCaptured = false;
};