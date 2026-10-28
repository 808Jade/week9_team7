#pragma once

#include "Engine.h"

#include "Render/SkyboxRenderer.h"
#include "Text/TextRenderer.h"
#include "Render/FXAARenderer.h"
#include "Render/PostProcessSettings.h"

class UGameInstance;

class UGameEngine : public UEngine
{
	DECLARE_CLASS(UGameEngine, UEngine)

public:
	FEngineConfig GetConfig() const override;

	virtual bool Init() override;
	virtual void TickInput(float DeltaTime);
	virtual void Tick(float DeltaTime) override;
	virtual void PreExit();

	void RenderFrame();

	// 감마 보정 등 화면 후처리 설정. 토글 키나 시네마틱 스크립트가 여기서 값을 바꾼다.
	FPostProcessSettings& GetPostProcessSettings() { return PostProcess; }

	bool IsFXAAEnabled() const { return bEnableFXAA; }
	void SetEnableFXAA(bool bEnable) { bEnableFXAA = bEnable; }

private:
	UGameInstance* GameInstance = nullptr;
	UWorld* GameWorld = nullptr;
	TSharedPtr<FGameViewportClient> GameViewport;

	FRenderer* Renderer = nullptr;
	TUniquePtr<FSkyboxRenderer> SkyboxRenderer;
	TUniquePtr<FTextRenderer> TextRenderer;
	TUniquePtr<FFXAARenderer> FXAARenderer;

	FPostProcessSettings PostProcess;

	bool bEnableFXAA = true;

	// ImGui 프레임워크 상태
	bool bImGuiInitialized = false;
};
