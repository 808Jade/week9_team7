#pragma once

#include "GameFramework/Pawn.h"

class USphereComponent;
class UCameraComponent;
class APortalGunActor;

class APortalPawn : public APawn
{
	DECLARE_CLASS(APortalPawn, APawn)

	REFLECT_START(ClassName)
		PROPERTY(MoveSpeed)
		PROPERTY(LookSensitivity)
		PROPERTY(CollisionRadius)
		PROPERTY(EyeHeight)
		PROPERTY(bEnableGravity)
		PROPERTY(Gravity)
		PROPERTY(JumpVelocity)
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

	// 포탈건 액터로 명령 위임
	virtual void PrimaryAction();
	virtual void TogglePortalType();
	virtual void Interact();

	APortalGunActor* GetPortalGun() const { return PortalGun; }
	USphereComponent* GetSphereComponent() const { return SphereComponent; }
	UCameraComponent* GetCameraComponent() const { return CameraComponent; }

private:
	// 컴포넌트
	USphereComponent* SphereComponent = nullptr; // 루트 물리 콜라이더
	UCameraComponent* CameraComponent = nullptr; // 1인칭 눈높이 카메라

	// 이동 & 시선 파라미터
	float MoveSpeed = 6.0f;
	float LookSensitivity = 1.0f;
	float CollisionRadius = 0.5f;
	float EyeHeight = 1.1f;

	// 중력, 점프
	bool bEnableGravity = true;
	float Gravity = 9.8f;
	float JumpVelocity = 5.5f;
	float VerticalVelocity = 0.0f;
	bool bIsGrounded = false;

	// 보유 중인 포탈건 액터
	APortalGunActor* PortalGun = nullptr;
};
