#pragma once

#include "GameFramework/Pawn.h"

class USphereComponent;
class UCameraComponent;

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
	virtual void Tick(float DeltaTime) override;

	// 이동 및 시선 회전 입력
	virtual void AddMovementInput(const FVector& WorldDirection, float ScaleValue) override;
	virtual void AddControllerYawInput(float Value) override;
	virtual void AddControllerPitchInput(float Value) override;
	virtual void PostEditChangeProperty(const FProperty& Property) override;

	// 점프
	void Jump();
	bool IsGrounded() const { return bIsGrounded; }

	USphereComponent* GetSphereComponent() const { return SphereComponent; }
	UCameraComponent* GetCameraComponent() const { return CameraComponent; }

private:
	// 컴포넌트
	USphereComponent* SphereComponent = nullptr; // 루트 물리 콜라이더
	UCameraComponent* CameraComponent = nullptr; // 1인칭 눈높이 카메라

	// 이동 & 시선 파라미터
	float MoveSpeed = 15.0f;
	float LookSensitivity = 1.0f;
	float CollisionRadius = 1.0f;
	float EyeHeight = 1.6f; // 구 중심에서 카메라 눈높이까지의 높이(Z)

	// 중력, 점프
	bool bEnableGravity = true;
	float Gravity = 9.8f;
	float JumpVelocity = 10.0f;
	float VerticalVelocity = 0.0f;
	bool bIsGrounded = false;
};