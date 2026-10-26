#pragma once
#include "GameFramework/Actor/CanvasActor.h"

class UUIImageComponent;
class UUITextComponent;

enum class EEndingType
{
	Success,
	Fail
};

class AEndingCanvasActor : public ACanvasActor
{
	DECLARE_CLASS(AEndingCanvasActor, ACanvasActor)
	REFLECT_START(ClassName)
	REFLECT_END()

public:
	AEndingCanvasActor();
	virtual ~AEndingCanvasActor() = default;

	virtual void Tick(float DeltaTime) override;

	// 엔딩 종류 (성공 / 실패) 설정
	void SetEndingType(EEndingType InType);
	EEndingType GetEndingType() const { return CurrentEndingType; }
	void SetEndingFail(bool bFail) { SetEndingType(bFail ? EEndingType::Fail : EEndingType::Success); }

	// 최종 탈출 시간(스코어) 설정 및 표시 갱신
	void SetClearTime(float InTime);
	void SetClearTimeText(const FString& InText);
	float GetClearTime() const { return ClearTime; }
	void UpdateClearTimeDisplay();

	virtual void BeginPlay() override;

	// 컴포넌트 접근자
	UUIImageComponent* GetBackgroundImage() const { return BackgroundImage; }
	UUITextComponent* GetTitleText() const { return TitleText; }
	UUITextComponent* GetSubtitleText() const { return SubtitleText; }
	UUITextComponent* GetClearTimeText() const { return ClearTimeText; }
	UUITextComponent* GetReturnPromptText() const { return ReturnPromptText; }

private:
	UUIImageComponent* BackgroundImage = nullptr;
	UUITextComponent* TitleText = nullptr;
	UUITextComponent* SubtitleText = nullptr;
	UUITextComponent* ClearTimeText = nullptr;
	UUITextComponent* ReturnPromptText = nullptr;

	EEndingType CurrentEndingType = EEndingType::Success;
	float ClearTime = 0.0f;

	// 복귀 문구 깜빡임(Blink) 애니메이션 타이머
	float BlinkTimer = 0.0f;
};
