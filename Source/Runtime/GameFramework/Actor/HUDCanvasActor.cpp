#include "EnginePCH.h"
#include "HUDCanvasActor.h"
#include "Components/UIImageComponent.h"
#include "Components/UITextComponent.h"
#include "Input/InputSystem.h"
#include "GameFramework/Actor/UIManagerActor.h"
#include "Asset/AssetManager.h"
#include <cstdio>

AHUDCanvasActor::AHUDCanvasActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. 포탈 크로스헤어 (뷰포트 정중앙 자동 배치, Layer 10)
	CrosshairImage = CreateDefaultSubobject<UUIImageComponent>("CrosshairImage");
	CrosshairImage->SetupAttachment(GetRootComponent());
	CrosshairImage->SetSize(150.0f, 250.0f);
	CrosshairImage->SetCenterOnViewport(true); // 항상 화면 정중앙
	CrosshairImage->SetColor(FColor(255, 255, 255, 255));
	CrosshairImage->SetLayer(10);
	if (UTexture2D* InitTex = UAssetManager::Get().LoadTexture("Assets/Models/Crosshair/crosshair_unfilled.png"))
	{
		CrosshairImage->SetTexture(InitTex);
	}

	// 2. 상단 중앙 타이머 텍스트 (화면 상단 4% 위치 가로 중앙 정렬, Layer 20)
	TimerText = CreateDefaultSubobject<UUITextComponent>("HUDTimerText");
	TimerText->SetupAttachment(GetRootComponent());
	TimerText->SetText("00:00.00");
	TimerText->SetFontSize(32.0f);
	TimerText->SetCenterHorizontally(true);
	TimerText->SetRelativeY(0.04f); // 화면 맨 상단 중앙
	TimerText->SetColor(FColor(255, 255, 255, 255)); // 하얀색
	TimerText->SetLayer(20);
	TimerText->SetVisibility(true);

	// 3. 상호작용 안내 텍스트 (조준선 아래 중앙 배치, Layer 20)
	InteractPromptText = CreateDefaultSubobject<UUITextComponent>("InteractPromptText");
	InteractPromptText->SetupAttachment(GetRootComponent());
	InteractPromptText->SetFontSize(22.0f);
	InteractPromptText->SetCenterHorizontally(true);
	InteractPromptText->SetRelativeY(0.56f);
	InteractPromptText->SetColor(FColor(255, 255, 255, 255));
	InteractPromptText->SetLayer(20);
	InteractPromptText->SetVisibility(false);

	// 4. 자막/대사 텍스트 (화면 하단 중앙 배치, Layer 20)
	SubtitleText = CreateDefaultSubobject<UUITextComponent>("SubtitleText");
	SubtitleText->SetupAttachment(GetRootComponent());
	SubtitleText->SetFontSize(24.0f);
	SubtitleText->SetCenterHorizontally(true);
	SubtitleText->SetRelativeY(0.88f);
	SubtitleText->SetColor(FColor(255, 230, 100, 255));
	SubtitleText->SetLayer(20);
	SubtitleText->SetVisibility(false);

	// 5. 전체 화면 암전/페이드 오버레이 (뷰포트 전체 자동 일치, Layer 99)
	FadeImage = CreateDefaultSubobject<UUIImageComponent>("FadeImage");
	FadeImage->SetupAttachment(GetRootComponent());
	FadeImage->SetMatchViewportSize(true);
	FadeImage->SetColor(FColor(0, 0, 0, 255));
	FadeImage->SetOpacity(0.0f);
	FadeImage->SetLayer(99);
	FadeImage->SetVisibility(false);
}

void AHUDCanvasActor::BeginPlay()
{
	Super::BeginPlay();

	// 크로스헤어 텍스처 4종 로드 및 캐싱
	CrosshairTexUnfilled = UAssetManager::Get().LoadTexture("Assets/Models/Crosshair/crosshair_unfilled.png");
	CrosshairTexBlue = UAssetManager::Get().LoadTexture("Assets/Models/Crosshair/crosshair_blue.png");
	CrosshairTexOrange = UAssetManager::Get().LoadTexture("Assets/Models/Crosshair/crosshair_orange.png");
	CrosshairTexBoth = UAssetManager::Get().LoadTexture("Assets/Models/Crosshair/crosshair_both.png");

	SetCrosshairState(ECrosshairState::Unfilled);
}

