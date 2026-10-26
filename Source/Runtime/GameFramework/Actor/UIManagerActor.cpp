#include "EnginePCH.h"
#include "UIManagerActor.h"
#include "HUDCanvasActor.h"
#include "TitleCanvasActor.h"
#include "PauseCanvasActor.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "UObject/Casts.h"

AUIManagerActor* AUIManagerActor::Instance = nullptr;

AUIManagerActor::AUIManagerActor()
{
	PrimaryActorTick.bCanEverTick = false; // 이벤트 방식으로 작동하므로 매 프레임 Tick은 끌 수 있습니다.
	if (!Instance)
	{
		Instance = this;
	}
}

AUIManagerActor::~AUIManagerActor()
{
	if (Instance == this)
	{
		Instance = nullptr;
	}
}

void AUIManagerActor::BeginPlay()
{
	Super::BeginPlay();
	Instance = this;

	// 레벨에 배치된 캔버스들을 자동으로 탐색하여 연결
	AutoDiscoverCanvases();

	// 시작 화면 모드로 갱신
	SetUIMode(CurrentMode);
}

void AUIManagerActor::EndPlay()
{
	if (Instance == this)
	{
		Instance = nullptr;
	}
	Super::EndPlay();
}

void AUIManagerActor::AutoDiscoverCanvases()
{
	if (!GetWorld()) return;

	ULevel* Level = GetWorld()->GetPersistentLevel();
	if (!Level) return;

	// 레벨에 이미 배치된 HUD가 있는지 검색
	for (AActor* Actor : Level->GetActors())
	{
		if (!Actor) continue;

		if (!HUDCanvas)
		{
			if (AHUDCanvasActor* FoundHUD = Cast<AHUDCanvasActor>(Actor))
			{
				HUDCanvas = FoundHUD;
			}
		}

		if (!TitleCanvas)
		{
			if (ATitleCanvasActor* FoundTitle = Cast<ATitleCanvasActor>(Actor))
			{
				TitleCanvas = FoundTitle;
			}
		}

		if (!PauseCanvas)
		{
			if (APauseCanvasActor* FoundPause = Cast<APauseCanvasActor>(Actor))
			{
				PauseCanvas = FoundPause;
			}
		}
	}

	// 만약 레벨에 배치되지 않았다면 자동으로 생성(Spawn)
	if (!HUDCanvas)
	{
		HUDCanvas = GetWorld()->SpawnActor<AHUDCanvasActor>();
	}
}

void AUIManagerActor::SetUIMode(EUIMode NewMode)
{
	CurrentMode = NewMode;

	// 1. 인게임 HUD (InGame 모드일 때만 활성화)
	if (HUDCanvas)
	{
		HUDCanvas->SetCanvasVisibility(CurrentMode == EUIMode::InGame);
	}

	// 2. 타이틀 화면 (Title 모드일 때만 활성화)
	if (TitleCanvas)
	{
		TitleCanvas->SetCanvasVisibility(CurrentMode == EUIMode::Title);
	}

	// 3. 일시정지 메뉴 (Pause 모드일 때만 활성화)
	if (PauseCanvas)
	{
		PauseCanvas->SetCanvasVisibility(CurrentMode == EUIMode::Pause);
	}
}

void AUIManagerActor::SetHUDCanvas(AHUDCanvasActor* InCanvas)
{
	HUDCanvas = InCanvas;
	if (HUDCanvas)
	{
		HUDCanvas->SetCanvasVisibility(CurrentMode == EUIMode::InGame);
	}
}

void AUIManagerActor::SetTitleCanvas(ATitleCanvasActor* InCanvas)
{
	TitleCanvas = InCanvas;
	if (TitleCanvas)
	{
		TitleCanvas->SetCanvasVisibility(CurrentMode == EUIMode::Title);
	}
}

void AUIManagerActor::SetPauseCanvas(APauseCanvasActor* InCanvas)
{
	PauseCanvas = InCanvas;
	if (PauseCanvas)
	{
		PauseCanvas->SetCanvasVisibility(CurrentMode == EUIMode::Pause);
	}
}
