#include "EnginePCH.h"
#include "UIManagerActor.h"
#include "HUDCanvasActor.h"
#include "TitleCanvasActor.h"
#include "PauseCanvasActor.h"
#include "EndingCanvasActor.h"
#include "ElevatorTriggerActor.h"
#include "GameFramework/PortalPlayerController.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "UObject/Casts.h"

#include "Lua/LuaManager.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

AUIManagerActor* AUIManagerActor::Instance = nullptr;
float AUIManagerActor::SavedClearTime = 0.0f;
FString AUIManagerActor::ReturnScenePath = "";
EUIMode AUIManagerActor::NextSceneInitialUIMode = EUIMode::Title;

AUIManagerActor* AUIManagerActor::GetOrSpawn(UWorld* InWorld)
{
	if (Instance) return Instance;
	UWorld* TargetWorld = InWorld ? InWorld : FLuaManager::Get().GetActiveWorld();
	if (TargetWorld)
	{
		Instance = TargetWorld->SpawnActor<AUIManagerActor>();
		if (Instance)
		{
			if (TargetWorld->HasBegunPlay())
			{
				Instance->BeginPlay();
			}
			else
			{
				Instance->AutoDiscoverCanvases();
			}
		}
	}
	return Instance;
}

AUIManagerActor::AUIManagerActor()
{
	PrimaryActorTick.bCanEverTick = false; // 이벤트 방식으로 작동하므로 매 프레임 Tick은 끕니다.
	if (!Instance)
	{
		Instance = this;

	if (NextSceneInitialUIMode != EUIMode::Title)
	{
		CurrentMode = NextSceneInitialUIMode;
		NextSceneInitialUIMode = EUIMode::Title;
	}
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

	if (NextSceneInitialUIMode != EUIMode::Title)
	{
		CurrentMode = NextSceneInitialUIMode;
		NextSceneInitialUIMode = EUIMode::Title;
	}

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

	AActor* ElevatorActor = nullptr;
	AElevatorTriggerActor* ElevatorTrigger = nullptr;

	// 레벨에 이미 배치된 UI 캔버스 및 액터 검색
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

		if (!EndingCanvas)
		{
			if (AEndingCanvasActor* FoundEnding = Cast<AEndingCanvasActor>(Actor))
			{
				EndingCanvas = FoundEnding;
			}
		}

		if (!ElevatorTrigger)
		{
			if (AElevatorTriggerActor* FoundTrigger = Cast<AElevatorTriggerActor>(Actor))
			{
				ElevatorTrigger = FoundTrigger;
			}
		}

		if (!ElevatorActor)
		{
			const FString Name = Actor->GetName();
			if (Name == "ExitElevator_Final" || Name.find("ExitElevator") != std::string::npos || Name.find("Elevator") != std::string::npos)
			{
				ElevatorActor = Actor;
			}
		}
	}

	// 만약 레벨에 배치되지 않았다면 자동으로 생성(Spawn)
	if (!HUDCanvas)
	{
		HUDCanvas = GetWorld()->SpawnActor<AHUDCanvasActor>(); if (HUDCanvas && GetWorld()->HasBegunPlay()) HUDCanvas->BeginPlay();
	}
	if (!TitleCanvas)
	{
		TitleCanvas = GetWorld()->SpawnActor<ATitleCanvasActor>(); if (TitleCanvas && GetWorld()->HasBegunPlay()) TitleCanvas->BeginPlay();
	}
	if (!PauseCanvas)
	{
		PauseCanvas = GetWorld()->SpawnActor<APauseCanvasActor>(); if (PauseCanvas && GetWorld()->HasBegunPlay()) PauseCanvas->BeginPlay();
	}
	if (!EndingCanvas)
	{
		EndingCanvas = GetWorld()->SpawnActor<AEndingCanvasActor>(); if (EndingCanvas && GetWorld()->HasBegunPlay()) EndingCanvas->BeginPlay();
	}

	// 엘리베이터 트리거가 없고 엘리베이터 액터가 있다면 자동 생성
	if (!ElevatorTrigger && ElevatorActor)
	{
		ElevatorTrigger = GetWorld()->SpawnActor<AElevatorTriggerActor>();
		if (ElevatorTrigger)
		{
			ElevatorTrigger->SetActorLocation(ElevatorActor->GetActorLocation());
		}
	}
}

