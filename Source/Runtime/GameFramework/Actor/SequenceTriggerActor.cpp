#include "EnginePCH.h"
#include "SequenceTriggerActor.h"
#include "GameFramework/PortalPawn.h"
#include "GameFramework/PortalPlayerController.h"
#include "Engine/World.h"
#include <cmath>

ASequenceTriggerActor::ASequenceTriggerActor()
{
	PrimaryActorTick.bCanEverTick = true;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>("SequenceTriggerBox");
	TriggerBox->SetupAttachment(GetRootComponent());
	TriggerBox->SetBoxExtent(TriggerExtent);
	TriggerBox->SetCurrentCollisionResponse(ECollisionResponse::Overlap);

	SequencePlayer = MakeShared<FSequencePlayer>();
}

void ASequenceTriggerActor::BeginPlay()
{
	Super::BeginPlay();

	if (!TriggerBox)
	{
		TriggerBox = Cast<UBoxComponent>(FindComponentByName(FName("SequenceTriggerBox")));
	}

	if (TriggerBox)
	{
		TriggerBox->SetBoxExtent(TriggerExtent);
		TriggerBox->SetCurrentCollisionResponse(ECollisionResponse::Overlap);
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ASequenceTriggerActor::OnTriggerBeginOverlap);
	}

	bTriggered = false;

	if (!SequencePlayer)
	{
		SequencePlayer = MakeShared<FSequencePlayer>();
	}

	if (!SequencePath.empty())
	{
		SequencePlayer->GetSequence()->LoadFromFile(SequencePath);
	}
	SequencePlayer->SetLooping(bLoop);

	if (bAutoPlay)
	{
		Play();
	}
}

void ASequenceTriggerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (SequencePlayer && SequencePlayer->IsPlaying())
	{
		SequencePlayer->Tick(DeltaTime, GetWorld());
	}

	// 거리 기반 AABB 체크 (콜라이더 이벤트 누락 대비 보조 감지)
	if (!bTriggered && !bAutoPlay)
	{
		if (GetWorld() && GetWorld()->GetPlayerPawn())
		{
			APawn* Pawn = GetWorld()->GetPlayerPawn();
			FVector PawnLoc = Pawn->GetActorLocation();
			FVector Center = GetActorLocation();

			if (std::abs(PawnLoc.X - Center.X) <= TriggerExtent.X &&
				std::abs(PawnLoc.Y - Center.Y) <= TriggerExtent.Y &&
				std::abs(PawnLoc.Z - Center.Z) <= TriggerExtent.Z)
			{
				Play();
			}
		}
	}
}

void ASequenceTriggerActor::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bTriggered && bTriggerOnce) return;
	if (!OtherActor) return;

	if (Cast<APortalPawn>(OtherActor) || (GetWorld() && OtherActor == GetWorld()->GetPlayerPawn()))
	{
		Play();
	}
}

void ASequenceTriggerActor::Play()
{
	if (bTriggered && bTriggerOnce) return;
	bTriggered = true;

	if (bTriggerOnce && TriggerBox)
	{
		TriggerBox->SetCurrentCollisionResponse(ECollisionResponse::NoCollision);
	}

	if (!SequencePlayer)
	{
		SequencePlayer = MakeShared<FSequencePlayer>();
	}

	if (!SequencePath.empty())
	{
		SequencePlayer->GetSequence()->LoadFromFile(SequencePath);
	}

	SequencePlayer->SetLooping(bLoop);
	SequencePlayer->Play();

	HTR_LOG(Info, "[ASequenceTriggerActor] Started playing sequence: {}", SequencePath);

	if (bDisablePlayerInputOnPlay)
	{
		if (GetWorld() && GetWorld()->GetPlayerController())
		{
			if (APortalPlayerController* PC = Cast<APortalPlayerController>(GetWorld()->GetPlayerController()))
			{
				PC->SetMouseCapture(false);
			}
		}
	}
}

void ASequenceTriggerActor::Pause()
{
	if (SequencePlayer)
	{
		SequencePlayer->Pause();
	}
}

void ASequenceTriggerActor::Stop()
{
	if (SequencePlayer)
	{
		SequencePlayer->Stop(GetWorld());
	}
}
