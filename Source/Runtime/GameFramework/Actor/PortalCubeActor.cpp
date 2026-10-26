#include "EnginePCH.h"
#include "PortalCubeActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Asset/AssetManager.h"

APortalCubeActor::APortalCubeActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorEnableCollision(true);

	// 1. 루트 박스 콜라이더 (1m x 1m x 1m 큐브, Extent = 0.5m)
	BoxComponent = CreateDefaultSubobject<UBoxComponent>("BoxComponent");
	SetRootComponent(BoxComponent);
	BoxComponent->SetBoxExtent(FVector(0.5f, 0.5f, 0.5f));
	BoxComponent->SetCurrentCollisionResponse(ECollisionResponse::Block);

	// 2. 포탈 큐브 3D 메시
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("StaticMeshComponent");
	StaticMeshComponent->SetupAttachment(BoxComponent);

	UStaticMesh* CubeMesh = UAssetManager::GetAssetByPath<UStaticMesh>("Models/PortalCube/portal_cube.obj");
	if (CubeMesh)
	{
		StaticMeshComponent->SetStaticMesh(CubeMesh);
	}
}

void APortalCubeActor::BeginPlay()
{
	Super::BeginPlay();
	bIsHeld = false;
	Velocity = FVector::ZeroVector;
	bIsGrounded = false;
}

void APortalCubeActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 플레이어가 들고 있을 때는 자체 물리 이동을 멈추고 플레이어 손을 따름
	if (bIsHeld) return;

	// 중력 및 이동 처리
	if (bEnableGravity && GetRootComponent())
	{
		Velocity.Z -= Gravity * DeltaTime;

		// 공기 저항 및 수평 감속
		Velocity.X *= FMath::Clamp(1.0f - 2.0f * DeltaTime, 0.0f, 1.0f);
		Velocity.Y *= FMath::Clamp(1.0f - 2.0f * DeltaTime, 0.0f, 1.0f);

		const FVector Delta = Velocity * DeltaTime;
		FHitResult Hit;

		// bSweep = true로 바닥 및 벽과 충돌 검사
		GetRootComponent()->MoveComponent(Delta, GetActorRotation(), true, &Hit);

		if (Hit.bBlockingHit)
		{
			// 바닥에 닿았을 때
			if (Hit.ImpactNormal.Z > 0.5f)
			{
				bIsGrounded = true;
				Velocity = FVector::ZeroVector;
			}
			// 벽에 부딪혔을 때 수평 속도 정지
			else
			{
				Velocity.X = 0.0f;
				Velocity.Y = 0.0f;
			}
		}
		else
		{
			bIsGrounded = false;
		}
	}
}

void APortalCubeActor::OnPickedUp(APawn* InHolder)
{
	bIsHeld = true;
	HoldingPawn = InHolder;
	Velocity = FVector::ZeroVector;
	bIsGrounded = false;

	// 플레이어가 들고 있는 동안 플레이어 시선이나 이동을 막지 않도록 콜리전 비활성화
	SetActorEnableCollision(false);
}

void APortalCubeActor::OnDropped(const FVector& InThrowVelocity)
{
	bIsHeld = false;
	HoldingPawn = nullptr;
	Velocity = InThrowVelocity;
	bIsGrounded = false;

	// 손에서 놓으면 다시 물리 충돌 활성화
	SetActorEnableCollision(true);
}
