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

	// 1. 엔딩 배경 이미지 (뷰포트 전체 자동 일치, Layer 5)
	BackgroundImage = CreateDefaultSubobject<UUIImageComponent>("EndingBackground");
	BackgroundImage->SetupAttachment(GetRootComponent());
	BackgroundImage->SetTexture(UAssetManager::Get().LoadTexture("Assets/Models/Background/SuccessEndingBackground.png"));
	BackgroundImage->SetMatchViewportSize(true);
	BackgroundImage->SetColor(FColor(255, 255, 255, 255));
	BackgroundImage->SetLayer(5);

	// 2. 메인 엔딩 타이틀 텍스트 (Layer 20)
	TitleText = CreateDefaultSubobject<UUITextComponent>("EndingTitleText");
	TitleText->SetupAttachment(GetRootComponent());
	TitleText->SetText("TEST COMPLETE");
	TitleText->SetFontSize(64.0f);
	TitleText->SetCenterHorizontally(true);
	TitleText->SetRelativeX(0.25f);
	TitleText->SetRelativeY(0.28f);
	TitleText->SetColor(FColor(255, 255, 255, 255)); // 하얀색
	TitleText->SetLayer(20);

	// 3. 서브타이틀 텍스트 (Layer 20)
	SubtitleText = CreateDefaultSubobject<UUITextComponent>("EndingSubtitleText");
	SubtitleText->SetupAttachment(GetRootComponent());
	SubtitleText->SetText("모든 테스트 챔버를 탈출하였습니다.");
	SubtitleText->SetFontSize(22.0f);
	SubtitleText->SetCenterHorizontally(true);
	SubtitleText->SetRelativeX(0.25f);
	SubtitleText->SetRelativeY(0.38f);
	SubtitleText->SetColor(FColor(200, 200, 200, 255));
	SubtitleText->SetLayer(20);

	// 4. 최종 탈출 시간 / 스코어 텍스트 (Layer 20)
	ClearTimeText = CreateDefaultSubobject<UUITextComponent>("EndingClearTimeText");
	ClearTimeText->SetupAttachment(GetRootComponent());
	ClearTimeText->SetText("탈출 시간: 00:00.00");
	ClearTimeText->SetFontSize(38.0f);
	ClearTimeText->SetCenterHorizontally(true);
	ClearTimeText->SetRelativeX(0.25f);
	ClearTimeText->SetRelativeY(0.50f);
	ClearTimeText->SetColor(FColor(255, 255, 255, 255)); // 하얀색
	ClearTimeText->SetLayer(20);

	// 5. [ Enter ] 시작 화면으로 안내 (Layer 20)
	ReturnPromptText = CreateDefaultSubobject<UUITextComponent>("EndingReturnPrompt");
	ReturnPromptText->SetupAttachment(GetRootComponent());
	ReturnPromptText->SetText("[ Enter ]  시작 화면으로");
	ReturnPromptText->SetFontSize(28.0f);
	ReturnPromptText->SetCenterHorizontally(true);
	ReturnPromptText->SetRelativeX(0.25f);
	ReturnPromptText->SetRelativeY(0.66f);
	ReturnPromptText->SetColor(FColor(255, 255, 255, 255)); // 하얀색
	ReturnPromptText->SetLayer(20);

	// 처음에는 숨김
	SetCanvasVisibility(false);
}

void AEndingCanvasActor::BeginPlay()
{
	Super::BeginPlay();

	// 현재 엔딩 타입(기본값: Success)에 맞추어 배경 및 텍스트 스타일 초기화
	SetEndingType(CurrentEndingType);
}

