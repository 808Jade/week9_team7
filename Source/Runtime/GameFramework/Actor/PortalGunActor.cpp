#include "EnginePCH.h"
#include "GameFramework/Actor/PortalGunActor.h"
#include "GameFramework/PortalPawn.h"
#include "GameFramework/Actor/PortalCubeActor.h"
#include "GameFramework/Actor/PortalActor.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Asset/AssetManager.h"
#include "Engine/World.h"
#include "Collision/Ray.h"
#include "Collision/HitResult.h"

APortalGunActor::APortalGunActor()
{
	PrimaryActorTick.bCanEverTick = true;	
	SetActorEnableCollision(false);

	// 1. 루트 메시 컴포넌트
	GunMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("GunMeshComponent");
	SetRootComponent(GunMeshComponent);

	// 2. 포탈건 3D 메시 연결
	UStaticMesh* GunMesh = UAssetManager::GetAssetByPath<UStaticMesh>("Models/PortalGun/portal_gun.obj");
	if (GunMesh)
	{
		GunMeshComponent->SetStaticMesh(GunMesh);
	}
}

void APortalGunActor::BeginPlay()
{
	Super::BeginPlay();
	HeldCube = nullptr;

	// 포탈 2개를 미리 만들어 두고(숨김), 쏠 때마다 이동만 시킨다
	if (!BluePortal && GetWorld())
	{
		APortalActor::SpawnPair(GetWorld(), BluePortal, OrangePortal);
	}
}

void APortalGunActor::EndPlay()
{
	Super::EndPlay();
	if (HeldCube)
	{
		HeldCube = nullptr;
	}
}

void APortalGunActor::AttachToPawn(APortalPawn* InOwnerPawn)
{
	OwnerPawn = InOwnerPawn;
	if (InOwnerPawn && GunMeshComponent)
	{
		UCameraComponent* Cam = InOwnerPawn->GetCameraComponent();
		if (Cam)
		{
			GunMeshComponent->SetupAttachment(Cam, EAttachmentRule::KeepRelative);
			GunMeshComponent->SetRelativeLocation(GunOffsetLocation);
			GunMeshComponent->SetRelativeRotation(GunOffsetRotation);
			GunMeshComponent->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
		}
	}
}

void APortalGunActor::FirePortal(const EPortalType PortalToFire, const FHitResult& HitResult)
{
	APortalActor* Portal = GetPortal(PortalToFire);
	if (!Portal || !HitResult.bBlockingHit)
	{
		return;
	}

	Portal->PlaceAt(HitResult.ImpactPoint, HitResult.ImpactNormal);
}

void APortalGunActor::Hold(APortalCubeActor* CubeToGrab)
{
	if(!CubeToGrab)
	{
		return;
	}

	HTR_LOG(Warning, "Hold");

	HeldCube = CubeToGrab;
	HeldCube->OnGrabbed();
}

void APortalGunActor::Unhold()
{
	if(HeldCube)
	{
		HTR_LOG(Warning, "Unhold");

		HeldCube->OnDropped(FVector::ZeroVector);
		HeldCube = nullptr;
	}
}

void APortalGunActor::ResetPortals()
{
	if (BluePortal)
	{
		BluePortal->ResetPortal();
	}
	if (OrangePortal)
	{
		OrangePortal->ResetPortal();
	}
}

void APortalGunActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsSwinging && GunMeshComponent)
	{
		SwingTimer += DeltaTime;
		const float StrikeTime = SwingDuration * 0.35f;
		const FVector StrikeLocation = GunOffsetLocation + FVector(0.25f, -0.12f, -0.05f);
		const FRotator StrikeRotation = GunOffsetRotation + FRotator(-15.0f, -25.0f, 35.0f);
		if (SwingTimer < StrikeTime)
		{
			// 진행률
			const float Alpha = SwingTimer / StrikeTime;
			const float EasedAlpha = sinf(Alpha * 1.5707963f); // sin(0 ~ PI/2)
			GunMeshComponent->SetRelativeLocation(GunOffsetLocation + (StrikeLocation - GunOffsetLocation) * EasedAlpha);
			GunMeshComponent->SetRelativeRotation(FRotator(
				GunOffsetRotation.Pitch + (StrikeRotation.Pitch - GunOffsetRotation.Pitch) * EasedAlpha,
				GunOffsetRotation.Yaw + (StrikeRotation.Yaw - GunOffsetRotation.Yaw) * EasedAlpha,
				GunOffsetRotation.Roll + (StrikeRotation.Roll - GunOffsetRotation.Roll) * EasedAlpha
			));
		}
		else if (SwingTimer < SwingDuration)
		{
			const float Alpha = (SwingTimer - StrikeTime) / (SwingDuration - StrikeTime);
			const float EasedAlpha = Alpha * Alpha * (3.0f - 2.0f * Alpha);
			GunMeshComponent->SetRelativeLocation(StrikeLocation + (GunOffsetLocation - StrikeLocation) * EasedAlpha);
			GunMeshComponent->SetRelativeRotation(FRotator(
				StrikeRotation.Pitch + (GunOffsetRotation.Pitch - StrikeRotation.Pitch) * EasedAlpha,
				StrikeRotation.Yaw + (GunOffsetRotation.Yaw - StrikeRotation.Yaw) * EasedAlpha,
				StrikeRotation.Roll + (GunOffsetRotation.Roll - StrikeRotation.Roll) * EasedAlpha
			));
		}
		else
		{
			bIsSwinging = false;
			SwingTimer = 0.0f;
			GunMeshComponent->SetRelativeLocation(GunOffsetLocation);
			GunMeshComponent->SetRelativeRotation(GunOffsetRotation);
		}
	}

	if(IsHoldingCube() && OwnerPawn)
	{
		UCameraComponent* Cam = OwnerPawn->GetCameraComponent();
		if(!Cam)
		{
			return;
		}

		FVector CamLoc = Cam->GetWorldLocation();
		FVector CamForward = Cam->GetWorldRotation().Quaternion().GetForwardVector().Normalized();
		FVector TargetLocation = CamLoc + (CamForward * HoldDistance);
		FRotator TargetRotation = Cam->GetWorldRotation();

		float DistanceToTarget = FVector::Distance(HeldCube->GetActorLocation(), TargetLocation);
		if(DistanceToTarget > MaxBreakDistance)
		{
			Unhold();
			return;
		}

		HeldCube->UpdateHoldPhysics(TargetLocation, TargetRotation, DeltaTime);
	}
}

void APortalGunActor::Swing()
{
	if (bIsSwinging || IsHoldingCube())
	{
		return;
	}
	bIsSwinging = true;
	SwingTimer = 0.0f;
}