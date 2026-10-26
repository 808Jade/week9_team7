#pragma once

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

class UBoxComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

class APortalCubeActor : public AActor
{
	DECLARE_CLASS(APortalCubeActor, AActor)

	REFLECT_START(ClassName)
		REFLECT_END()

public:
	APortalCubeActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	void OnGrabbed();
	void OnDropped(const FVector& LaunchVelocity = FVector::ZeroVector);
	void UpdateHoldPhysics(const FVector& TargetLocation, const FRotator& TargetRotation, float DeltaTime);

	UBoxComponent* GetBoxComponent() const { return BoxComponent; }
	UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovementComponent; }

private:
	UBoxComponent* BoxComponent = nullptr;
	UStaticMeshComponent* StaticMeshComponent = nullptr;
	UProjectileMovementComponent* ProjectileMovementComponent = nullptr;

	bool bIsHeld = false;
	APawn* HoldingPawn = nullptr;
	bool bIsGrounded = false;

	float PullStrength = 120.0f;
	float DampingRatio = 12.0f;
	float RotationInterSpeed = 10.0f;
};