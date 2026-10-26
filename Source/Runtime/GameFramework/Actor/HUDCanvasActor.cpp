#include "EnginePCH.h"
#include "HUDCanvasActor.h"
#include "Components/UIImageComponent.h"
#include "Components/UITextComponent.h"
#include "Input/InputSystem.h"
#include "GameFramework/Actor/UIManagerActor.h"
#include "GameFramework/Actor/EndingCanvasActor.h"
#include "Asset/AssetManager.h"
#include <cstdio>
#include <imgui.h>
#include "Engine/Engine.h"
#include "Launch/LaunchEngineLoop.h"
#include "GameFramework/PortalPlayerController.h"
#include "GameFramework/PortalPawn.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"

#define U8(str) reinterpret_cast<const char*>(u8##str)

AHUDCanvasActor::AHUDCanvasActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. 포탈 크로스헤어 (뷰포트 정중앙 자동 배치, Layer 10)
	CrosshairImage = CreateDefaultSubobject<UUIImageComponent>("CrosshairImage");
	CrosshairImage->SetupAttachment(GetRootComponent());
	CrosshairImage->SetSize(100.0f, 200.0f);
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

	if (CrosshairImage)
	{
		CrosshairImage->SetSize(120.0f, 220.0f);
		CrosshairImage->SetCenterOnViewport(true);
	}

	SetCrosshairState(ECrosshairState::Unfilled);
	ApplyMouseSensitivity();
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

void AHUDCanvasActor::SetSettingsWindowVisible(bool bVisible)
{
	bShowSettingsWindow = bVisible;

	ImGuiIO& io = ImGui::GetIO();
	io.MouseDrawCursor = bShowSettingsWindow;

	if (bShowSettingsWindow)
	{
		// 설정창 열림: 마우스 커서 표시 및 잠금 해제
		if (GetWorld())
		{
			if (APortalPlayerController* PPC = Cast<APortalPlayerController>(GetWorld()->GetPlayerController()))
			{
				PPC->SetMouseCapture(false);
			}
		}
		while (::ShowCursor(TRUE) < 1);
		::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
	}
	else
	{
		// 설정창 닫힘 (게임 복귀): 마우스 잠금 및 커서 숨김
		if (GetWorld())
		{
			if (APortalPlayerController* PPC = Cast<APortalPlayerController>(GetWorld()->GetPlayerController()))
			{
				PPC->SetMouseCapture(true);
			}
		}
		while (::ShowCursor(FALSE) >= 0);
	}
}

void AHUDCanvasActor::ApplyMouseSensitivity()
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (APlayerController* PC = World->GetPlayerController())
	{
		PC->SetLookSensitivity(MouseSensitivity);
	}
	if (APortalPawn* Pawn = Cast<APortalPawn>(World->GetPlayerPawn()))
	{
		Pawn->SetLookSensitivity(MouseSensitivity * 10.0f);
	}
	if (ACameraActor* Cam = World->GetMainCamera())
	{
		if (UCameraComponent* CamCom = Cam->GetCameraComponent())
		{
			CamCom->SetMouseSensitivity(MouseSensitivity);
		}
	}
}

void AHUDCanvasActor::DrawSettingsWindow()
{
	float ViewportW = 1280.0f;
	float ViewportH = 720.0f;
	if (GEngine)
	{
		ViewportW = static_cast<float>(GEngine->GetEngineLoop().GetViewportWidth());
		ViewportH = static_cast<float>(GEngine->GetEngineLoop().GetViewportHeight());
	}

	ImVec2 Center(ViewportW * 0.5f, ViewportH * 0.5f);
	ImGui::SetNextWindowPos(Center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(440.0f, 240.0f), ImGuiCond_Always);

	ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	if (ImGui::Begin(U8("설정 (Settings)"), nullptr, WindowFlags))
	{
		ImGui::Dummy(ImVec2(0.0f, 5.0f));
		ImGui::Text(U8("마우스 조작 설정"));
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0.0f, 5.0f));

		// 마우스 감도 슬라이더
		ImGui::Text(U8("마우스 감도:"));
		ImGui::SameLine();
		if (ImGui::SliderFloat("##MouseSensitivity", &MouseSensitivity, 0.01f, 1.0f, "%.2f"))
		{
			ApplyMouseSensitivity();
		}

		ImGui::Dummy(ImVec2(0.0f, 15.0f));
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		// 버튼 배치
		float ButtonWidth = 185.0f;
		float ButtonHeight = 38.0f;

		if (ImGui::Button(U8("닫기 (계속하기)"), ImVec2(ButtonWidth, ButtonHeight)))
		{
			SetSettingsWindowVisible(false);
		}

		ImGui::SameLine(0.0f, 20.0f);

		// 그만두기 버튼은 붉은색 계열로 강조 (엔딩 실패 화면으로 전환)
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.20f, 0.20f, 1.00f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.25f, 0.25f, 1.00f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.55f, 0.15f, 0.15f, 1.00f));
		if (ImGui::Button(U8("그만두기"), ImVec2(ButtonWidth, ButtonHeight)))
		{
			SetSettingsWindowVisible(false);
			if (AUIManagerActor::Get())
			{
				if (AEndingCanvasActor* EndingCanvas = AUIManagerActor::Get()->GetEndingCanvas())
				{
					EndingCanvas->SetEndingType(EEndingType::Fail);
				}
				AUIManagerActor::Get()->SetUIMode(EUIMode::Ending);
			}
		}
		ImGui::PopStyleColor(3);
	}
	ImGui::End();
}

void AHUDCanvasActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ESC 키 입력 시 설정창 토글
	if ((IsCanvasVisible() || bShowSettingsWindow) && FInputSystem::IsKeyPressed(EKeyCode::Escape))
	{
		SetSettingsWindowVisible(!bShowSettingsWindow);
	}

	// 설정창이 열려있을 때: ImGui 렌더링 및 커서 보장
	if (bShowSettingsWindow)
	{
		ImGui::GetIO().MouseDrawCursor = true;
		::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
		DrawSettingsWindow();
		return;
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
