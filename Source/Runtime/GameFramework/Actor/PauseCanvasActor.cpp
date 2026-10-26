#include "EnginePCH.h"
#include "PauseCanvasActor.h"
#include "Components/UIImageComponent.h"
#include "Components/UITextComponent.h"
#include "Input/InputSystem.h"
#include "GameFramework/Actor/UIManagerActor.h"

APauseCanvasActor::APauseCanvasActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. 반투명 검은 배경 오버레이 (Layer 40)
	DimOverlay = CreateDefaultSubobject<UUIImageComponent>("PauseDimOverlay");
	DimOverlay->SetupAttachment(GetRootComponent());
	DimOverlay->SetPosition(0.0f, 0.0f);
	DimOverlay->SetSize(1920.0f, 1080.0f);
	DimOverlay->SetColor(FColor(0, 0, 0, 180)); // 70% 정도 어두워지는 반투명 블랙
	DimOverlay->SetLayer(40);

	// 2. 일시정지 타이틀 텍스트 (Layer 50)
	TitleText = CreateDefaultSubobject<UUITextComponent>("PauseTitleText");
	TitleText->SetupAttachment(GetRootComponent());
	TitleText->SetText("PAUSED");
	TitleText->SetFontSize(48.0f);
	TitleText->SetPosition(860.0f, 320.0f);
	TitleText->SetColor(FColor(255, 255, 255, 255));
	TitleText->SetLayer(50);

	// 3. [ ESC ] 계속하기 (Layer 50)
	ResumePromptText = CreateDefaultSubobject<UUITextComponent>("PauseResumePrompt");
	ResumePromptText->SetupAttachment(GetRootComponent());
	ResumePromptText->SetText("[ ESC ]  계속하기");
	ResumePromptText->SetFontSize(24.0f);
	ResumePromptText->SetPosition(860.0f, 480.0f);
	ResumePromptText->SetColor(FColor(0, 200, 255, 255)); // 포탈 블루
	ResumePromptText->SetLayer(50);

	// 4. [ R ] 챔버 재시작 (Layer 50)
	RestartPromptText = CreateDefaultSubobject<UUITextComponent>("PauseRestartPrompt");
	RestartPromptText->SetupAttachment(GetRootComponent());
	RestartPromptText->SetText("[ R ]  챔버 재시작");
	RestartPromptText->SetFontSize(22.0f);
	RestartPromptText->SetPosition(860.0f, 540.0f);
	RestartPromptText->SetColor(FColor(200, 200, 200, 255));
	RestartPromptText->SetLayer(50);

	// 5. [ M ] 메인 메뉴로 (Layer 50)
	MenuPromptText = CreateDefaultSubobject<UUITextComponent>("PauseMenuPrompt");
	MenuPromptText->SetupAttachment(GetRootComponent());
	MenuPromptText->SetText("[ M ]  메인 메뉴로");
	MenuPromptText->SetFontSize(22.0f);
	MenuPromptText->SetPosition(860.0f, 600.0f);
	MenuPromptText->SetColor(FColor(200, 200, 200, 255));
	MenuPromptText->SetLayer(50);

	// 게임 시작 시에는 기본적으로 숨김 상태로 시작합니다.
	SetCanvasVisibility(false);
}

void APauseCanvasActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 일시정지 화면이 켜져 있을 때만 키 입력 처리
	if (!IsCanvasVisible())
	{
		return;
	}

	// 1. ESC 누르면 -> 게임 재개 (인게임 HUD 모드로 복귀)
	if (FInputSystem::IsKeyPressed(EKeyCode::Escape))
	{
		if (AUIManagerActor::Get())
		{
			AUIManagerActor::Get()->SetUIMode(EUIMode::InGame);
		}
	}
	// 2. M 누르면 -> 메인 메뉴(타이틀 화면)로 이동
	else if (FInputSystem::IsKeyPressed(EKeyCode::M))
	{
		if (AUIManagerActor::Get())
		{
			AUIManagerActor::Get()->SetUIMode(EUIMode::Title);
		}
	}
}