#pragma once
#include "GameFramework/Actor/CanvasActor.h"

class UUIImageComponent;
class UUITextComponent;

class APauseCanvasActor : public ACanvasActor
{
	DECLARE_CLASS(APauseCanvasActor, ACanvasActor)
	REFLECT_START(ClassName)
		REFLECT_END()

public:
	APauseCanvasActor();
	virtual ~APauseCanvasActor() = default;

	virtual void Tick(float DeltaTime) override;

	// 컴포넌트 접근자
	UUIImageComponent* GetDimOverlay() const { return DimOverlay; }
	UUITextComponent* GetTitleText() const { return TitleText; }
	UUITextComponent* GetResumePromptText() const { return ResumePromptText; }
	UUITextComponent* GetRestartPromptText() const { return RestartPromptText; }
	UUITextComponent* GetMenuPromptText() const { return MenuPromptText; }

private:
	UUIImageComponent* DimOverlay = nullptr;
	UUITextComponent* TitleText = nullptr;
	UUITextComponent* ResumePromptText = nullptr;
	UUITextComponent* RestartPromptText = nullptr;
	UUITextComponent* MenuPromptText = nullptr;
};