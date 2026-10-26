#include "EnginePCH.h"
#include "PortalCubeActor.h"
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

}

void APortalCubeActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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

	ProjectileMovementComponent->AddForce(SpringForce + DampingForce);
}
