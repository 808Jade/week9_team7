#include "EnginePCH.h"
#include "TitleCanvasActor.h"
#include "Components/UIImageComponent.h"
#include "Components/UITextComponent.h"
#include "Input/InputSystem.h"
#include "GameFramework/Actor/UIManagerActor.h"
#include "Asset/AssetManager.h"
#include "Launch/LaunchEngineLoop.h"
#include "Engine/Engine.h"
#include "Text/Font.h"

namespace
{
	// ───────── 레이아웃 상수 (뷰포트 비율) ─────────
	constexpr float LeftMargin      = 0.07f;  // 로고/메뉴 왼쪽 정렬선
	constexpr float LogoRelY        = 0.09f;
	constexpr float LogoFontSize    = 96.0f;

	constexpr float MenuTopRelY     = 0.50f;  // 첫 메뉴 항목 위치
	constexpr float MenuStepRelY    = 0.058f; // 메뉴 항목 간격
	constexpr float MenuFontSize    = 50.0f;
	constexpr float SelectionRelW   = 0.26f;  // 선택 바 가로 길이 (뷰포트 비율)
	constexpr float SelectionPadX   = 18.0f;  // 선택 바가 텍스트 왼쪽으로 튀어나오는 픽셀
	constexpr float SelectionLerp   = 18.0f;  // 선택 바 이동 보간 속도

	const FColor PortalBlue      = FColor(0, 162, 255, 255);
	const FColor LogoColor       = FColor(228, 228, 228, 255);
	const FColor MenuNormalColor = FColor(200, 200, 200, 255);
	const FColor MenuActiveColor = FColor(255, 255, 255, 255);
	const FColor SelectionColor  = FColor(120, 120, 120, 165);

	// 포탈 2 메뉴처럼 글자 사이를 띄워 둔 문자열 (폰트에 자간 옵션이 없어서 공백으로 표현)
	const char* MenuLabels[] =
	{
		"싱 글   플 레 이",
		"제 작 진",
		"종 료",
	};

	const char* TeamLabel = "TEAM 7";
	const char* CreditLabels[] =
	{
		"김태훈, 황성민, 김종곤, 구민상",
		"",
		"[ ESC ]  닫기",
	};

	float LerpF(float A, float B, float T) { return A + (B - A) * T; }
}

ATitleCanvasActor::ATitleCanvasActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. 배경 이미지 (Layer 5)
	BackgroundImage = CreateDefaultSubobject<UUIImageComponent>("TitleBackground");
	BackgroundImage->SetupAttachment(GetRootComponent());

	// 2. 전체 디밍 + 좌측 그라데이션 (Layer 6~7)
	DimOverlay = CreateDefaultSubobject<UUIImageComponent>("TitleDimOverlay");
	DimOverlay->SetupAttachment(GetRootComponent());

	for (int32 i = 0; i < GradientStripCount; ++i)
	{
		const FString Name = "TitleGradient" + std::to_string(i);
		GradientStrips[i] = CreateDefaultSubobject<UUIImageComponent>(Name);
		GradientStrips[i]->SetupAttachment(GetRootComponent());
	}

	// 3. 로고
	LogoText = CreateDefaultSubobject<UUITextComponent>("TitleLogoText");
	LogoText->SetupAttachment(GetRootComponent());
	LogoDivider = CreateDefaultSubobject<UUIImageComponent>("TitleLogoDivider");
	LogoDivider->SetupAttachment(GetRootComponent());
	LogoNumber = CreateDefaultSubobject<UUITextComponent>("TitleLogoNumber");
	LogoNumber->SetupAttachment(GetRootComponent());

	// 4. 메뉴
	SelectionBar = CreateDefaultSubobject<UUIImageComponent>("TitleSelectionBar");
	SelectionBar->SetupAttachment(GetRootComponent());

	for (int32 i = 0; i < MenuCount; ++i)
	{
		const FString Name = "TitleMenuItem" + std::to_string(i);
		MenuTexts[i] = CreateDefaultSubobject<UUITextComponent>(Name);
		MenuTexts[i]->SetupAttachment(GetRootComponent());
	}

	FooterText = CreateDefaultSubobject<UUITextComponent>("TitleFooterText");
	FooterText->SetupAttachment(GetRootComponent());
	HintText = CreateDefaultSubobject<UUITextComponent>("TitleHintText");
	HintText->SetupAttachment(GetRootComponent());

	// 5. 제작진 패널
	CreditsPanel = CreateDefaultSubobject<UUIImageComponent>("TitleCreditsPanel");
	CreditsPanel->SetupAttachment(GetRootComponent());
	CreditsAccent = CreateDefaultSubobject<UUIImageComponent>("TitleCreditsAccent");
	CreditsAccent->SetupAttachment(GetRootComponent());
	CreditsTitle = CreateDefaultSubobject<UUITextComponent>("TitleCreditsTitle");
	CreditsTitle->SetupAttachment(GetRootComponent());
	for (int32 i = 0; i < CreditLineCount; ++i)
	{
		const FString Name = "TitleCreditsLine" + std::to_string(i);
		CreditsLines[i] = CreateDefaultSubobject<UUITextComponent>(Name);
		CreditsLines[i]->SetupAttachment(GetRootComponent());
	}

	ApplyLayout();
}

