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
		const bool bIsFloorPortal = (std::abs(PortalNormal.Z) > 0.7f);

		FVector CurrentVel = ProjectileMovementComponent ? ProjectileMovementComponent->Velocity : FVector::ZeroVector;
		const float VelocityIntoPortal = FVector::Dot(CurrentVel, -PortalNormal);
		
		// 큐브 중심 기준 거리 0.60m 이내로 포탈 표면에 접촉했는지 검사
		const bool bInBounds = TargetPortal->IsPointWithinPortalBounds(CurrentLocation, 0.15f);
		const bool bTouchingPortalPlane = (DistToPlane <= 0.55f && DistToPlane >= -0.70f);

		// 포탈 진입 조건: 들려있거나, 바닥 포탈이거나, 포탈로 이동 중이거나, 이미 평면에 닿은 경우
		const bool bHeadingIntoPortal = bIsHeld || bIsFloorPortal || (VelocityIntoPortal > -0.2f) || (DistToPlane < 0.2f);

		// 포탈 개구부 안쪽이면 바닥/벽 콜라이더에 걸리지 않도록 Sweep을 끄고, 바깥이면 켠다
		if(ProjectileMovementComponent)
		{
			ProjectileMovementComponent->bSweepCollision = !bInBounds;
		}

		if(bInBounds && bTouchingPortalPlane && bHeadingIntoPortal)
		{
			APortalActor* ExitPortal = TargetPortal->GetLinkedPortal();
			if(ExitPortal)
			{
				const FMatrix TeleportMatrix = TargetPortal->GetTeleportMatrix();
				// 1. 위치 변환
				FVector NewLoc = TeleportMatrix.TransformPosition(CurrentLocation);

				// 출구 포탈의 안전 경계 내로 로컬 좌표 클램핑 (출구 모서리 벽/바닥 끼임 방지)
				FVector ExitLocal = ExitPortal->GetPortalMatrix().Inverse().TransformPosition(NewLoc);
				const float SafeHalfWidth = (ExitPortal->GetPortalWidth() * 0.5f) - 0.55f;
				const float SafeHalfHeight = (ExitPortal->GetPortalHeight() * 0.5f) - 0.55f;
				ExitLocal.Y = FMath::Clamp(ExitLocal.Y, -SafeHalfWidth, SafeHalfWidth);
				ExitLocal.Z = FMath::Clamp(ExitLocal.Z, -SafeHalfHeight, SafeHalfHeight);
				NewLoc = ExitPortal->GetPortalMatrix().TransformPosition(ExitLocal);

				// 큐브 대각선 반경(0.87m)을 고려하여 출구 벽 밖으로 0.9m 확실하게 배치
				NewLoc += ExitPortal->GetPortalNormal() * 0.9f;

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
		else if(!bInBounds && DistToPlane < 0.50f)
		{
			// 모서리 바깥 벽이나 바닥을 뚫고 들어간 경우: 안전장치 (표면 밖으로 밀어내고 반사)
			SetActorLocation(CurrentLocation + PortalNormal * (0.52f - DistToPlane));
			if(ProjectileMovementComponent)
			{
				const float NormalVel = FVector::Dot(ProjectileMovementComponent->Velocity, PortalNormal);
				if(NormalVel < 0.0f)
				{
					ProjectileMovementComponent->Velocity -= PortalNormal * (NormalVel * (1.0f + ProjectileMovementComponent->Bounciness));
				}
				ProjectileMovementComponent->bSweepCollision = true;
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
	ProjectileMovementComponent->bSimulationEnabled = true;
}

void APortalCubeActor::TeleportThroughPortal(const FVector& NewLoc, const FRotator& NewRot, const FVector& NewVel)
{
	SetActorLocation(NewLoc);
	SetActorRotation(NewRot);

	if(ProjectileMovementComponent)
	{
		ProjectileMovementComponent->Velocity = NewVel;
		ProjectileMovementComponent->bSimulationEnabled = true;
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
			if(ProjectileMovementComponent && Portal->IsPointWithinPortalBounds(GetActorLocation(), 0.15f))
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
