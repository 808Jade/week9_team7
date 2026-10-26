#include "EnginePCH.h"
#include "HUDCanvasActor.h"
#include "Components/UIImageComponent.h"
#include "Components/UITextComponent.h"
#include "Input/InputSystem.h"
#include "GameFramework/Actor/UIManagerActor.h"

AHUDCanvasActor::AHUDCanvasActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. 크로스헤어 (중앙 조준점 기본 배치, Layer 10)
	CrosshairImage = CreateDefaultSubobject<UUIImageComponent>("CrosshairImage");
	CrosshairImage->SetupAttachment(GetRootComponent());
	CrosshairImage->SetPosition(954.0f, 534.0f); // 1920x1080 기준 정중앙 기본값
	CrosshairImage->SetSize(12.0f, 12.0f);
	CrosshairImage->SetColor(FColor(255, 255, 255, 220));
	CrosshairImage->SetLayer(1);

	// 2. 상호작용 안내 텍스트 (조준선 아래 기본 배치, Layer 20)
	InteractPromptText = CreateDefaultSubobject<UUITextComponent>("InteractPromptText");
	InteractPromptText->SetupAttachment(GetRootComponent());
	InteractPromptText->SetPosition(860.0f, 570.0f);
	InteractPromptText->SetFontSize(22.0f);
	InteractPromptText->SetColor(FColor(255, 255, 255, 255));
	InteractPromptText->SetLayer(2);
	InteractPromptText->SetVisibility(false);

	// 3. 자막/대사 텍스트 (화면 하단 기본 배치, Layer 20)
	SubtitleText = CreateDefaultSubobject<UUITextComponent>("SubtitleText");
	SubtitleText->SetupAttachment(GetRootComponent());
	SubtitleText->SetPosition(700.0f, 960.0f);
	SubtitleText->SetFontSize(24.0f);
	SubtitleText->SetColor(FColor(255, 230, 100, 255));
	SubtitleText->SetLayer(2);
	SubtitleText->SetVisibility(false);

	// 4. 전체 화면 암전/페이드 오버레이 (최상단, Layer 99)
	FadeImage = CreateDefaultSubobject<UUIImageComponent>("FadeImage");
	FadeImage->SetupAttachment(GetRootComponent());
	FadeImage->SetPosition(0.0f, 0.0f);
	FadeImage->SetSize(1920.0f, 1080.0f);
	FadeImage->SetColor(FColor(0, 0, 0, 255));
	FadeImage->SetOpacity(0.0f);
	FadeImage->SetLayer(99);
	FadeImage->SetVisibility(false);
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
		if (InteractPromptText) InteractPromptText->SetVisibility(bShowInteractPrompt);
		if (SubtitleText) SubtitleText->SetVisibility(SubtitleRemainingTime > 0.0f);
		if (FadeImage) FadeImage->SetVisibility(CurrentFadeOpacity > 0.001f);
	}
	else
	{
		if (CrosshairImage) CrosshairImage->SetVisibility(false);
		if (InteractPromptText) InteractPromptText->SetVisibility(false);
		if (SubtitleText) SubtitleText->SetVisibility(false);
		if (FadeImage) FadeImage->SetVisibility(false);
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