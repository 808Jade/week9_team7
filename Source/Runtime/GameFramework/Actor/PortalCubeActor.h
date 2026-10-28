#pragma once

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

class UBoxComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class APortalActor;

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

	float PullStrength = 240.0f;
	float DampingRatio = 12.0f;
	float RotationInterSpeed = 10.0f;

public:
	void TeleportThroughPortal(const FVector& NewLoc, const FRotator& NewRot, const FVector& NewVel);
	bool CanTeleport() const { return TeleportCooldownTimer <= 0.0f; }

private:
	APortalActor* CurrentOverlappingPortal = nullptr;
	FVector PreviousLocation = FVector::ZeroVector;
	float TeleportCooldownTimer = 0.0f;
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, bool bFromSweep, const FHitResult& SweepResult);
	void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp);

};