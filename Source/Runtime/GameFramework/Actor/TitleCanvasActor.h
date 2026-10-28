#pragma once
#include "GameFramework/Actor/CanvasActor.h"

class UUIImageComponent;
class UUITextComponent;
class UFont;

// 포탈 2 스타일 메인 메뉴
//  - 좌상단 대형 로고 ("PORTAL" | "3")
//  - 좌하단 세로 메뉴 리스트 + 회색 선택 바 (방향키/W/S/마우스 이동, Enter/Space/클릭 결정)
//  - 화면 좌측을 어둡게 누르는 그라데이션 + 전체 디밍
//  - 하단 팀 표기 / 제작진 패널
class ATitleCanvasActor : public ACanvasActor
{
	DECLARE_CLASS(ATitleCanvasActor, ACanvasActor)
	REFLECT_START(ClassName)
		REFLECT_END()

public:
	ATitleCanvasActor();
	virtual ~ATitleCanvasActor() = default;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// 컴포넌트 접근자 (기존 호환)
	UUIImageComponent* GetBackgroundImage() const { return BackgroundImage; }
	UUITextComponent* GetTitleText() const { return LogoText; }

private:
	enum class EMenuItem : int32
	{
		NewGame = 0,
		Credits,
		Quit,
		Count
	};

	static constexpr int32 MenuCount = static_cast<int32>(EMenuItem::Count);
	static constexpr int32 GradientStripCount = 8;
	static constexpr int32 CreditLineCount = 2;

	void ApplyLayout();
	void RemoveLegacyComponents();
	void UpdateLogoLayout();
	void UpdateSelectionVisual(float DeltaTime);
	void UpdateCreditsVisibility();
	void HandleInput();
	void ActivateItem(int32 Index);
	int32 FindHoveredItem() const;

	float GetMenuItemRelY(int32 Index) const;

	// 배경
	UUIImageComponent* BackgroundImage = nullptr;
	UUIImageComponent* DimOverlay = nullptr;
	UUIImageComponent* GradientStrips[GradientStripCount] = {};

	// 로고
	UUITextComponent* LogoText = nullptr;     // "PORTAL"
	UUIImageComponent* LogoDivider = nullptr;  // 포탈 블루 세로 막대
	UUITextComponent* LogoNumber = nullptr;   // "3"

	// 메뉴
	UUIImageComponent* SelectionBar = nullptr;
	UUITextComponent* MenuTexts[MenuCount] = {};
	UUITextComponent* FooterText = nullptr;
	UUITextComponent* HintText = nullptr;

	// 제작진 패널
	UUIImageComponent* CreditsPanel = nullptr;
	UUIImageComponent* CreditsAccent = nullptr;
	UUITextComponent* CreditsTitle = nullptr;
	UUITextComponent* CreditsLines[CreditLineCount] = {};

	UFont* MenuFont = nullptr;

	int32 SelectedIndex = 0;
	float SelectionBarRelY = -1.0f; // 선택 바 현재 위치 (보간용)
	bool bShowCredits = false;
	bool bWasVisible = false;
	float FadeInAlpha = 0.0f;
	int32 LastMouseX = -1;
	int32 LastMouseY = -1;
};
