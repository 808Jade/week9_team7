#include "EnginePCH.h"
#include "ElevatorTriggerActor.h"
#include "GameFramework/Actor/UIManagerActor.h"
#include "GameFramework/Actor/EndingCanvasActor.h"
#include "GameFramework/PortalPawn.h"
#include "Engine/World.h"
#include "Serialization/JsonArchive.h"
#include <cmath>
#include <filesystem>

AElevatorTriggerActor::AElevatorTriggerActor()
{
	PrimaryActorTick.bCanEverTick = true;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>("ElevatorTriggerBox");
	TriggerBox->SetupAttachment(GetRootComponent());
	TriggerBox->SetBoxExtent(TriggerExtent);
	TriggerBox->SetCurrentCollisionResponse(ECollisionResponse::Overlap);
}

void AElevatorTriggerActor::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerBox)
	{
		TriggerBox->SetBoxExtent(TriggerExtent);
		TriggerBox->SetCurrentCollisionResponse(ECollisionResponse::Overlap);
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AElevatorTriggerActor::OnTriggerBeginOverlap);
	}
	bTriggered = false;
}

void AElevatorTriggerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 이미 트리거되었거나 인게임 모드가 아닐 경우 처리
	if (bTriggered)
	{
		// 플레이어가 타이틀을 거쳐 다시 챔버 시작 지점으로 리스폰되었다면 트리거 플래그 초기화
		if (AUIManagerActor::Get() && AUIManagerActor::Get()->GetCurrentUIMode() == EUIMode::InGame)
		{
			if (GetWorld() && GetWorld()->GetPlayerPawn())
			{
				APawn* Pawn = GetWorld()->GetPlayerPawn();
				FVector Diff = Pawn->GetActorLocation() - GetActorLocation();
				if (std::abs(Diff.X) > TriggerExtent.X + 2.0f)
				{
					bTriggered = false;
				}
			}
		}
		return;
	}

	if (!AUIManagerActor::Get() || AUIManagerActor::Get()->GetCurrentUIMode() != EUIMode::InGame)
	{
		return;
	}

	// 거리 및 AABB 기반 체크 (물리 오버랩 콜백 누락 방지 및 100% 신뢰성 보장)
	if (GetWorld() && GetWorld()->GetPlayerPawn())
	{
		APawn* Pawn = GetWorld()->GetPlayerPawn();
		FVector PawnLoc = Pawn->GetActorLocation();
		FVector Center = GetActorLocation();

		if (std::abs(PawnLoc.X - Center.X) <= TriggerExtent.X &&
			std::abs(PawnLoc.Y - Center.Y) <= TriggerExtent.Y &&
			std::abs(PawnLoc.Z - Center.Z) <= TriggerExtent.Z)
		{
			TriggerEnding();
		}
	}
}

void AElevatorTriggerActor::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bTriggered) return;
	if (!OtherActor) return;

	if (Cast<APortalPawn>(OtherActor))
	{
		TriggerEnding();
	}
}

void AElevatorTriggerActor::TriggerEnding()
{
	if (bTriggered) return;
	if (AUIManagerActor::Get() && AUIManagerActor::Get()->GetCurrentUIMode() == EUIMode::InGame)
	{
		bTriggered = true;
		AUIManagerActor::Get()->StopTimer();
		AUIManagerActor::SetSavedClearTime(AUIManagerActor::Get()->GetElapsedTime());

		FString CurrentScene = FJsonArchive::GetLastLoadedScenePath();
		if (CurrentScene.empty())
		{
			CurrentScene = "Scenes/PortalFlingChamber.scene";
		}
		AUIManagerActor::SetReturnScenePath(CurrentScene);

		const FString EndingScenePath = "Scenes/EndingScene.scene";
		if (GetWorld() && std::filesystem::exists(EndingScenePath))
		{
			HTR_LOG(Info, "[AElevatorTriggerActor] Player entered elevator! Transitioning to {}", EndingScenePath);
			AUIManagerActor::SetNextSceneInitialUIMode(EUIMode::None);
			GetWorld()->RequestLoadScene(EndingScenePath);
			return;
		}

		HTR_LOG(Info, "[AElevatorTriggerActor] Player entered elevator! Triggering Ending UI.");
		if (AEndingCanvasActor* EndingCanvas = AUIManagerActor::Get()->GetEndingCanvas())
		{
			EndingCanvas->SetEndingType(EEndingType::Success);
		}
		AUIManagerActor::Get()->SetUIMode(EUIMode::Ending);
	}
}
