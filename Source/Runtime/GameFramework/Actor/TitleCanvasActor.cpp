#include "EnginePCH.h"
#include "TitleCanvasActor.h"
#include "Components/UIImageComponent.h"
#include "Components/UITextComponent.h"
#include "Input/InputSystem.h"
#include "GameFramework/Actor/UIManagerActor.h"
#include "Asset/AssetManager.h"
#include "Launch/LaunchEngineLoop.h"
#include "Engine/Engine.h"

ATitleCanvasActor::ATitleCanvasActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. 순수 검은 배경 오버레이 (뷰포트 전체 자동 일치, Layer 5)
	BackgroundImage = CreateDefaultSubobject<UUIImageComponent>("TitleBackground");
	BackgroundImage->SetupAttachment(GetRootComponent());
	BackgroundImage->SetMatchViewportSize(true);
	BackgroundImage->SetTexture(UAssetManager::Get().LoadTexture("Assets/Models/Background/TitleBackground.png"));
	BackgroundImage->SetColor(FColor(255, 255, 255, 255));
	BackgroundImage->SetLayer(5);

	// 2. 메인 타이틀 텍스트 (화면 상단 30% 위치, 중앙에서 0.25 왼쪽 가로 정렬, Layer 20)
	TitleText = CreateDefaultSubobject<UUITextComponent>("TitleLogoText");
	TitleText->SetupAttachment(GetRootComponent());
	TitleText->SetText("PORTAL3");
	TitleText->SetFontSize(72.0f);
	TitleText->SetCenterHorizontally(true);
	TitleText->SetRelativeX(0.25f);
	TitleText->SetRelativeY(0.30f);
	TitleText->SetColor(FColor(255, 255, 255, 255)); // 하얀색
	TitleText->SetLayer(20);

	// 3. 서브타이틀 텍스트 (화면 상단 42% 위치, 중앙에서 0.25 왼쪽 가로 정렬, Layer 20)
	SubtitleText = CreateDefaultSubobject<UUITextComponent>("TitleSubtitleText");
	SubtitleText->SetupAttachment(GetRootComponent());
	SubtitleText->SetText("Aperture Science Enrichment Center");
	SubtitleText->SetFontSize(22.0f);
	SubtitleText->SetCenterHorizontally(true);
	SubtitleText->SetRelativeX(0.25f);
	SubtitleText->SetRelativeY(0.42f);
	SubtitleText->SetColor(FColor(200, 200, 200, 255));
	SubtitleText->SetLayer(20);

	// 4. [ Enter ] 시작 안내 (화면 상단 60% 위치, 중앙에서 0.25 왼쪽 가로 정렬, Layer 20)
	StartPromptText = CreateDefaultSubobject<UUITextComponent>("TitleStartPrompt");
	StartPromptText->SetupAttachment(GetRootComponent());
	StartPromptText->SetText("[ Enter ]  시작");
	StartPromptText->SetFontSize(28.0f);
	StartPromptText->SetCenterHorizontally(true);
	StartPromptText->SetRelativeX(0.25f);
	StartPromptText->SetRelativeY(0.60f);
	StartPromptText->SetColor(FColor(255, 255, 255, 255)); // 하얀색
	StartPromptText->SetLayer(20);

	// 5. [ ESC ] 종료 안내 (화면 상단 68% 위치, 중앙에서 0.25 왼쪽 가로 정렬, Layer 20)
	QuitPromptText = CreateDefaultSubobject<UUITextComponent>("TitleQuitPrompt");
	QuitPromptText->SetupAttachment(GetRootComponent());
	QuitPromptText->SetText("[ ESC ]  종료");
	QuitPromptText->SetFontSize(20.0f);
	QuitPromptText->SetCenterHorizontally(true);
	QuitPromptText->SetRelativeX(0.25f);
	QuitPromptText->SetRelativeY(0.68f);
	QuitPromptText->SetColor(FColor(160, 160, 160, 255));
	QuitPromptText->SetLayer(20);
}

void ATitleCanvasActor::BeginPlay()
{
	Super::BeginPlay();

	// 배경 텍스처 로드 및 색상(White: 255, 255, 255) 적용 (텍스처 본연의 색상 표시)
	if (BackgroundImage)
	{
		UTexture2D* BgTex = UAssetManager::Get().LoadTexture("Assets/Models/Background/TitleBackground.png");
		if (BgTex)
		{
			BackgroundImage->SetTexture(BgTex);
		}
		BackgroundImage->SetColor(FColor(255, 255, 255, 255));
	}

	// 씬 파일에서 로드된 이후에도 화면 중앙(0.5)에서 0.25 왼쪽(0.25 비율) 위치에 가로 중앙 정렬 적용
	if (TitleText) TitleText->SetRelativeX(0.25f);
	if (SubtitleText) SubtitleText->SetRelativeX(0.25f);
	if (StartPromptText) StartPromptText->SetRelativeX(0.25f);
	if (QuitPromptText) QuitPromptText->SetRelativeX(0.25f);
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
		if (GEngine)
		{
			GEngine->GetEngineLoop().RequestExit();
		}
	}
}
