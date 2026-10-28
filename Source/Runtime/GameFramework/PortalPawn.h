#pragma once

#include "GameFramework/Pawn.h"
#include "Math/Vector.h"

class USphereComponent;
class UBoxComponent;
class UCapsuleComponent;
class UCameraComponent;
class UStaticMeshComponent;
class APortalGunActor;
class APlayerStart;
class APortalActor;
enum class EPortalType : int32;   // 정의: PortalGunActor.h

class APortalPawn : public APawn
{
	DECLARE_CLASS(APortalPawn, APawn)

	REFLECT_START(ClassName)
		PROPERTY(MoveSpeed)
		PROPERTY(SprintSpeed)
		PROPERTY(CrouchSpeed)
		PROPERTY(LookSensitivity)
		PROPERTY(CollisionRadius)
		PROPERTY(CrouchRadius)
		PROPERTY(EyeHeight)
		PROPERTY(CrouchEyeHeight)
		PROPERTY(StandHalfHeight)
		PROPERTY(CrouchHalfHeight)
		PROPERTY(bEnableGravity)
		PROPERTY(Gravity)
		PROPERTY(JumpVelocity)
		PROPERTY(AirAcceleration)
		PROPERTY(GroundFriction)
		PROPERTY(BrakingDeceleration)
		PROPERTY(WallFriction)
		PROPERTY(MaxStepHeight)
		PROPERTY(KillZ)
		PROPERTY(bEnableHeadBob)
		PROPERTY(Velocity)
		PROPERTY(CameraFarZ)
		REFLECT_END()

public:
	APortalPawn();

	virtual void BeginPlay() override;
	virtual void EndPlay() override;
	virtual void Tick(float DeltaTime) override;

	// 이동 및 시선 회전 입력
	virtual void AddMovementInput(const FVector& WorldDirection, float ScaleValue) override;
	virtual void AddControllerYawInput(float Value) override;
	virtual void AddControllerPitchInput(float Value) override;
	virtual void PostEditChangeProperty(const FProperty& Property) override;

	// 점프
	void Jump();
	bool IsGrounded() const { return bIsGrounded; }

	// 속도 및 물리
	FVector GetVelocity() const { return Velocity; }
	void SetVelocity(const FVector& InVelocity) { Velocity = InVelocity; }

	// 달리기 (Sprint)
	void SetSprinting(bool bSprint) { bIsSprinting = bSprint; }
	bool IsSprinting() const { return bIsSprinting; }

	// 웅크리기 (Crouch)
	void SetCrouching(bool bCrouch) { bIsCrouched = bCrouch; }
	void ToggleCrouch() { bIsCrouched = !bIsCrouched; }
	bool IsCrouched() const { return bIsCrouched; }

	float GetZoomScale() const;

	// 사망 및 리스폰
	void Respawn();
	void KillPlayer() { Respawn(); }
	float GetKillZ() const { return KillZ; }
	void SetKillZ(float InKillZ) { KillZ = InKillZ; }

	// 포탈 순간이동 수신 인터페이스 & 쿨다운
	void TeleportThroughPortal(const FVector& NewLocation, const FRotator& NewRotation, const FVector& NewVelocity);
	bool CanTeleport() const { return TeleportCooldownTimer <= 0.0f; }
	float GetTeleportCooldownTimer() const { return TeleportCooldownTimer; }
	void SetTeleportCooldown(float Duration = 0.2f) { TeleportCooldownTimer = Duration; }

	// 포탈건 액터로 명령 위임
	// 포탈 상태 및 리셋
	bool IsBluePortalPlaced() const;
	bool IsOrangePortalPlaced() const;
	void ResetPortals();

	virtual void PrimaryAction();     // 좌클릭: 파란 포탈
	virtual void SecondaryAction();   // 우클릭: 주황 포탈
	virtual void TogglePortalType();
	virtual void Interact();

	APortalGunActor* GetPortalGun() const { return PortalGun; }
	USphereComponent* GetSphereComponent() const { return SphereComponent; }
	UCameraComponent* GetCameraComponent() const { return CameraComponent; }
	float GetLookSensitivity() const { return LookSensitivity; }
	void SetLookSensitivity(float InSens) { LookSensitivity = InSens; }

	UStaticMeshComponent* GetMeshComponent() const { return StaticMeshComponent; }
	float GetCameraFarZ() const { return CameraFarZ; }
	void SetCameraFarZ(float InFarZ);

private:
	// 카메라 정면으로 트레이스해서 포탈건에 발사 위임
	void FirePortal(EPortalType Type);

