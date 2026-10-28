#pragma once

#include "GameFramework/Actor.h"

class UStaticMeshComponent;
class APortalCubeActor;
class APortalPawn;
class APortalActor;
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
		PROPERTY(PickUpRange)
		PROPERTY(MaxBreakDistance)
		REFLECT_END()

public:
	APortalGunActor();

	virtual void BeginPlay() override;
	virtual void EndPlay() override;
	virtual void Tick(float DeltaTime) override;

	// 폰의 카메라에 포탈건 장착
	void AttachToPawn(APortalPawn* InOwnerPawn);

	// 세부 동작
	void FirePortal(const EPortalType PortalToFire, const FHitResult& HitResult, const FVector& ViewerForward = FVector::ZeroVector);
	void Hold(APortalCubeActor* CubeToGrab);
	void Unhold();
	void ResetPortals();

	UStaticMeshComponent* GetGunMeshComponent() const { return GunMeshComponent; }
	const bool IsHoldingCube() { return HeldCube != nullptr; }
	const float GetPickUpRange() { return PickUpRange; }

	// 색에 맞는 포탈 (BeginPlay에서 미리 스폰)
	APortalActor* GetPortal(EPortalType Type) const
	{
		return Type == EPortalType::Blue ? BluePortal : OrangePortal;
	}

private:
	// 포탈건 3D 메시 컴포넌트
	UStaticMeshComponent* GunMeshComponent = nullptr;

	// 소유자 폰
	APortalPawn* OwnerPawn = nullptr;

	// 포탈 2개 (게임 시작 시 숨김 상태로 스폰, 쏠 때마다 이동만)
	APortalActor* BluePortal = nullptr;
	APortalActor* OrangePortal = nullptr;

	// 큐브 홀딩 상태
	APortalCubeActor* HeldCube = nullptr;
	float HoldDistance = 2.0f;     // 카메라 앞 2m에 큐브를 띄움
	float PickUpRange = 3.0f;
	float MaxBreakDistance = 4.0f;

	// 1인칭 카메라 기준 포탈건 오프셋 (전방 40cm, 우측 25cm, 아래 25cm)
	FVector GunOffsetLocation = FVector(0.4f, 0.25f, -0.25f);

	// FRotator(Pitch(상하), Yaw(좌우), Roll(기울기))
	FRotator GunOffsetRotation = FRotator(0.0f, 180.0f, 0.0f);
};