void AHUDCanvasActor::SetCrosshairState(ECrosshairState InState)
{
	CurrentCrosshairState = InState;
	if (!CrosshairImage)
	{
		return;
	}

	UTexture2D* TargetTex = nullptr;
	switch (InState)
	{
	case ECrosshairState::Blue:
		TargetTex = CrosshairTexBlue ? CrosshairTexBlue : UAssetManager::Get().LoadTexture("Assets/Models/Crosshair/crosshair_blue.png");
		break;
	case ECrosshairState::Orange:
		TargetTex = CrosshairTexOrange ? CrosshairTexOrange : UAssetManager::Get().LoadTexture("Assets/Models/Crosshair/crosshair_orange.png");
		break;
	case ECrosshairState::Both:
		TargetTex = CrosshairTexBoth ? CrosshairTexBoth : UAssetManager::Get().LoadTexture("Assets/Models/Crosshair/crosshair_both.png");
		break;
	case ECrosshairState::Unfilled:
	default:
		TargetTex = CrosshairTexUnfilled ? CrosshairTexUnfilled : UAssetManager::Get().LoadTexture("Assets/Models/Crosshair/crosshair_unfilled.png");
		break;
	}

	if (TargetTex)
	{
		CrosshairImage->SetTexture(TargetTex);
	}
}

void AHUDCanvasActor::SetCrosshairStateByName(const FString& InStateName)
{
	if (_stricmp(InStateName.c_str(), "blue") == 0)
	{
		SetCrosshairState(ECrosshairState::Blue);
	}
	else if (_stricmp(InStateName.c_str(), "orange") == 0)
	{
		SetCrosshairState(ECrosshairState::Orange);
	}
	else if (_stricmp(InStateName.c_str(), "both") == 0)
	{
		SetCrosshairState(ECrosshairState::Both);
	}
	else
	{
		SetCrosshairState(ECrosshairState::Unfilled);
	}
}

FString AHUDCanvasActor::GetCrosshairStateName() const
{
	switch (CurrentCrosshairState)
	{
	case ECrosshairState::Blue: return "blue";
	case ECrosshairState::Orange: return "orange";
	case ECrosshairState::Both: return "both";
	case ECrosshairState::Unfilled:
	default: return "unfilled";
	}
}

void AHUDCanvasActor::SetCrosshairSize(float InWidth, float InHeight)
{
	if (CrosshairImage)
	{
		CrosshairImage->SetSize(InWidth, InHeight);
	}
}

void AHUDCanvasActor::SetCrosshairVisibility(bool bVisible)
{
	if (CrosshairImage)
	{
		CrosshairImage->SetVisibility(bVisible);
	}
}

void AHUDCanvasActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 인게임 중 ESC 누르면 -> 일시정지 모드로 전환
	if (IsCanvasVisible() && FInputSystem::IsKeyPressed(EKeyCode::Escape))
	{
		if (AUIManagerActor::Get())
		{
			AUIManagerActor::Get()->SetUIMode(EUIMode::Pause);
			return;
		}
	}

	// 게임 진행 시간 실시간 누적 및 상단 타이머 갱신
	if (IsCanvasVisible() && bTimerRunning)
	{
		ElapsedTime += DeltaTime;
		UpdateTimerDisplay();
	}

	// 자막 자동 숨김 타이머
	if (SubtitleRemainingTime > 0.0f)
	{
		SubtitleRemainingTime -= DeltaTime;
		if (SubtitleRemainingTime <= 0.0f)
		{
			HideSubtitle();
		}
	}

	// 페이드 애니메이션 보간 (Lerp)
	if (bIsFading)
	{
		FadeElapsed += DeltaTime;
		float Alpha = (FadeDuration > 0.0f) ? (FadeElapsed / FadeDuration) : 1.0f;
		if (Alpha >= 1.0f)
		{
			Alpha = 1.0f;
			bIsFading = false;
		}

		CurrentFadeOpacity = StartFadeOpacity + (TargetFadeOpacity - StartFadeOpacity) * Alpha;
		if (FadeImage)
		{
			FadeImage->SetOpacity(CurrentFadeOpacity);
			FadeImage->SetVisibility(CurrentFadeOpacity > 0.001f);
		}
	}
}

