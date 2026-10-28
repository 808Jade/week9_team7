#include "EnginePCH.h"
#include "TriggerActor.h"
#include "Components/LuaScriptComponent.h"
#include "GameFramework/PortalPawn.h"
#include "Engine/World.h"
#include <cmath>

ATriggerActor::ATriggerActor()
{
	PrimaryActorTick.bCanEverTick = true;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>("TriggerBox");
	TriggerBox->SetupAttachment(GetRootComponent());
	TriggerBox->SetBoxExtent(TriggerExtent);
	TriggerBox->SetCurrentCollisionResponse(ECollisionResponse::Overlap);
}

void ATriggerActor::BeginPlay()
{
	Super::BeginPlay();

	if (!TriggerBox)
	{
		TriggerBox = Cast<UBoxComponent>(FindComponentByName(FName("TriggerBox")));
	}

	if (TriggerBox)
	{
		TriggerBox->SetBoxExtent(TriggerExtent);
		TriggerBox->SetCurrentCollisionResponse(ECollisionResponse::Overlap);
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ATriggerActor::OnTriggerBeginOverlap);
	}

	bTriggered = false;
}

void ATriggerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 거리 기반 AABB 체크 (콜라이더 누락 방지 보조 감지)
	if (!bTriggered)
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
				ExecuteTrigger(Pawn);
			}
		}
	}
}

void ATriggerActor::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bTriggered && bTriggerOnce) return;
	if (!OtherActor) return;

	if (Cast<APortalPawn>(OtherActor) || (GetWorld() && OtherActor == GetWorld()->GetPlayerPawn()))
	{
		ExecuteTrigger(OtherActor);
	}
}

void ATriggerActor::ExecuteTrigger(AActor* OtherActor)
{
	if (bTriggered && bTriggerOnce) return;
	bTriggered = true;

	// 1회성 트리거인 경우 발동 즉시 콜라이더를 NoCollision으로 전환하여 물리 충돌 및 중복 연산 방지
	if (bTriggerOnce && TriggerBox)
	{
		TriggerBox->SetCurrentCollisionResponse(ECollisionResponse::NoCollision);
	}

	HTR_LOG(Info, "[ATriggerActor] {} triggered by {}", GetName(), OtherActor ? OtherActor->GetName() : "Unknown");

	// 부착된 LuaScriptComponent가 있다면 OnOverlap 호출!
	if (ULuaScriptComponent* LuaComp = FindComponentByClass<ULuaScriptComponent>())
	{
		LuaComp->OnOverlap(OtherActor);
	}
}