void ATitleCanvasActor::RemoveLegacyComponents()
{
	// 예전 타이틀 화면 컴포넌트가 씬 파일(.scene)에 저장돼 있으면 로드 시 다시 생성되므로 제거한다
	static const char* LegacyNames[] =
	{
		"TitleSubtitleText",
		"TitleStartPrompt",
		"TitleQuitPrompt",
	};
	for (const char* LegacyName : LegacyNames)
	{
		if (UActorComponent* Legacy = FindComponentByName(FName(LegacyName)))
		{
			DestroyComponent(Legacy);
		}
	}
}

void ATitleCanvasActor::BeginPlay()
{
	RemoveLegacyComponents();
	Super::BeginPlay();

	// 씬 파일에 예전 값이 저장돼 있어도 항상 코드 기준 레이아웃으로 덮어쓴다
	ApplyLayout();
	SelectedIndex = 0;
	SelectionBarRelY = GetMenuItemRelY(0);
	bShowCredits = false;
}

float ATitleCanvasActor::GetMenuItemRelY(int32 Index) const
{
	return MenuTopRelY + MenuStepRelY * static_cast<float>(Index);
}

void ATitleCanvasActor::ApplyLayout()
{
	// 포탈 2 메뉴 느낌의 얇은 산세리프 폰트 (없으면 기본 폰트 유지)
	MenuFont = UAssetManager::GetAssetByPath<UFont>("Fonts/Pretendard.json");
	auto ApplyFont = [this](UUITextComponent* Text)
	{
		if (Text && MenuFont) Text->SetFont(MenuFont);
	};

	// ── 배경 ──
	if (BackgroundImage)
	{
		BackgroundImage->SetMatchViewportSize(true);
		if (UTexture2D* BgTex = UAssetManager::Get().LoadTexture("Assets/Models/Background/TitleBackground.png"))
		{
			BackgroundImage->SetTexture(BgTex);
		}
		BackgroundImage->SetColor(FColor(255, 255, 255, 255));
		BackgroundImage->SetLayer(5);
	}

	if (DimOverlay)
	{
		DimOverlay->SetMatchViewportSize(true);
		DimOverlay->SetColor(FColor(0, 4, 10, 95)); // 살짝 푸른 기가 도는 어두운 톤
		DimOverlay->SetLayer(6);
	}

	// 좌측일수록 겹치는 띠가 많아져서 더 어두워지는 계단식 그라데이션
	for (int32 i = 0; i < GradientStripCount; ++i)
	{
		UUIImageComponent* Strip = GradientStrips[i];
		if (!Strip) continue;
		const float T = static_cast<float>(i) / static_cast<float>(GradientStripCount);
		Strip->SetRelativeRect(0.0f, 0.0f, 0.62f * (1.0f - T), 1.0f);
		Strip->SetColor(FColor(0, 0, 0, 28));
		Strip->SetLayer(7);
	}

	// ── 로고 ──
	if (LogoText)
	{
		ApplyFont(LogoText);
		LogoText->SetText("PORTAL");
		LogoText->SetFontSize(LogoFontSize);
		LogoText->SetCenterHorizontally(false);
		LogoText->SetCenterVertically(false);
		LogoText->SetRelativeX(LeftMargin);
		LogoText->SetRelativeY(LogoRelY);
		LogoText->SetColor(LogoColor);
		LogoText->SetLayer(20);
	}

	if (LogoDivider)
	{
		LogoDivider->SetSize(6.0f, LogoFontSize * 0.72f);
		LogoDivider->SetColor(PortalBlue);
		LogoDivider->SetLayer(20);
	}

	if (LogoNumber)
	{
		ApplyFont(LogoNumber);
		LogoNumber->SetText("3");
		LogoNumber->SetFontSize(LogoFontSize);
		LogoNumber->SetCenterHorizontally(false);
		LogoNumber->SetCenterVertically(false);
		LogoNumber->SetRelativeY(LogoRelY);
		LogoNumber->SetColor(LogoColor);
		LogoNumber->SetLayer(20);
	}
	UpdateLogoLayout();

	// ── 메뉴 ──
	if (SelectionBar)
	{
		SelectionBar->SetRelativeRect(LeftMargin, GetMenuItemRelY(SelectedIndex), SelectionRelW, -1.0f,
			-SelectionPadX, -8.0f);
		SelectionBar->SetSize(1.0f, MenuFontSize * 1.19f + 6.0f);
		SelectionBar->SetColor(SelectionColor);
		SelectionBar->SetLayer(15);
	}

	for (int32 i = 0; i < MenuCount; ++i)
	{
		UUITextComponent* Item = MenuTexts[i];
		if (!Item) continue;
		ApplyFont(Item);
		Item->SetText(MenuLabels[i]);
		Item->SetFontSize(MenuFontSize);
		Item->SetCenterHorizontally(false);
		Item->SetCenterVertically(false);
		Item->SetRelativeX(LeftMargin);
		Item->SetRelativeY(GetMenuItemRelY(i));
		Item->SetColor(MenuNormalColor);
		Item->SetLayer(20);
	}

	if (FooterText)
	{
		ApplyFont(FooterText);
		FooterText->SetText(FString(TeamLabel) + "     Aperture Science Enrichment Center");
		FooterText->SetFontSize(18.0f);
		FooterText->SetCenterHorizontally(false);
		FooterText->SetRelativeX(LeftMargin);
		FooterText->SetRelativeY(0.90f);
		FooterText->SetColor(FColor(150, 160, 170, 255));
		FooterText->SetLayer(20);
	}

	if (HintText)
	{
		ApplyFont(HintText);
		HintText->SetText("[ W / S ]  이동      [ Enter ]  선택");
		HintText->SetFontSize(16.0f);
		HintText->SetCenterHorizontally(true);
		HintText->SetRelativeX(0.86f);
		HintText->SetRelativeY(0.92f);
		HintText->SetColor(FColor(140, 140, 140, 255));
		HintText->SetLayer(20);
	}

	// ── 제작진 패널 (메뉴 오른쪽) ──
	constexpr float PanelX = 0.36f;
	constexpr float PanelY = 0.47f;
	constexpr float PanelW = 0.27f;
	constexpr float PanelH = 0.32f;

	if (CreditsPanel)
	{
		CreditsPanel->SetRelativeRect(PanelX, PanelY, PanelW, PanelH);
		CreditsPanel->SetColor(FColor(8, 10, 14, 185));
		CreditsPanel->SetLayer(15);
	}
	if (CreditsAccent)
	{
		CreditsAccent->SetRelativeRect(PanelX, PanelY, -1.0f, PanelH);
		CreditsAccent->SetSize(4.0f, 1.0f);
		CreditsAccent->SetColor(PortalBlue);
		CreditsAccent->SetLayer(16);
	}
	if (CreditsTitle)
	{
		ApplyFont(CreditsTitle);
		CreditsTitle->SetText(TeamLabel);
		CreditsTitle->SetFontSize(60.0f);
		CreditsTitle->SetCenterHorizontally(false);
		CreditsTitle->SetRelativeX(PanelX + 0.02f);
		CreditsTitle->SetRelativeY(PanelY + 0.03f);
		CreditsTitle->SetColor(PortalBlue);
		CreditsTitle->SetLayer(20);
	}
	for (int32 i = 0; i < CreditLineCount; ++i)
	{
		UUITextComponent* Line = CreditsLines[i];
		if (!Line) continue;
		ApplyFont(Line);
		Line->SetText(CreditLabels[i]);
		Line->SetFontSize(i == CreditLineCount - 1 ? 30.0f : 40.0f);
		Line->SetCenterHorizontally(false);
		Line->SetRelativeX(PanelX + 0.02f);
		Line->SetRelativeY(PanelY + 0.11f + 0.042f * static_cast<float>(i));
		Line->SetColor(i == CreditLineCount - 1 ? FColor(140, 140, 140, 255) : FColor(220, 220, 220, 255));
		Line->SetLayer(20);
	}
}