void AHUDCanvasActor::SetCanvasVisibility(bool bVisible)
{
	Super::SetCanvasVisibility(bVisible);

	if (bVisible)
	{
		if (CrosshairImage) CrosshairImage->SetVisibility(true);
		if (TimerText) TimerText->SetVisibility(bShowTimer);
		if (InteractPromptText) InteractPromptText->SetVisibility(bShowInteractPrompt);
		if (SubtitleText) SubtitleText->SetVisibility(SubtitleRemainingTime > 0.0f);
		if (FadeImage) FadeImage->SetVisibility(CurrentFadeOpacity > 0.001f);
	}
	else
	{
		if (CrosshairImage) CrosshairImage->SetVisibility(false);
		if (TimerText) TimerText->SetVisibility(false);
		if (InteractPromptText) InteractPromptText->SetVisibility(false);
		if (SubtitleText) SubtitleText->SetVisibility(false);
		if (FadeImage) FadeImage->SetVisibility(false);
	}
}

FString AHUDCanvasActor::GetFormattedTime() const
{
	int32 TotalSeconds = static_cast<int32>(ElapsedTime);
	int32 Minutes = TotalSeconds / 60;
	int32 Seconds = TotalSeconds % 60;
	int32 Hundredths = static_cast<int32>((ElapsedTime - TotalSeconds) * 100.0f);
	char Buf[32];
	std::snprintf(Buf, sizeof(Buf), "%02d:%02d.%02d", Minutes, Seconds, Hundredths);
	return FString(Buf);
}

void AHUDCanvasActor::UpdateTimerDisplay()
{
	if (TimerText && bShowTimer)
	{
		TimerText->SetText(GetFormattedTime());
	}
}

void AHUDCanvasActor::SetTimerText(const FString& InText)
{
	if (TimerText)
	{
		TimerText->SetText(InText);
	}
}

void AHUDCanvasActor::SetTimerVisibility(bool bVisible)
{
	bShowTimer = bVisible;
	if (TimerText)
	{
		TimerText->SetVisibility(bVisible && IsCanvasVisible());
	}
}

void AHUDCanvasActor::ShowInteractPrompt(const FString& InText)
{
	bShowInteractPrompt = true;
	if (InteractPromptText)
	{
		InteractPromptText->SetText(InText);
		InteractPromptText->SetVisibility(IsCanvasVisible());
	}
}

void AHUDCanvasActor::HideInteractPrompt()
{
	bShowInteractPrompt = false;
	if (InteractPromptText)
	{
		InteractPromptText->SetVisibility(false);
	}
}

void AHUDCanvasActor::ShowSubtitle(const FString& InText, float Duration)
{
	SubtitleRemainingTime = Duration;
	if (SubtitleText)
	{
		SubtitleText->SetText(InText);
		SubtitleText->SetVisibility(IsCanvasVisible());
	}
}

void AHUDCanvasActor::HideSubtitle()
{
	SubtitleRemainingTime = 0.0f;
	if (SubtitleText)
	{
		SubtitleText->SetVisibility(false);
	}
}

void AHUDCanvasActor::StartFade(float TargetOpacity, float Duration)
{
	StartFadeOpacity = CurrentFadeOpacity;
	TargetFadeOpacity = TargetOpacity;
	FadeDuration = Duration;
	FadeElapsed = 0.0f;
	bIsFading = true;

	if (FadeImage && (StartFadeOpacity > 0.001f || TargetFadeOpacity > 0.001f))
	{
		FadeImage->SetVisibility(true);
	}
}
