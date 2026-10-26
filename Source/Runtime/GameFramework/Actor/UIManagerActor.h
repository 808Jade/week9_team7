#pragma once
#include "GameFramework/Actor.h"

class AHUDCanvasActor;
class ATitleCanvasActor;
class APauseCanvasActor;
class AEndingCanvasActor;
class ACanvasActor;

enum class EUIMode : uint8
{
	None,
	Title,
	InGame,
	Pause,
	Ending
};

class AUIManagerActor : public AActor
{
	DECLARE_CLASS(AUIManagerActor, AActor)
	REFLECT_START(ClassName)
	REFLECT_END()

public:
	AUIManagerActor();
	virtual ~AUIManagerActor();

	virtual void BeginPlay() override;
	virtual void EndPlay() override;

	// 어디서든 한 줄로 UI 매니저에 접근할 수 있는 정적 함수
	static AUIManagerActor* Get() { return Instance; }

	// UI 모드 변경 (Title, InGame, Pause, Ending)
	void SetUIMode(EUIMode NewMode);
	EUIMode GetCurrentUIMode() const { return CurrentMode; }

	// 각 캔버스 등록 및 가져오기
	void SetHUDCanvas(AHUDCanvasActor* InCanvas);
	AHUDCanvasActor* GetHUDCanvas() const { return HUDCanvas; }

	void SetTitleCanvas(ATitleCanvasActor* InCanvas);
	ATitleCanvasActor* GetTitleCanvas() const { return TitleCanvas; }

	void SetPauseCanvas(APauseCanvasActor* InCanvas);
	APauseCanvasActor* GetPauseCanvas() const { return PauseCanvas; }

	void SetEndingCanvas(AEndingCanvasActor* InCanvas);
	AEndingCanvasActor* GetEndingCanvas() const { return EndingCanvas; }

	// 게임 진행 시간 및 타이머 제어
	float GetElapsedTime() const;
	FString GetFormattedTime() const;
	void ResetTimer();
	void StartTimer();
	void StopTimer();

	// 크로스헤어 상태 제어
	void SetCrosshairState(const FString& InState);
	FString GetCrosshairState() const;
	void SetCrosshairSize(float InWidth, float InHeight);
	void SetCrosshairVisibility(bool bVisible);

private:
	void AutoDiscoverCanvases();

	static AUIManagerActor* Instance;

	EUIMode CurrentMode = EUIMode::Title; // 기본 시작 모드는 타이틀(시작) 화면

	AHUDCanvasActor* HUDCanvas = nullptr;
	ATitleCanvasActor* TitleCanvas = nullptr;
	APauseCanvasActor* PauseCanvas = nullptr;
	AEndingCanvasActor* EndingCanvas = nullptr;
};
