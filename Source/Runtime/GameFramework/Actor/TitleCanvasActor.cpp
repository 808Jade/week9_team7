#include "EnginePCH.h"
#include "TitleCanvasActor.h"
#include "Components/UIImageComponent.h"
#include "Components/UITextComponent.h"
#include "Input/InputSystem.h"
#include "GameFramework/Actor/UIManagerActor.h"

ATitleCanvasActor::ATitleCanvasActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. 어두운 배경 오버레이 (Layer 5)
	BackgroundImage = CreateDefaultSubobject<UUIImageComponent>("TitleBackground");
	BackgroundImage->SetupAttachment(GetRootComponent());
	BackgroundImage->SetPosition(0.0f, 0.0f);
	BackgroundImage->SetSize(1920.0f, 1080.0f);
	BackgroundImage->SetColor(FColor(12, 16, 22, 220)); // 어두운 블루그레이 톤
	BackgroundImage->SetLayer(5);

	// 2. 메인 타이틀 텍스트 (포탈 블루 색상, Layer 20)
	TitleText = CreateDefaultSubobject<UUITextComponent>("TitleLogoText");
	TitleText->SetupAttachment(GetRootComponent());
	TitleText->SetText("PORTAL");
	TitleText->SetFontSize(64.0f);
	TitleText->SetPosition(780.0f, 320.0f);
	TitleText->SetColor(FColor(0, 180, 255, 255)); // Aperture Blue
	TitleText->SetLayer(20);

	// 3. 서브타이틀 텍스트 (Layer 20)
	SubtitleText = CreateDefaultSubobject<UUITextComponent>("TitleSubtitleText");
	SubtitleText->SetupAttachment(GetRootComponent());
	SubtitleText->SetText("Aperture Science Enrichment Center");
	SubtitleText->SetFontSize(20.0f);
	SubtitleText->SetPosition(680.0f, 410.0f);
	SubtitleText->SetColor(FColor(180, 180, 180, 255));
	SubtitleText->SetLayer(20);

	// 4. [ Enter ] 게임 시작 안내 (Layer 20)
	StartPromptText = CreateDefaultSubobject<UUITextComponent>("TitleStartPrompt");
	StartPromptText->SetupAttachment(GetRootComponent());
	StartPromptText->SetText("[ Enter ]  게임 시작");
	StartPromptText->SetFontSize(26.0f);
	StartPromptText->SetPosition(820.0f, 580.0f);
	StartPromptText->SetColor(FColor(255, 255, 255, 255));
	StartPromptText->SetLayer(20);

	// 5. [ ESC ] 종료 안내 (Layer 20)
	QuitPromptText = CreateDefaultSubobject<UUITextComponent>("TitleQuitPrompt");
	QuitPromptText->SetupAttachment(GetRootComponent());
	QuitPromptText->SetText("[ ESC ]  종료");
	QuitPromptText->SetFontSize(22.0f);
	QuitPromptText->SetPosition(870.0f, 650.0f);
	QuitPromptText->SetColor(FColor(150, 150, 150, 255));
	QuitPromptText->SetLayer(20);
}

void ATitleCanvasActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 캔버스가 활성화되어 있을 때만 입력 처리 및 애니메이션
	if (!IsCanvasVisible())
	{
		return;
	}

	// 1. [ Enter ] 깜빡임(Blink) 효과
	BlinkTimer += DeltaTime * 3.0f;
	if (StartPromptText)
	{
		float Opacity = 0.5f + 0.5f * sinf(BlinkTimer);
		StartPromptText->SetOpacity(Opacity);
	}

	// 2. 키보드 입력 처리
	if (FInputSystem::IsKeyPressed(EKeyCode::Enter))
	{
		// Enter 키 누르면 -> 인게임 HUD 모드로 진입!
		if (AUIManagerActor::Get())
		{
			AUIManagerActor::Get()->SetUIMode(EUIMode::InGame);
		}
	}
	else if (FInputSystem::IsKeyPressed(EKeyCode::Escape))
	{
		// ESC 키 누르면 -> 게임 종료
		::PostQuitMessage(0);
	}
}