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

void APortalGunActor::FirePortal(const EPortalType PortalToFire, const FHitResult& HitResult, const FVector& ViewerForward)
{
	APortalActor* Portal = GetPortal(PortalToFire);
	if (!Portal || !HitResult.bBlockingHit)
	{
		return;
	}

	FVector Forward = ViewerForward;
	if (Forward.SizeSquared() < 1e-4f && OwnerPawn && OwnerPawn->GetCameraComponent())
	{
		Forward = OwnerPawn->GetCameraComponent()->GetWorldRotation().Quaternion().GetForwardVector().Normalized();
	}

	Portal->PlaceAt(HitResult, Forward);
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