void ATitleCanvasActor::UpdateLogoLayout()
{
	if (!LogoText) return;

	// "PORTAL" 텍스트 실제 폭을 기준으로 막대와 숫자를 옆에 붙인다
	const float LogoWidth = LogoText->GetSize().X;
	const float DividerGap = LogoFontSize * 0.16f;

	if (LogoDivider)
	{
		// 대문자 높이(약 0.72em)에 맞춰 세로 위치 정렬
		LogoDivider->SetRelativeRect(LeftMargin, LogoRelY, -1.0f, -1.0f,
			LogoWidth + DividerGap, LogoFontSize * 0.08f);
	}
	if (LogoNumber)
	{
		LogoNumber->SetRelativeX(LeftMargin, LogoWidth + DividerGap * 2.0f + 6.0f);
	}
}

int32 ATitleCanvasActor::FindHoveredItem() const
{
	const float MouseX = static_cast<float>(FInputSystem::GetMouseX());
	const float MouseY = static_cast<float>(FInputSystem::GetMouseY());

	for (int32 i = 0; i < MenuCount; ++i)
	{
		const UUITextComponent* Item = MenuTexts[i];
		if (!Item) continue;

		const FVector4& Rect = Item->GetScreenRect();          // X, Y, W, H
		const FVector4& View = Item->GetScreenViewportSize();  // W, H
		if (Rect.Z <= 0.0f || View.X <= 0.0f) continue;       // 아직 한 번도 그려지지 않음

		const float Left = Rect.X - SelectionPadX;
		const float Right = Left + View.X * SelectionRelW;
		const float Top = Rect.Y - 8.0f;
		const float Bottom = Rect.Y + Rect.W;

		if (MouseX >= Left && MouseX <= Right && MouseY >= Top && MouseY <= Bottom)
		{
			return i;
		}
	}
	return -1;
}

