#include "EnginePCH.h"
#include "GameFramework/PortalPawn.h"
#include "Components/SphereComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"

APortalPawn::APortalPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorEnableCollision(true);

	// 1. 루트 컴포넌트: 구 콜라이더 (팀원이 구현한 SphereComponent)
	SphereComponent = CreateDefaultSubobject<USphereComponent>("SphereComponent");
	SetRootComponent(SphereComponent);
	SphereComponent->SetSphereRadius(CollisionRadius);

	// 2. 1인칭 카메라 컴포넌트
	CameraComponent = CreateDefaultSubobject<UCameraComponent>("CameraComponent");
	CameraComponent->SetupAttachment(SphereComponent);
	CameraComponent->SetRelativeLocation({ 0.0f, 0.0f, EyeHeight });
	CameraComponent->SetRelativeRotation({ 0.0f, 0.0f, 0.0f });

	//  카메라 컴포넌트 자체 틱의 마우스 입력 경합 방지
	CameraComponent->SetExternalInputManaged(true);
}

void APortalPawn::BeginPlay()
{
	Super::BeginPlay();
	VerticalVelocity = 0.0f;
	bIsGrounded = false;
}

void APortalPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 중력 및 바닥 충돌 처리
	if (bEnableGravity && GetRootComponent())
	{
		VerticalVelocity -= Gravity * DeltaTime;

		const FVector DeltaZ = FVector(0.0f, 0.0f, VerticalVelocity * DeltaTime);
		FHitResult Hit;

		// bSweep = true로 아래로 스위프하여 바닥 박스 콜라이더와 충돌 검사
		const bool bMoved = GetRootComponent()->MoveComponent(DeltaZ, GetActorRotation(), true, &Hit);

		// 아래로 떨어지다가 바닥에 막혔을 때 (HitNormal의 Z가 위쪽을 향함)
		if (!bMoved && VerticalVelocity < 0.0f && Hit.ImpactNormal.Z > 0.5f)
		{
			bIsGrounded = true;
			VerticalVelocity = 0.0f;
		}
		else
		{
			bIsGrounded = false;
		}
	}
}

void APortalPawn::AddMovementInput(const FVector& WorldDirection, float ScaleValue)
{
	if (GetRootComponent())
	{
		// 수평 이동: bSweep = true로 벽(박스)에 부딪히면 멈춤
		const FVector Delta = WorldDirection * (ScaleValue * MoveSpeed);
		FHitResult Hit;
		GetRootComponent()->MoveComponent(Delta, GetActorRotation(), true, &Hit);
	}
}

void APortalPawn::AddControllerYawInput(float Value)
{
	if (!GetRootComponent()) return;

	// 좌우 회전(Yaw): 플레이어 몸체(구 콜라이더) 전체를 회전
	FRotator Rotation = GetActorRotation();
	Rotation.Yaw += Value * LookSensitivity;
	GetRootComponent()->SetRelativeRotation(Rotation);
}

void APortalPawn::AddControllerPitchInput(float Value)
{
	if (!CameraComponent) return;

	// 상하 회전(Pitch): 몸체는 가만히 두고 1인칭 카메라(목)만 위아래로 회전 (-89도 ~ +89도)
	FRotator CamRot = CameraComponent->GetRelativeRotation();
	CamRot.Pitch = FMath::Clamp(CamRot.Pitch + Value * LookSensitivity, -89.0f, 89.0f);
	CameraComponent->SetRelativeRotation(CamRot);
}

void APortalPawn::Jump()
{
	if (bIsGrounded)
	{
		VerticalVelocity = JumpVelocity;
		bIsGrounded = false;
	}
}

void APortalPawn::PostEditChangeProperty(const FProperty& Property)
{
	Super::PostEditChangeProperty(Property);

	// 에디터 Details 창에서 슬라이더나 수치를 변경하면 즉시 컴포넌트에 반영
	if (SphereComponent)
	{
		SphereComponent->SetSphereRadius(CollisionRadius);
	}
	if (CameraComponent)
	{
		CameraComponent->SetRelativeLocation({ 0.0f, 0.0f, EyeHeight });
	}
}