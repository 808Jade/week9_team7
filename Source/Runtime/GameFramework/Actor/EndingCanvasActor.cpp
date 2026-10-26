#include "EnginePCH.h"
#include "EndingCanvasActor.h"
#include "Components/UIImageComponent.h"
#include "Components/UITextComponent.h"
#include "Input/InputSystem.h"
#include "GameFramework/Actor/UIManagerActor.h"
#include "GameFramework/PortalPawn.h"
#include "Engine/World.h"
#include <cstdio>
#include "Asset/AssetManager.h"

AEndingCanvasActor::AEndingCanvasActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. 순수 검은 배경 오버레이 (뷰포트 전체 자동 일치, Layer 5)
	BackgroundImage = CreateDefaultSubobject<UUIImageComponent>("EndingBackground");
	BackgroundImage->SetupAttachment(GetRootComponent());
	BackgroundImage->SetTexture(UAssetManager::Get().LoadTexture("Assets/Models/Background/EndingBackground.png"));
	BackgroundImage->SetMatchViewportSize(true);
	BackgroundImage->SetColor(FColor(255, 255, 255, 255)); // 완전 까만 화면
	BackgroundImage->SetLayer(5);

	// 2. 메인 엔딩 타이틀 텍스트 (화면 상단 28% 위치 가로 중앙 정렬, Layer 20)
	TitleText = CreateDefaultSubobject<UUITextComponent>("EndingTitleText");
	TitleText->SetupAttachment(GetRootComponent());
	TitleText->SetText("TEST COMPLETE");
	TitleText->SetFontSize(64.0f);
	TitleText->SetCenterHorizontally(true);
	TitleText->SetRelativeY(0.28f);
	TitleText->SetColor(FColor(255, 255, 255, 255)); // 하얀색
	TitleText->SetLayer(20);

	// 3. 서브타이틀 텍스트 (화면 상단 38% 위치 가로 중앙 정렬, Layer 20)
	SubtitleText = CreateDefaultSubobject<UUITextComponent>("EndingSubtitleText");
	SubtitleText->SetupAttachment(GetRootComponent());
	SubtitleText->SetText("모든 테스트 챔버를 탈출하였습니다.");
	SubtitleText->SetFontSize(22.0f);
	SubtitleText->SetCenterHorizontally(true);
	SubtitleText->SetRelativeY(0.38f);
	SubtitleText->SetColor(FColor(200, 200, 200, 255));
	SubtitleText->SetLayer(20);

	// 4. 최종 탈출 시간 / 스코어 텍스트 (화면 상단 50% 위치 가로 중앙 정렬, Layer 20)
	ClearTimeText = CreateDefaultSubobject<UUITextComponent>("EndingClearTimeText");
	ClearTimeText->SetupAttachment(GetRootComponent());
	ClearTimeText->SetText("탈출 시간: 00:00.00");
	ClearTimeText->SetFontSize(38.0f);
	ClearTimeText->SetCenterHorizontally(true);
	ClearTimeText->SetRelativeY(0.50f);
	ClearTimeText->SetColor(FColor(255, 255, 255, 255)); // 하얀색
	ClearTimeText->SetLayer(20);

	// 5. [ Enter ] 시작 화면으로 안내 (화면 상단 66% 위치 가로 중앙 정렬, Layer 20)
	ReturnPromptText = CreateDefaultSubobject<UUITextComponent>("EndingReturnPrompt");
	ReturnPromptText->SetupAttachment(GetRootComponent());
	ReturnPromptText->SetText("[ Enter ]  시작 화면으로");
	ReturnPromptText->SetFontSize(28.0f);
	ReturnPromptText->SetCenterHorizontally(true);
	ReturnPromptText->SetRelativeY(0.66f);
	ReturnPromptText->SetColor(FColor(255, 255, 255, 255)); // 하얀색
	ReturnPromptText->SetLayer(20);

	// 처음에는 숨김
	SetCanvasVisibility(false);
}

void AEndingCanvasActor::BeginPlay()
{
	Super::BeginPlay();

	// 배경 텍스처 로드 및 색상(White: 255, 255, 255) 적용 (텍스처 본연의 색상 표시)
	if (BackgroundImage)
	{
		UTexture2D* BgTex = UAssetManager::Get().LoadTexture("Assets/Models/Background/EndingBackground.png");
		if (BgTex)
		{
			BackgroundImage->SetTexture(BgTex);
		}
		BackgroundImage->SetColor(FColor(255, 255, 255, 255));
	}

	// 씬 파일에서 로드된 이후에도 화면 중앙(0.5)에서 0.25 왼쪽(0.25 비율) 위치에 가로 중앙 정렬 적용
	if (TitleText) TitleText->SetRelativeX(0.25f);
	if (SubtitleText) SubtitleText->SetRelativeX(0.25f);
	if (ClearTimeText) ClearTimeText->SetRelativeX(0.25f);
	if (ReturnPromptText) ReturnPromptText->SetRelativeX(0.25f);
}


void AEndingCanvasActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!IsCanvasVisible())
	{
		return;
	}

	// 1. [ Enter ] 문구 깜빡임 애니메이션
	BlinkTimer += DeltaTime * 3.0f;
	if (ReturnPromptText)
	{
		float Opacity = 0.5f + 0.5f * sinf(BlinkTimer);
		ReturnPromptText->SetOpacity(Opacity);
	}

	// 2. [ Enter ] 키 누르면 -> 시작 화면으로 복귀 & 플레이어 시작 지점으로 리스폰
	if (FInputSystem::IsKeyPressed(EKeyCode::Enter))
	{
		if (AUIManagerActor::Get())
		{
			AUIManagerActor::Get()->SetUIMode(EUIMode::Title);
		}

		if (GetWorld() && GetWorld()->GetPlayerPawn())
		{
			if (APortalPawn* Pawn = Cast<APortalPawn>(GetWorld()->GetPlayerPawn()))
			{
				Pawn->Respawn();
			}
		}
	}
}

void AEndingCanvasActor::SetClearTime(float InTime)
{
	ClearTime = InTime;
	int32 TotalSeconds = static_cast<int32>(InTime);
	int32 Minutes = TotalSeconds / 60;
	int32 Seconds = TotalSeconds % 60;
	int32 Hundredths = static_cast<int32>((InTime - TotalSeconds) * 100.0f);
	char Buf[64];
	std::snprintf(Buf, sizeof(Buf), "탈출 시간: %02d:%02d.%02d", Minutes, Seconds, Hundredths);
	SetClearTimeText(Buf);
}

void AEndingCanvasActor::SetClearTimeText(const FString& InText)
{
	if (ClearTimeText)
	{
		ClearTimeText->SetText(InText);
	}
}