void AUIManagerActor::SetUIMode(EUIMode NewMode)
{
	const EUIMode PrevMode = CurrentMode;
	CurrentMode = NewMode;
	if (!EndingCanvas || !HUDCanvas)
	{
		AutoDiscoverCanvases();
	}

	// 0. 게임 타이머 및 크로스헤어 연동 (시작, 정지, 리셋, 클리어 타임 기록)
	if (HUDCanvas)
	{
		if (CurrentMode == EUIMode::InGame)
		{
			if (PrevMode == EUIMode::Title || PrevMode == EUIMode::Ending)
			{
				HUDCanvas->ResetTimer();
			}
			HUDCanvas->StartTimer();
		}
		else if (CurrentMode == EUIMode::Pause)
		{
			HUDCanvas->StopTimer();
		}
		else if (CurrentMode == EUIMode::Ending)
		{
			HUDCanvas->StopTimer();
			if (EndingCanvas)
			{
				float Elapsed = HUDCanvas->GetElapsedTime();
				if (Elapsed <= 0.01f && SavedClearTime > 0.0f)
				{
					Elapsed = SavedClearTime;
				}
				EndingCanvas->SetClearTime(Elapsed);
			}
		}
		else if (CurrentMode == EUIMode::Title)
		{
			HUDCanvas->StopTimer();
			HUDCanvas->ResetTimer();
			SetCrosshairState("unfilled");
			if (EndingCanvas)
			{
				EndingCanvas->SetEndingType(EEndingType::Success);
			}
		}
	}

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

	// 4. 엔딩 화면 (Ending 모드일 때만 활성화)
	if (EndingCanvas)
	{
		EndingCanvas->SetCanvasVisibility(CurrentMode == EUIMode::Ending);
	}

	// 5. 마우스 커서 잠금/해제 상태 동기화
	if (GetWorld())
	{
		if (APlayerController* PC = GetWorld()->GetPlayerController())
		{
			if (APortalPlayerController* PPC = Cast<APortalPlayerController>(PC))
			{
				PPC->SetMouseCapture(CurrentMode == EUIMode::InGame);
			}
			if (CurrentMode == EUIMode::Ending || CurrentMode == EUIMode::Title)
			{
				if (APlayerCameraManager* PCM = PC->GetPlayerCameraManager())
				{
					PCM->SetManualFade(0.0f);
				}
			}
		}
		if (CurrentMode == EUIMode::Ending || CurrentMode == EUIMode::Title)
		{
			if (GEngine)
			{
				if (FWorldContext* Context = GEngine->GetWorldContextFromWorld(GetWorld()))
				{
					if (Context->GameViewport)
					{
						Context->GameViewport->SetLetterBox(false, 0.0f);
					}
				}
			}
		}
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

void AUIManagerActor::SetEndingCanvas(AEndingCanvasActor* InCanvas)
{
	EndingCanvas = InCanvas;
	if (EndingCanvas)
	{
		EndingCanvas->SetCanvasVisibility(CurrentMode == EUIMode::Ending);
	}
}

float AUIManagerActor::GetElapsedTime() const
{
	return HUDCanvas ? HUDCanvas->GetElapsedTime() : 0.0f;
}

FString AUIManagerActor::GetFormattedTime() const
{
	return HUDCanvas ? HUDCanvas->GetFormattedTime() : FString("00:00.00");
}

void AUIManagerActor::ResetTimer()
{
	if (HUDCanvas)
	{
		HUDCanvas->ResetTimer();
	}
}

void AUIManagerActor::StartTimer()
{
	if (HUDCanvas)
	{
		HUDCanvas->StartTimer();
	}
}

void AUIManagerActor::StopTimer()
{
	if (HUDCanvas)
	{
		HUDCanvas->StopTimer();
	}
}

void AUIManagerActor::SetCrosshairState(const FString& InState)
{
	if (HUDCanvas)
	{
		HUDCanvas->SetCrosshairStateByName(InState);
	}
}

FString AUIManagerActor::GetCrosshairState() const
{
	return HUDCanvas ? HUDCanvas->GetCrosshairStateName() : FString("unfilled");
}

void AUIManagerActor::SetCrosshairSize(float InWidth, float InHeight)
{
	if (HUDCanvas)
	{
		HUDCanvas->SetCrosshairSize(InWidth, InHeight);
	}
}

void AUIManagerActor::SetCrosshairVisibility(bool bVisible)
{
	if (HUDCanvas)
	{
		HUDCanvas->SetCrosshairVisibility(bVisible);
	}
}
