#pragma once
#include "GameFramework/Actor/CanvasActor.h"

class UUIImageComponent;
class UUITextComponent;
class UTexture2D;

enum class ECrosshairState : uint8
{
	Unfilled,
	Blue,
	Orange,
	Both
};

class AHUDCanvasActor : public ACanvasActor
{
	DECLARE_CLASS(AHUDCanvasActor, ACanvasActor)
	REFLECT_START(ClassName)
		REFLECT_END()

public:
	AHUDCanvasActor();
	virtual ~AHUDCanvasActor() = default;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetCanvasVisibility(bool bVisible) override;

	// 크로스헤어 상태 제어 (Unfilled, Blue, Orange, Both)
	void SetCrosshairState(ECrosshairState InState);
	void SetCrosshairStateByName(const FString& InStateName);
	ECrosshairState GetCrosshairState() const { return CurrentCrosshairState; }
	FString GetCrosshairStateName() const;

	void SetCrosshairSize(float InWidth, float InHeight);
	void SetCrosshairVisibility(bool bVisible);

	// 상호작용 안내문구 표시 / 숨김
	void ShowInteractPrompt(const FString& InText);
	void HideInteractPrompt();

	// 자막/대사 표시 (Duration 초 후 자동 숨김)
	void ShowSubtitle(const FString& InText, float Duration = 3.0f);
	void HideSubtitle();

	// 설정창 (ImGui) 제어
	void SetSettingsWindowVisible(bool bVisible);
	bool IsSettingsWindowVisible() const { return bShowSettingsWindow; }
	void DrawSettingsWindow();
	void ApplyMouseSensitivity();

	// 화면 페이드 연출
	void StartFade(float TargetOpacity, float Duration);

	// 게임 타이머 제어
	void StartTimer() { bTimerRunning = true; }
	void StopTimer() { bTimerRunning = false; }
	void ResetTimer() { ElapsedTime = 0.0f; UpdateTimerDisplay(); }
	float GetElapsedTime() const { return ElapsedTime; }
	void SetElapsedTime(float InTime) { ElapsedTime = InTime; UpdateTimerDisplay(); }
	bool IsTimerRunning() const { return bTimerRunning; }
	FString GetFormattedTime() const;

	void SetTimerText(const FString& InText);
	void SetTimerVisibility(bool bVisible);

	// 컴포넌트 접근자
	UUIImageComponent* GetCrosshairImage() const { return CrosshairImage; }
	UUITextComponent* GetInteractPromptText() const { return InteractPromptText; }
	UUITextComponent* GetSubtitleText() const { return SubtitleText; }
	UUIImageComponent* GetFadeImage() const { return FadeImage; }
	UUITextComponent* GetTimerText() const { return TimerText; }

private:
	void UpdateTimerDisplay();

	UUIImageComponent* CrosshairImage = nullptr;
	UUITextComponent* InteractPromptText = nullptr;
	UUITextComponent* SubtitleText = nullptr;
	UUIImageComponent* FadeImage = nullptr;
	UUITextComponent* TimerText = nullptr;

	// 크로스헤어 상태 및 텍스처
	ECrosshairState CurrentCrosshairState = ECrosshairState::Unfilled;
	UTexture2D* CrosshairTexUnfilled = nullptr;
	UTexture2D* CrosshairTexBlue = nullptr;
	UTexture2D* CrosshairTexOrange = nullptr;
	UTexture2D* CrosshairTexBoth = nullptr;

	// 게임 진행 시간 측정
	float ElapsedTime = 0.0f;
	bool bTimerRunning = false;
	bool bShowTimer = true;

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

	// ImGui 인게임 설정창 상태
	bool bShowSettingsWindow = false;
	float MouseSensitivity = 0.1f;
};
