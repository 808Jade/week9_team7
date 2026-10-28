#include "EnginePCH.h"
#include "TurretActor.h"
#include "Engine/World.h"
#include "Asset/AssetManager.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

ATurretActor::ATurretActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorEnableCollision(true);

	// 1. 충돌체 (루트 컴포넌트: 벽과 바닥 충돌 감지)
	CollisionBox = CreateDefaultSubobject<UBoxComponent>("CollisionBox");
	CollisionBox->SetBoxExtent(FVector(0.45f, 0.35f, 0.7f));
	CollisionBox->SetCurrentCollisionResponse(ECollisionResponse::Block);
	SetRootComponent(CollisionBox);

	// 2. 터렛 3D 메시 (충돌체에 부착)
	TurretMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("TurretMeshComponent");
	TurretMeshComponent->SetupAttachment(CollisionBox);
	TurretMeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -0.75f));

	UStaticMesh* TurretMesh = UAssetManager::GetAssetByPath<UStaticMesh>("Models/Turret/turret_bundle.obj");
	if (TurretMesh)
	{
		TurretMeshComponent->SetStaticMesh(TurretMesh);
	}
}

void ATurretActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 1. 중력 적용 (공중에 떠 있거나 넉백 중일 때)
	if (!bIsGrounded || Velocity.Z > 0.0f)
	{
		Velocity.Z -= Gravity * DeltaTime;
	}

	// 2. 넉백 이동 & 충돌 스위프 (벽/바닥 충돌 감지)
	const FVector DeltaMove = Velocity * DeltaTime;
	if (DeltaMove.SizeSquared() > 1e-8f && GetRootComponent())
	{
		FHitResult Hit;
		GetRootComponent()->MoveComponent(DeltaMove, GetActorRotation(), true, &Hit);

		if (Hit.bBlockingHit)
		{
			// 바닥 충돌 (아래로 떨어지다 바닥에 닿음)
			if (Hit.ImpactNormal.Z > 0.5f && Velocity.Z <= 0.0f)
			{
				bIsGrounded = true;
				Velocity.Z = 0.0f;
			}
			// 천장 충돌
			else if (Hit.ImpactNormal.Z < -0.5f && Velocity.Z > 0.0f)
			{
				Velocity.Z = 0.0f;
			}

			// 벽면 충돌 (수평 이동 중 벽에 부딪힘)
			if (Hit.ImpactNormal.Z <= 0.5f)
			{
				// 벽면에 수평 속도 제거 (벽 뚫기 방지)
				Velocity.X = 0.0f;
				Velocity.Y = 0.0f;
			}
		}
		else
		{
			// 허공에 떠 있는 상태
			bIsGrounded = false;
		}
	}

	// 지상 마찰 감속
	if (bIsGrounded)
	{
		const float GroundFriction = 8.0f;
		Velocity.X *= FMath::Clamp(1.0f - GroundFriction * DeltaTime, 0.0f, 1.0f);
		Velocity.Y *= FMath::Clamp(1.0f - GroundFriction * DeltaTime, 0.0f, 1.0f);
	}

	// 스쿼시 크기 원래대로 복원
	if (TurretMeshComponent)
	{
		FVector CurrentScale = TurretMeshComponent->GetRelativeScale3D();
		const FVector TargetScale = FVector(1.0f, 1.0f, 1.0f);
		const float LerpSpeed = 6.0f; // 스쿼시 복원 속도
		const float Alpha = FMath::Clamp(LerpSpeed * DeltaTime, 0.0f, 1.0f);
		FVector NewScale = CurrentScale + (TargetScale - CurrentScale) * Alpha;
		TurretMeshComponent->SetRelativeScale3D(NewScale);

		// 넉백과 스쿼시가 모두 끝나면 피격 플래그 리셋 (다시 맞을 수 있도록)
		if (bIsHit && bIsGrounded && Velocity.SizeSquared() < 0.01f && (CurrentScale - TargetScale).SizeSquared() < 0.001f)
		{
			TurretMeshComponent->SetRelativeScale3D(TargetScale);
			Velocity = FVector::ZeroVector;
			bIsHit = false;
		}
	}
}

void ATurretActor::OnHit(const FVector& Direction, const FVector& ImpactPoint)
{
	if (bIsHit) return; // 이미 맞은 상태라면 중복 적용 방지
	bIsHit = true;
	bIsGrounded = false;

	// 터렛이 맞았을 때 시간 느리게 적용 (Hit Stop)
	if (GetWorld())
	{
		GetWorld()->SetTimeDilation(0.06f, 0.02f);
	}

	// 터렛 메시를 눌러서 찌그러뜨리는 크기를 적용 (Hit Squash)
	ApplyHitSquash(Direction);

	// 터렛의 넉백을 위한 속도 벡터 계산 (수평 밀림 4.0m/s + 살짝만 공중으로 0.8m/s 띄움)
	FVector HorizDir = FVector(Direction.X, Direction.Y, 0.0f);
	if (HorizDir.SizeSquared() > 1e-4f)
	{
		HorizDir = HorizDir.Normalized();
	}
	else
	{
		HorizDir = GetActorQuat().GetForwardVector();
	}

	Velocity = (HorizDir * 4.0f) + FVector(0.0f, 0.0f, 0.8f);
}

void ATurretActor::ApplyHitSquash(const FVector& HitDirection)
{
	if (!TurretMeshComponent) return;
	FVector SquashScale = GetDirectionalSquashScale(HitDirection);
	TurretMeshComponent->SetRelativeScale3D(SquashScale);
}

FVector ATurretActor::GetDirectionalSquashScale(const FVector& HitDirection) const
{
	FVector HorizHitDir = FVector(HitDirection.X, HitDirection.Y, 0.0f);
	if (HorizHitDir.SizeSquared() > 1e-6f)
	{
		HorizHitDir = HorizHitDir.Normalized();
	}
	else
	{
		HorizHitDir = GetActorQuat().GetForwardVector();
	}

	const FQuat Quat = GetActorQuat();
	const FVector Forward = Quat.GetForwardVector();
	const FVector Right = Quat.GetRightVector();

	const float ForwardDot = std::abs(FVector::Dot(HorizHitDir, Forward));
	const float RightDot = std::abs(FVector::Dot(HorizHitDir, Right));

	const float TotalDot = ForwardDot + RightDot;
	const float ForwardWeight = (TotalDot > 1e-4f) ? (ForwardDot / TotalDot) : 1.0f;
	const float RightWeight = (TotalDot > 1e-4f) ? (RightDot / TotalDot) : 0.0f;

	const float MinScale = 0.65f;
	const float MaxScale = 1.25f;
	const float UpScale = 1.15f;

	const float ScaleX = MaxScale - (MaxScale - MinScale) * ForwardWeight;
	const float ScaleY = MaxScale - (MaxScale - MinScale) * RightWeight;
	const float ScaleZ = UpScale;

	return FVector(ScaleX, ScaleY, ScaleZ);
}
