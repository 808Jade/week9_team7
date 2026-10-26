#pragma once

#include "GameFramework/Actor.h"

class UStaticMeshComponent;
class APortalCubeActor;
class APortalPawn;
struct FHitResult;

enum class EPortalType : int32
{
	Blue = 0,
	Orange = 1
};

class APortalGunActor : public AActor
{
	DECLARE_CLASS(APortalGunActor, AActor)

	REFLECT_START(ClassName)
		PROPERTY(HoldDistance)
		PROPERTY(PickupRange)
		REFLECT_END()

public:
	APortalGunActor();

	virtual void BeginPlay() override;
	virtual void EndPlay() override;
	virtual void Tick(float DeltaTime) override;

	// 폰의 카메라에 포탈건 장착
	void AttachToPawn(APortalPawn* InOwnerPawn);

	// 세부 동작	
	void FirePortal(const EPortalType PortalToFire, const FHitResult& HitResult);
	void Hold(APortalCubeActor* CubeToGrab);
	void Unhold();

	UStaticMeshComponent* GetGunMeshComponent() const { return GunMeshComponent; }
	const bool IsHoldingCube() { return HeldCube != nullptr; }

private:
	// 포탈건 3D 메시 컴포넌트
	UStaticMeshComponent* GunMeshComponent = nullptr;

	// 소유자 폰
	APortalPawn* OwnerPawn = nullptr;

	// 큐브 홀딩 상태
	APortalCubeActor* HeldCube = nullptr;
	float HoldDistance = 2.0f;     // 카메라 앞 2m에 큐브를 띄움
	float MaxBreakDistance = 3.5f;	

	// 1인칭 카메라 기준 포탈건 오프셋 (전방 40cm, 우측 25cm, 아래 25cm)
	FVector GunOffsetLocation = FVector(0.4f, 0.25f, -0.25f);

	// FRotator(Pitch(상하), Yaw(좌우), Roll(기울기))
	FRotator GunOffsetRotation = FRotator(0.0f, 180.0f, 0.0f);
};