void AEndingCanvasActor::SetEndingType(EEndingType InType)
{
	CurrentEndingType = InType;

	if (InType == EEndingType::Fail)
	{
		// 1. 실패 배경 텍스처 (Assets/Models/Background/FailEndingBackground.png)
		if (BackgroundImage)
		{
			UTexture2D* FailTex = UAssetManager::Get().LoadTexture("Assets/Models/Background/FailEndingBackground.png");
			if (FailTex)
			{
				BackgroundImage->SetTexture(FailTex);
			}
			BackgroundImage->SetColor(FColor(255, 255, 255, 255));
		}

		// 2. 타이틀: "실패"
		if (TitleText)
		{
			TitleText->SetText("실패");
			TitleText->SetFontSize(72.0f);
			TitleText->SetColor(FColor(255, 70, 70, 255));
			TitleText->SetCenterHorizontally(true);
			TitleText->SetRelativeX(0.25f);
			TitleText->SetRelativeY(0.32f);
			TitleText->SetVisibility(true);
		}

		// 3. 서브타이틀은 실패 화면에서 숨김 (실패, 걸린 시간, 엔터 안내만 표시)
		if (SubtitleText)
		{
			SubtitleText->SetVisibility(false);
		}

		// 4. 시간: "걸린 시간: 00:00.00"
		if (ClearTimeText)
		{
			ClearTimeText->SetFontSize(36.0f);
			ClearTimeText->SetColor(FColor(255, 255, 255, 255));
			ClearTimeText->SetCenterHorizontally(true);
			ClearTimeText->SetRelativeX(0.25f);
			ClearTimeText->SetRelativeY(0.48f);
			ClearTimeText->SetVisibility(true);
		}

		// 5. 복귀 안내: "[ Enter ]  시작화면 가기"
		if (ReturnPromptText)
		{
			ReturnPromptText->SetText("[ Enter ]  시작화면 가기");
			ReturnPromptText->SetFontSize(28.0f);
			ReturnPromptText->SetColor(FColor(255, 255, 255, 255));
			ReturnPromptText->SetCenterHorizontally(true);
			ReturnPromptText->SetRelativeX(0.25f);
			ReturnPromptText->SetRelativeY(0.64f);
			ReturnPromptText->SetVisibility(true);
		}

		UpdateClearTimeDisplay();
	}
	else
	{
		// 1. 성공 배경 텍스처 (Assets/Models/Background/SuccessEndingBackground.png)
		if (BackgroundImage)
		{
			UTexture2D* SuccessTex = UAssetManager::Get().LoadTexture("Assets/Models/Background/SuccessEndingBackground.png");
			if (SuccessTex)
			{
				BackgroundImage->SetTexture(SuccessTex);
			}
			BackgroundImage->SetColor(FColor(255, 255, 255, 255));
		}

		// 2. 타이틀: "TEST COMPLETE"
		if (TitleText)
		{
			TitleText->SetText("TEST COMPLETE");
			TitleText->SetFontSize(64.0f);
			TitleText->SetColor(FColor(255, 255, 255, 255));
			TitleText->SetCenterHorizontally(true);
			TitleText->SetRelativeX(0.25f);
			TitleText->SetRelativeY(0.28f);
			TitleText->SetVisibility(true);
		}

		// 3. 서브타이틀: "모든 테스트 챔버를 탈출하였습니다."
		if (SubtitleText)
		{
			SubtitleText->SetText("모든 테스트 챔버를 탈출하였습니다.");
			SubtitleText->SetFontSize(22.0f);
			SubtitleText->SetColor(FColor(200, 200, 200, 255));
			SubtitleText->SetCenterHorizontally(true);
			SubtitleText->SetRelativeX(0.25f);
			SubtitleText->SetRelativeY(0.38f);
			SubtitleText->SetVisibility(true);
		}

		// 4. 시간: "탈출 시간: 00:00.00"
		if (ClearTimeText)
		{
			ClearTimeText->SetFontSize(38.0f);
			ClearTimeText->SetColor(FColor(255, 255, 255, 255));
			ClearTimeText->SetCenterHorizontally(true);
			ClearTimeText->SetRelativeX(0.25f);
			ClearTimeText->SetRelativeY(0.50f);
			ClearTimeText->SetVisibility(true);
		}

		// 5. 복귀 안내: "[ Enter ]  시작 화면으로"
		if (ReturnPromptText)
		{
			ReturnPromptText->SetText("[ Enter ]  시작 화면으로");
			ReturnPromptText->SetFontSize(28.0f);
			ReturnPromptText->SetColor(FColor(255, 255, 255, 255));
			ReturnPromptText->SetCenterHorizontally(true);
			ReturnPromptText->SetRelativeX(0.25f);
			ReturnPromptText->SetRelativeY(0.66f);
			ReturnPromptText->SetVisibility(true);
		}

		UpdateClearTimeDisplay();
	}
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

void AEndingCanvasActor::UpdateClearTimeDisplay()
{
	int32 TotalSeconds = static_cast<int32>(ClearTime);
	int32 Minutes = TotalSeconds / 60;
	int32 Seconds = TotalSeconds % 60;
	int32 Hundredths = static_cast<int32>((ClearTime - TotalSeconds) * 100.0f);
	char Buf[64];
	if (CurrentEndingType == EEndingType::Fail)
	{
		std::snprintf(Buf, sizeof(Buf), "걸린 시간: %02d:%02d.%02d", Minutes, Seconds, Hundredths);
	}
	else
	{
		std::snprintf(Buf, sizeof(Buf), "탈출 시간: %02d:%02d.%02d", Minutes, Seconds, Hundredths);
	}
	SetClearTimeText(Buf);
}

void AEndingCanvasActor::SetClearTime(float InTime)
{
	ClearTime = InTime;
	UpdateClearTimeDisplay();
}

void AEndingCanvasActor::SetClearTimeText(const FString& InText)
{
	if (ClearTimeText)
	{
		ClearTimeText->SetText(InText);
	}
}
