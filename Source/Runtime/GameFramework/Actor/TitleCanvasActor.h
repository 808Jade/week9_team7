#pragma once
#include "GameFramework/Actor/CanvasActor.h"

class UUIImageComponent;
class UUITextComponent;

class ATitleCanvasActor : public ACanvasActor
{
	DECLARE_CLASS(ATitleCanvasActor, ACanvasActor)
	REFLECT_START(ClassName)
		REFLECT_END()

public:
	ATitleCanvasActor();
	virtual ~ATitleCanvasActor() = default;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// 컴포넌트 접근자
	UUIImageComponent* GetBackgroundImage() const { return BackgroundImage; }
	UUITextComponent* GetTitleText() const { return TitleText; }
	UUITextComponent* GetSubtitleText() const { return SubtitleText; }
	UUITextComponent* GetStartPromptText() const { return StartPromptText; }
	UUITextComponent* GetQuitPromptText() const { return QuitPromptText; }

private:
	UUIImageComponent* BackgroundImage = nullptr;
	UUITextComponent* TitleText = nullptr;
	UUITextComponent* SubtitleText = nullptr;
	UUITextComponent* StartPromptText = nullptr;
	UUITextComponent* QuitPromptText = nullptr;

	// 시작 문구 깜빡임(Blink) 애니메이션 타이머
	float BlinkTimer = 0.0f;
};
