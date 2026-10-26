#include "EnginePCH.h"
#include "GameFramework/PortalPawn.h"
#include "GameFramework/Actor/PortalGunActor.h"
#include "Components/SphereComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"

APortalPawn::APortalPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorEnableCollision(true);

	// 1. 루트 구 콜라이더
	SphereComponent = CreateDefaultSubobject<USphereComponent>("SphereComponent");
	SetRootComponent(SphereComponent);
	SphereComponent->SetSphereRadius(CollisionRadius);

	// 2. 1인칭 카메라 컴포넌트
	CameraComponent = CreateDefaultSubobject<UCameraComponent>("CameraComponent");
	CameraComponent->SetupAttachment(SphereComponent);
	CameraComponent->SetRelativeLocation({ 0.0f, 0.0f, EyeHeight });
	CameraComponent->SetRelativeRotation({ 0.0f, 0.0f, 0.0f });

	CameraComponent->SetExternalInputManaged(true);
}

void APortalPawn::BeginPlay()
{
	Super::BeginPlay();
	VerticalVelocity = 0.0f;
	bIsGrounded = false;

	// 게임 시작 시 포탈건 액터 스폰 및 카메라에 장착
	if (GetWorld() && !PortalGun)
	{
		PortalGun = GetWorld()->SpawnActor<APortalGunActor>();
		if (PortalGun)
		{
			PortalGun->AttachToPawn(this);
		}
	}
}

void APortalPawn::EndPlay()
{
	Super::EndPlay();
	if (PortalGun)
	{
		PortalGun->Destroy();
		PortalGun = nullptr;
	}
}

void APortalPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 중력 및 바닥 충돌
	if (bEnableGravity && GetRootComponent())
	{
		VerticalVelocity -= Gravity * DeltaTime;
		const FVector DeltaZ = FVector(0.0f, 0.0f, VerticalVelocity * DeltaTime);

		if (VerticalVelocity > 0.0f)
		{
			GetRootComponent()->MoveComponent(DeltaZ, GetActorRotation(), false);
			bIsGrounded = false;
		}
		else
		{
			FHitResult Hit;
			const bool bMoved = GetRootComponent()->MoveComponent(DeltaZ, GetActorRotation(), true, &Hit);

			if (!bMoved && Hit.ImpactNormal.Z > 0.5f)
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
}

void APortalPawn::AddMovementInput(const FVector& WorldDirection, float ScaleValue)
{
	if (GetRootComponent())
	{
		const FVector Delta = WorldDirection * (ScaleValue * MoveSpeed);
		FHitResult Hit;
		GetRootComponent()->MoveComponent(Delta, GetActorRotation(), true, &Hit);
	}
}

void APortalPawn::AddControllerYawInput(float Value)
{
	if (!GetRootComponent()) return;

	FRotator Rotation = GetActorRotation();
	Rotation.Yaw += Value * LookSensitivity;
	GetRootComponent()->SetRelativeRotation(Rotation);
}

void APortalPawn::AddControllerPitchInput(float Value)
{
	if (!CameraComponent) return;

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
		if (GetRootComponent())
		{
			GetRootComponent()->MoveComponent(FVector(0.0f, 0.0f, 0.02f), GetActorRotation(), false);
		}
	}
}

void APortalPawn::PostEditChangeProperty(const FProperty& Property)
{
	Super::PostEditChangeProperty(Property);

	if (SphereComponent)
	{
		SphereComponent->SetSphereRadius(CollisionRadius);
	}
	if (CameraComponent)
	{
		CameraComponent->SetRelativeLocation({ 0.0f, 0.0f, EyeHeight });
	}
}

// 포탈건으로 모든 무기 동작 위임
void APortalPawn::PrimaryAction()
{
	if (PortalGun)
	{
		PortalGun->PrimaryAction();
	}
}

void APortalPawn::TogglePortalType()
{
	if (PortalGun)
	{
		PortalGun->TogglePortalType();
	}
}

void APortalPawn::Interact()
{
	if (PortalGun)
	{
		PortalGun->Interact();
	}
}
