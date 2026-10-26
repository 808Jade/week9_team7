#include "EnginePCH.h"
#include "PortalCubeActor.h"
#include "PortalActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ProjectileMovementComponent.h"
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
	StaticMeshComponent->SetRelativeScale3D(FVector(0.25f, 0.25f, 0.25f));

	UStaticMesh* CubeMesh = UAssetManager::GetAssetByPath<UStaticMesh>("Models/PortalCube/portal_cube.obj");
	if (CubeMesh)
	{
		StaticMeshComponent->SetStaticMesh(CubeMesh);
	}

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileComponent");
	ProjectileMovementComponent->SetVelocityInLocalSpace(FVector(0.0f, 0.0f, 0.0f));
	ProjectileMovementComponent->SetUpdatedComponent(GetRootComponent());
	ProjectileMovementComponent->bShouldBounce = true;
	ProjectileMovementComponent->Bounciness = 0.3f;
	ProjectileMovementComponent->Friction = 0.4f;
	ProjectileMovementComponent->MaxSpeed = 1000.0f;
}

void APortalCubeActor::BeginPlay()
{
	Super::BeginPlay();

	PreviousLocation = GetActorLocation();
	if(BoxComponent)
	{
		BoxComponent->OnComponentBeginOverlap.AddDynamic(this, &APortalCubeActor::OnBeginOverlap);
		BoxComponent->OnComponentEndOverlap.AddDynamic(this, &APortalCubeActor::OnEndOverlap);	
	}
}

void APortalCubeActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(!bIsHeld)
	{
		ProjectileMovementComponent->MaxSpeed = 1000.0f;
	}
	else
	{
		ProjectileMovementComponent->MaxSpeed = 7.0f;
	}

	if(TeleportCooldownTimer > 0.0f)
	{
		TeleportCooldownTimer -= DeltaTime;
	}

	const FVector CurrentLocation = GetActorLocation();
	APortalActor* TargetPortal = (CurrentOverlappingPortal && CurrentOverlappingPortal->IsActive()) ? CurrentOverlappingPortal : nullptr;
	if(TargetPortal && CanTeleport())
	{
		const FVector PortalNormal = TargetPortal->GetPortalNormal();
		const float DistToPlane = TargetPortal->GetDistanceToPlane(CurrentLocation);

		FVector CurrentVel = ProjectileMovementComponent ? ProjectileMovementComponent->Velocity : FVector::ZeroVector;
		const float VelocityIntoPortal = FVector::Dot(CurrentVel, -PortalNormal);
		// 큐브 중심 기준 거리 0.55m (큐브 반경 0.5m) 이내로 포탈 표면에 접촉했는지 검사
		// 1) 포탈 안쪽으로 이동 중이거나 들려 있는 상태
		// 2) 포탈 표면 0.55m 이내에 도달
		// 3) 포탈 개구부(가로/세로 범위) 내부인지 검사
		const bool bHeadingIntoPortal = (VelocityIntoPortal > -0.1f) || bIsHeld;
		const bool bTouchingPortalPlane = (DistToPlane <= 0.55f && DistToPlane >= -0.55f);
		if(bHeadingIntoPortal && bTouchingPortalPlane && TargetPortal->IsPointWithinPortalBounds(CurrentLocation))
		{
			APortalActor* ExitPortal = TargetPortal->GetLinkedPortal();
			if(ExitPortal)
			{
				const FMatrix TeleportMatrix = TargetPortal->GetTeleportMatrix();
				// 1. 위치 변환: 큐브 반경(0.5m)을 고려하여 출구 벽 밖으로 0.6m 확실하게 배치
				FVector NewLoc = TeleportMatrix.TransformPosition(CurrentLocation);
				NewLoc += ExitPortal->GetPortalNormal() * 0.6f;
				// 2. 회전 변환
				FMatrix RotMat = GetActorRotation().Quaternion().ToFMatrix() * TeleportMatrix;
				FRotator NewRot = MatrixToRotator(RotMat);
				// 3. 속도 벡터 변환
				FVector NewVel = TeleportMatrix.TransformVector(CurrentVel);
				// 4. 최소 사출 속도 보정 (벽에서 시원하게 튀어나오도록)
				const FVector ExitNormal = ExitPortal->GetPortalNormal();
				const float OutwardSpeed = FVector::Dot(NewVel, ExitNormal);
				const float MinExitSpeed = 3.5f;
				if(OutwardSpeed < MinExitSpeed)
				{
					NewVel += ExitNormal * (MinExitSpeed - OutwardSpeed);
				}
				// 5. 들려 있는 상태에서 들어갔다면 홀딩 해제(자연스럽게 사출)
				if(bIsHeld)
				{
					OnDropped(NewVel);
				}
				TeleportThroughPortal(NewLoc, NewRot, NewVel);

				if(ProjectileMovementComponent)
				{
					ProjectileMovementComponent->bSweepCollision = true;
				}

				TeleportCooldownTimer = 0.25f;
				CurrentOverlappingPortal = nullptr;
			}
		}
	}
	PreviousLocation = GetActorLocation();
}

void APortalCubeActor::OnGrabbed()
{
	bIsHeld = true;
	if(ProjectileMovementComponent)
	{
		ProjectileMovementComponent->Velocity = FVector::ZeroVector;
		ProjectileMovementComponent->ProjectileGravityScale = 0.0f;
		ProjectileMovementComponent->bSimulationEnabled = true;
	}
}

void APortalCubeActor::OnDropped(const FVector& LaunchVelocity)
{
	bIsHeld = false;
	if(ProjectileMovementComponent)
	{		
		ProjectileMovementComponent->ProjectileGravityScale = 1.0f;
		ProjectileMovementComponent->bSimulationEnabled = true;
	}	
}

void APortalCubeActor::UpdateHoldPhysics(const FVector& TargetLocation, const FRotator& TargetRotation, float DeltaTime)
{
	if(!bIsHeld || !ProjectileMovementComponent)
	{
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	const FVector Displacement = TargetLocation - CurrentLocation;
	const FVector CurrentVelocity = ProjectileMovementComponent->Velocity;
	
	const FVector SpringForce = Displacement * PullStrength;
	
	const FVector DampingForce = -CurrentVelocity * DampingRatio;

	ProjectileMovementComponent->MaxSpeed = 7.0f;
	ProjectileMovementComponent->AddForce(SpringForce + DampingForce);
}

void APortalCubeActor::TeleportThroughPortal(const FVector& NewLoc, const FRotator& NewRot, const FVector& NewVel)
{
	SetActorLocation(NewLoc);
	SetActorRotation(NewRot);

	if(ProjectileMovementComponent)
	{
		ProjectileMovementComponent->Velocity = NewVel;
	}
}

void APortalCubeActor::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, bool bFromSweep, const FHitResult& SweepResult)
{
	if(!CanTeleport())
	{
		return;
	}

	if(APortalActor* Portal = Cast<APortalActor>(OtherActor))
	{
		if(Portal->IsActive())
		{
			CurrentOverlappingPortal = Portal;

			if(ProjectileMovementComponent)
			{
				ProjectileMovementComponent->bSweepCollision = false;
			}
		}
	}
}

void APortalCubeActor::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp)
{
	if(OtherActor == CurrentOverlappingPortal)
	{
		CurrentOverlappingPortal = nullptr;

		if(ProjectileMovementComponent)
		{
			ProjectileMovementComponent->bSweepCollision = true;
		}
	}
}