void ATitleCanvasActor::ActivateItem(int32 Index)
{
	switch (static_cast<EMenuItem>(Index))
	{
	case EMenuItem::NewGame:
		bShowCredits = false;
		if (AUIManagerActor::Get())
		{
			AUIManagerActor::Get()->SetUIMode(EUIMode::InGame);
		}
		break;

	case EMenuItem::Credits:
		bShowCredits = !bShowCredits;
		break;

	case EMenuItem::Quit:
		if (GEngine)
		{
			GEngine->GetEngineLoop().RequestExit();
		}
		break;

	default:
		break;
	}
}

void ATitleCanvasActor::HandleInput()
{
	// 키보드 이동
	if (FInputSystem::IsKeyPressed(EKeyCode::Up) || FInputSystem::IsKeyPressed(EKeyCode::W))
	{
		SelectedIndex = (SelectedIndex + MenuCount - 1) % MenuCount;
	}
	else if (FInputSystem::IsKeyPressed(EKeyCode::Down) || FInputSystem::IsKeyPressed(EKeyCode::S))
	{
		SelectedIndex = (SelectedIndex + 1) % MenuCount;
	}

	// 마우스가 움직였을 때만 호버로 선택 변경 (키보드 선택을 덮어쓰지 않도록)
	const int32 MouseX = FInputSystem::GetMouseX();
	const int32 MouseY = FInputSystem::GetMouseY();
	const bool bMouseMoved = (MouseX != LastMouseX || MouseY != LastMouseY);
	LastMouseX = MouseX;
	LastMouseY = MouseY;

	const int32 Hovered = FindHoveredItem();
	if (bMouseMoved && Hovered >= 0)
	{
		SelectedIndex = Hovered;
	}

	// 결정
	if (FInputSystem::IsKeyPressed(EKeyCode::Enter) || FInputSystem::IsKeyPressed(EKeyCode::Space))
	{
		ActivateItem(SelectedIndex);
	}
	else if (Hovered >= 0 && FInputSystem::IsMousePressed(EMouseButton::Left))
	{
		SelectedIndex = Hovered;
		ActivateItem(Hovered);
	}
	else if (FInputSystem::IsKeyPressed(EKeyCode::Escape))
	{
		// ESC: 제작진 패널이 열려 있으면 닫고, 아니면 게임 종료 (기존 동작 유지)
		if (bShowCredits)
		{
			bShowCredits = false;
		}
		else if (GEngine)
		{
			GEngine->GetEngineLoop().RequestExit();
		}
	}
}

