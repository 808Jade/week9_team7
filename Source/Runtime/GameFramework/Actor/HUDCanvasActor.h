#pragma once
#include "GameFramework/Actor/CanvasActor.h"

class UUIImageComponent;
class UUITextComponent;

class AHUDCanvasActor : public ACanvasActor
{
	DECLARE_CLASS(AHUDCanvasActor, ACanvasActor)
	REFLECT_START(ClassName)
		REFLECT_END()

public:
	AHUDCanvasActor();
	virtual ~AHUDCanvasActor() = default;

	virtual void Tick(float DeltaTime) override;
	virtual void SetCanvasVisibility(bool bVisible) override;

	// 상호작용 안내문구 표시 / 숨김
	void ShowInteractPrompt(const FString& InText);
	void HideInteractPrompt();

	// 자막/대사 표시 (Duration 초 후 자동 숨김)
	void ShowSubtitle(const FString& InText, float Duration = 3.0f);
	void HideSubtitle();

	// 화면 페이드 연출 (TargetOpacity: 1.0f면 완전 암전, 0.0f면 투명 / Duration: 걸리는 시간)
	void StartFade(float TargetOpacity, float Duration);

	// 컴포넌트 접근자
	UUIImageComponent* GetCrosshairImage() const { return CrosshairImage; }
	UUITextComponent* GetInteractPromptText() const { return InteractPromptText; }
	UUITextComponent* GetSubtitleText() const { return SubtitleText; }
	UUIImageComponent* GetFadeImage() const { return FadeImage; }

private:
	UUIImageComponent* CrosshairImage = nullptr;
	UUITextComponent* InteractPromptText = nullptr;
	UUITextComponent* SubtitleText = nullptr;
	UUIImageComponent* FadeImage = nullptr;

	// 자막 타이머
	float SubtitleRemainingTime = 0.0f;

	// 페이드 연출 상태 변수
	bool bIsFading = false;
	float CurrentFadeOpacity = 0.0f;
	float StartFadeOpacity = 0.0f;
	float TargetFadeOpacity = 0.0f;
	float FadeDuration = 0.0f;
	float FadeElapsed = 0.0f;

	// 상호작용 표시 상태
	bool bShowInteractPrompt = false;
};