#pragma once

#include "GameFramework/Actor.h"

class UStaticMeshComponent;
class APortalCubeActor;
class APortalPawn;

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
		PROPERTY(ThrowStrength)
		PROPERTY(PickupRange)
		REFLECT_END()

public:
	APortalGunActor();

	virtual void BeginPlay() override;
	virtual void EndPlay() override;
	virtual void Tick(float DeltaTime) override;

	// 폰의 카메라에 포탈건 장착
	void AttachToPawn(APortalPawn* InOwnerPawn);

	// [좌클릭] 큐브 들고 있으면 던지기, 빈손이면 포탈 발사
	void PrimaryAction();

	// [우클릭] Blue <-> Orange 모드 전환
	void TogglePortalType();

	// 세부 동작
	void ThrowCube();
	void FirePortal();

	// [E 키] 큐브 집기 / 발밑에 살포시 내려놓기
	void Interact();

	EPortalType GetCurrentPortalType() const { return CurrentPortalType; }
	APortalCubeActor* GetHeldCube() const { return HeldCube; }
	UStaticMeshComponent* GetGunMeshComponent() const { return GunMeshComponent; }

private:
	// 포탈건 3D 메시 컴포넌트
	UStaticMeshComponent* GunMeshComponent = nullptr;

	// 소유자 폰
	APortalPawn* OwnerPawn = nullptr;

	// 포탈 상태
	EPortalType CurrentPortalType = EPortalType::Blue;

	// 큐브 홀딩 상태
	APortalCubeActor* HeldCube = nullptr;
	float HoldDistance = 2.0f;     // 카메라 앞 2m에 큐브를 띄움
	float ThrowStrength = 12.0f;   // 좌클릭 던지기 속도
	float PickupRange = 3.5f;      // E키 탐색 가능 거리

	// 1인칭 카메라 기준 포탈건 오프셋 (전방 40cm, 우측 25cm, 아래 25cm)
	FVector GunOffsetLocation = FVector(0.4f, 0.25f, -0.25f);

	// FRotator(Pitch(상하), Yaw(좌우), Roll(기울기))
	FRotator GunOffsetRotation = FRotator(0.0f, 180.0f, 0.0f);
};
