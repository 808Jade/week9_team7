#include "EnginePCH.h"
#include "GameFramework/Actor/PortalGunActor.h"
#include "GameFramework/PortalPawn.h"
#include "GameFramework/Actor/PortalCubeActor.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Asset/AssetManager.h"
#include "Engine/World.h"
#include "Collision/Ray.h"

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
	CurrentPortalType = EPortalType::Blue;
}

void APortalGunActor::EndPlay()
{
	Super::EndPlay();
	if (HeldCube)
	{
		HeldCube->OnDropped(FVector::ZeroVector);
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

void APortalGunActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 들고 있는 큐브 위치를 카메라 전방으로 갱신
	if (HeldCube && OwnerPawn)
	{
		UCameraComponent* Cam = OwnerPawn->GetCameraComponent();
		if (Cam)
		{
			const FVector CamLoc = Cam->GetWorldLocation();
			const FVector CamForward = Cam->GetWorldRotation().Quaternion().GetForwardVector().Normalized();
			const FVector TargetLocation = CamLoc + CamForward * HoldDistance;

			HeldCube->SetActorLocation(TargetLocation, false);
			HeldCube->SetActorRotation(OwnerPawn->GetActorRotation(), false);
		}
	}
}

void APortalGunActor::PrimaryAction()
{
	if (HeldCube)
	{
		ThrowCube();
	}
	else
	{
		FirePortal();
	}
}

void APortalGunActor::TogglePortalType()
{
	CurrentPortalType = (CurrentPortalType == EPortalType::Blue) ? EPortalType::Orange : EPortalType::Blue;
	HTR_LOG(Info, "[PortalGun] Switched to: {}", CurrentPortalType == EPortalType::Blue ? "BLUE (Primary)" : "ORANGE (Secondary)");
}

void APortalGunActor::ThrowCube()
{
	if (!HeldCube) return;

	UCameraComponent* Cam = OwnerPawn ? OwnerPawn->GetCameraComponent() : nullptr;
	const FVector CamForward = Cam ? 
		Cam->GetWorldRotation().Quaternion().GetForwardVector().Normalized() : 
		GetActorRotation().Quaternion().GetForwardVector().Normalized();

	const FVector ThrowVelocity = CamForward * ThrowStrength + FVector(0.0f, 0.0f, 2.0f);
	HeldCube->OnDropped(ThrowVelocity);
	HeldCube = nullptr;
}

void APortalGunActor::FirePortal()
{
	HTR_LOG(Info, "[PortalGun] Firing {} Portal!", CurrentPortalType == EPortalType::Blue ? "BLUE" : "ORANGE");
	// TODO: 여기에 화면 중앙 LineTrace 및 포탈 벽면 배치 로직 연결
}

void APortalGunActor::Interact()
{
	// 1. 이미 큐브를 들고 있다면 -> 발밑에 살포시 내려놓기
	if (HeldCube)
	{
		HeldCube->OnDropped(FVector::ZeroVector);
		HeldCube = nullptr;
		return;
	}

	// 2. 빈손이면 -> 전방 큐브 탐색
	if (!OwnerPawn || !OwnerPawn->GetCameraComponent() || !GetWorld()) return;

	UCameraComponent* Cam = OwnerPawn->GetCameraComponent();
	const FVector CamLoc = Cam->GetWorldLocation();
	const FVector CamForward = Cam->GetWorldRotation().Quaternion().GetForwardVector().Normalized();

	FRay Ray{ .Origin = CamLoc, .Direction = CamForward };
	FHitResult Hit;

	if (GetWorld()->LineTraceSingle(Ray, Hit, nullptr, nullptr, OwnerPawn))
	{
		if (Hit.HitActor && Hit.Distance <= PickupRange)
		{
			if (APortalCubeActor* Cube = Cast<APortalCubeActor>(Hit.HitActor))
			{
				HeldCube = Cube;
				HeldCube->OnPickedUp(OwnerPawn);
			}
		}
	}
}