void ATitleCanvasActor::UpdateSelectionVisual(float DeltaTime)
{
	const float TargetRelY = GetMenuItemRelY(SelectedIndex);
	if (SelectionBarRelY < 0.0f)
	{
		SelectionBarRelY = TargetRelY;
	}

	// 지수 감쇠 보간 (프레임레이트 독립)
	const float Alpha = 1.0f - expf(-SelectionLerp * DeltaTime);
	SelectionBarRelY = LerpF(SelectionBarRelY, TargetRelY, Alpha);

	if (SelectionBar)
	{
		SelectionBar->SetRelativeY(SelectionBarRelY);
		SelectionBar->SetOpacity(FadeInAlpha);
	}

	for (int32 i = 0; i < MenuCount; ++i)
	{
		if (!MenuTexts[i]) continue;
		const bool bSelected = (i == SelectedIndex);
		MenuTexts[i]->SetColor(bSelected ? MenuActiveColor : MenuNormalColor);
		MenuTexts[i]->SetOpacity(FadeInAlpha * (bSelected ? 1.0f : 0.85f));
	}

	if (LogoText) LogoText->SetOpacity(FadeInAlpha);
	if (LogoNumber) LogoNumber->SetOpacity(FadeInAlpha);
	if (LogoDivider) LogoDivider->SetOpacity(FadeInAlpha);
	if (FooterText) FooterText->SetOpacity(FadeInAlpha);
	if (HintText) HintText->SetOpacity(FadeInAlpha);
}

void ATitleCanvasActor::UpdateCreditsVisibility()
{
	// SetCanvasVisibility(true)가 모든 컴포넌트를 켜기 때문에 매 프레임 패널 표시 여부를 다시 맞춘다
	if (CreditsPanel) CreditsPanel->SetVisibility(bShowCredits);
	if (CreditsAccent) CreditsAccent->SetVisibility(bShowCredits);
	if (CreditsTitle) CreditsTitle->SetVisibility(bShowCredits);
	for (UUITextComponent* Line : CreditsLines)
	{
		if (Line) Line->SetVisibility(bShowCredits);
	}
}

void ATitleCanvasActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const bool bVisible = IsCanvasVisible();

	// 타이틀 화면에 새로 진입할 때마다 초기화 + 페이드 인
	if (bVisible && !bWasVisible)
	{
		SelectedIndex = 0;
		SelectionBarRelY = GetMenuItemRelY(0);
		bShowCredits = false;
		FadeInAlpha = 0.0f;
		LastMouseX = FInputSystem::GetMouseX();
		LastMouseY = FInputSystem::GetMouseY();
	}
	bWasVisible = bVisible;

	if (!bVisible)
	{
		return;
	}

	// 메뉴는 Time Dilation 영향을 받지 않도록 그대로 DeltaTime 사용
	FadeInAlpha = (FadeInAlpha + DeltaTime * 2.0f > 1.0f) ? 1.0f : FadeInAlpha + DeltaTime * 2.0f;

	UpdateLogoLayout();
	HandleInput();

	// HandleInput 에서 InGame 으로 넘어갔으면 더 이상 갱신하지 않음
	if (!IsCanvasVisible())
	{
		return;
	}

	UpdateSelectionVisual(DeltaTime);
	UpdateCreditsVisibility();
}
