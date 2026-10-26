#pragma once

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

class UBoxComponent;
class UStaticMeshComponent;

class APortalCubeActor : public AActor
{
	DECLARE_CLASS(APortalCubeActor, AActor)

	REFLECT_START(ClassName)
		PROPERTY(Gravity)
		PROPERTY(bEnableGravity)
		REFLECT_END()

public:
	APortalCubeActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// 플레이어가 집어 들었을 때 / 놓거나 던졌을 때
	void OnPickedUp(APawn* InHolder);
	void OnDropped(const FVector& InThrowVelocity);

	bool IsHeld() const { return bIsHeld; }
	UBoxComponent* GetBoxComponent() const { return BoxComponent; }

private:
	UBoxComponent* BoxComponent = nullptr;
	UStaticMeshComponent* StaticMeshComponent = nullptr;

	bool bIsHeld = false;
	APawn* HoldingPawn = nullptr;

	// 중력 & 물리 파라미터
	bool bEnableGravity = true;
	float Gravity = 9.8f;
	FVector Velocity = FVector::ZeroVector;
	bool bIsGrounded = false;
};