	// 컴포넌트
	USphereComponent* SphereComponent = nullptr; // 루트 물리 콜라이더
	UBoxComponent* BoxComponent = nullptr;
	UCapsuleComponent* CapsuleComponent = nullptr;
	UCameraComponent* CameraComponent = nullptr; // 1인칭 눈높이 카메라
	UStaticMeshComponent* StaticMeshComponent = nullptr;

	// 이동 파라미터
	float MoveSpeed = 6.0f;
	float SprintSpeed = 9.0f;
	float CrouchSpeed = 3.0f;
	float LookSensitivity = 1.0f;
	float CameraFarZ = 100.0f;

	// 물리 마찰력, 제동 및 계단 파라미터
	float AirAcceleration = 2.0f;          // 공중 조향 가속도 (공중에서 최고 속도 제한 적용)
	float GroundFriction = 12.0f;          // 지상 마찰력 계수
	float BrakingDeceleration = 35.0f;     // 키를 뗐을 때 즉시 멈추는 제동력 (얼음판 슬라이딩 완전 제거)
	float WallFriction = 0.25f;            // 벽면 미끄러짐 시 마찰력 (0.0: 얼음벽, 1.0: 즉시 멈춤)
	float MaxStepHeight = 0.4f;            // 계단 및 문턱 최대 넘기 높이 (0.5m = 50cm)
	float MaxPreservedFlingSpeed = 9.0f;    // 포탈 플링 시 보존되는 수평 최대 속도 한도

	// 포탈 재진입 핑퐁 방지 쿨다운
	float TeleportCooldownTimer = 0.0f;    // 0보다 크면 포탈 순간이동 재진입 불가
	float TeleportCooldownDuration = 0.2f; // 포탈 통과 직후 쿨다운 시간(0.2초)

	// 사망 및 리스폰
	float KillZ = -40.0f;                  // 이 높이 이하로 추락하면 사망 및 리스폰
	FVector InitialSpawnLocation = FVector::ZeroVector;
	FRotator InitialSpawnRotation = FRotator(0.0f, 0.0f, 0.0f);

	// 시각적 연출: 착지 반동 & 헤드 밥빙
	float CurrentLandingDip = 0.0f;        // 착지 시 카메라가 아래로 내려앉는 오프셋
	float LandingRecoveryScale = 1.0f;     // 낙하 속도에 따른 착지 딥 복귀 시간 배율
	bool bEnableHeadBob = true;            // 헤드 밥빙 활성화 여부
	float BobTimer = 0.0f;                 // 밥빙 타이머
	float BobAmountVertical = 0.03f;       // 밥빙 상하 흔들림 폭 (3cm)
	float BobAmountHorizontal = 0.015f;    // 밥빙 좌우 흔들림 폭 (1.5cm)
	float CurrentBobZ = 0.0f;
	float CurrentBobX = 0.0f;

	// 콜라이더 및 눈높이
	float CollisionRadius = 0.5f;
	float CrouchRadius = 0.35f;
	float EyeHeight = 1.0f;
	float CrouchEyeHeight = 0.5f;
	float CurrentEyeHeight = 1.0f;

	// 캡슐 반높이 (반지름 포함). 서 있을 때 1.5 → 키 3m, 눈높이 = 1.5 + EyeHeight
	float StandHalfHeight = 1.5f;
	float CrouchHalfHeight = 0.9f;
	float CurrentHalfHeight = 1.5f;

	// 중력, 점프, 3D 속도
	bool bEnableGravity = true;
	float Gravity = 9.8f * 1.5f;
	float JumpVelocity = 5.5f;
	FVector Velocity = FVector::ZeroVector;
	FVector PendingInputVector = FVector::ZeroVector;
	bool bIsGrounded = false;

	// 상태 플래그
	bool bIsSprinting = false;
	bool bIsCrouched = false;

	// 줌 FOV
	float DefaultFOV = 60.0f;

	// 보유 중인 포탈건 액터
	APortalGunActor* PortalGun = nullptr;
	APortalActor* CurrentOverlappingPortal = nullptr;

	int32 WalkVoiceID = -1;
	float WalkStopDelayTimer = 0.0f;

	void PlayLandSound(float ImpactSpeed);
	void PlayWarpSound();
	void UpdateWalkSound(float DeltaTime);

	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, bool bFromSweep, const FHitResult& SweepResult);
	void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp);
};
