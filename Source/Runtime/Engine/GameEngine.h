#pragma once

#include "Engine.h"

#include "Render/SkyboxRenderer.h"
#include "Text/TextRenderer.h"
#include "Render/FXAARenderer.h"

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

private:
	UGameInstance* GameInstance = nullptr;
	UWorld* GameWorld = nullptr;
	TSharedPtr<FGameViewportClient> GameViewport;

	FRenderer* Renderer = nullptr;
	TUniquePtr<FSkyboxRenderer> SkyboxRenderer;
	TUniquePtr<FTextRenderer> TextRenderer;
	TUniquePtr<FFXAARenderer> FXAARenderer;

	// ImGui 프레임워크 상태
	bool bImGuiInitialized = false;
};
