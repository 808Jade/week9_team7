#pragma once

#include "GameFramework/Actor.h"

class UStaticMeshComponent;
class UPointLightComponent;
class APortalCubeActor;
class APortalPawn;
class APortalActor;
struct FHitResult;
class UCameraModifier_Recoil;

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
		PROPERTY(MuzzleLocalOffset)
		PROPERTY(LaserCoreRadius)
		PROPERTY(LaserGlowRadius)
		PROPERTY(LaserBendRatio)
		PROPERTY(LaserLightIntensity)
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

	void Swing();
	bool IsSwinging() const { return bIsSwinging; }

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
	UCameraModifier_Recoil* RecoilModifier = nullptr;

	bool bIsSwinging = false;
	float SwingTimer = 0.0f;
	const float SwingDuration = 0.22f;
	// ===== 홀드 레이저 =====
	// 총구 → 큐브를 2차 베지어 곡선으로 잇는다.
	// 곡선을 LaserSegmentCount개 구간으로 나누고, 구간마다 원기둥(코어 + 외곽)을 하나씩 배치
	static constexpr int32 LaserSegmentCount = 16;
	TArray<UStaticMeshComponent*> LaserCoreSegments;
	TArray<UStaticMeshComponent*> LaserGlowSegments;
	float LaserBendRatio = 0.6f;      // 제어점 거리 비율 (클수록 많이 휜다)

	FVector MuzzleLocalOffset = FVector(0.35f, 0.0f, 0.05f); // 총 메시 기준 총구 위치
	float LaserCoreRadius = 0.004f;   // 코어 반지름 (m)
	float LaserGlowRadius = 0.012f;   // 외곽 반지름 (m)
	float LaserAlpha = 0.0f;          // 0~1, 켜고 끌 때 굵기 보간
	float LaserTime = 0.0f;

	// ===== 레이저 조명 =====
	// 총구 / 곡선 중간 / 큐브 앞, 세 곳에 포인트 라이트를 두고 곡선을 따라 이동시킨다
	UPointLightComponent* MuzzleLight = nullptr;
	UPointLightComponent* BeamLight = nullptr;
	UPointLightComponent* CubeLight = nullptr;
	float LaserLightIntensity = 1.0f; // 세 라이트 공통 배율

	UPointLightComponent* CreateLaserLight(const char* Name, float AttenuationRadius);
	void UpdateLaserLights(const FVector& P0, const FVector& P1, const FVector& P2, float Pulse);
	void TurnOffLaserLights();

	UStaticMeshComponent* CreateLaserComponent(const std::string& Name, const char* MaterialKey);
	void UpdateLaser(float DeltaTime);
	void SetLaserVisibility(bool bVisible);
	static FVector QuadBezier(const FVector& P0, const FVector& P1, const FVector& P2, float T);
	// 원기둥(로컬 Z축, 높이 1, 반지름 1)을 Start→End 방향으로 늘려 배치
	static void PlaceCylinder(UStaticMeshComponent* Comp, const FVector& Start, const FVector& End, float Radius);

	void PlayFireSound(EPortalType Type);
};
