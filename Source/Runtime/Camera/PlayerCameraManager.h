#pragma once

#include "GameFramework/Actor.h"
#include "Math/Vector.h"
#include "Math/Rotator.h"
#include "Math/Matrix.h"
#include "Math/Color.h"

class APlayerController;

struct FViewInfo
{
	FVector Location = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
	FMatrix ViewMatrix = FMatrix::Identity;
	FMatrix ProjectionMatrix = FMatrix::Identity;
	float FOV = 60.0f;
	float AspectRatio = 16.0f / 9.0f;
	float NearClip = 0.1f;
	float FarClip = 300.0f;
	bool bIsPerspective = true;
	bool bLetterBox = false;

	float FadeAlpha = 0.0f;
	FColor FadeColor = FColor::Black;
};

struct FViewTarget
{
	AActor* TargetActor = nullptr;
	FViewInfo ViewInfo;
};

class APlayerCameraManager : public AActor
{
	DECLARE_CLASS(APlayerCameraManager, AActor)

public:
	virtual void UpdateCamera(float DeltaTime);
	APlayerCameraManager();
	virtual void Tick(float DeltaTime) override;
	virtual void UpdateViewTarget(FViewTarget& OutVT, float DeltaTime);

	void SetViewTargetWithBlend(AActor* NewTarget, float BlendTime, /*EViewTargetBlendFunction BlendFunc = EViewTargetBlendFunction::VTBlend_Cubic,*/ float BlendExp = 2.0f, bool bLockOutgoing = false);

	void StartCameraFade(float FromAlpha, float ToAlpha, float Duration);

	void AddCameraModifier();
	void RemoveCameraModifier();

	void PlayCameraShake();

	void SetOwnerController(APlayerController* InController) { PCOwner = InController; }
	APlayerController* GetOwnerController() const { return PCOwner; }
	const FViewTarget& GetViewTarget() const { return ViewTarget; }
	void SetViewTarget(AActor* NewTarget);


	const FViewInfo& GetCameraViewInfo() const { return CameraCachePrivate; }

protected:
	APlayerController* PCOwner = nullptr;
	FViewTarget ViewTarget;
	FViewTarget PendingViewTarget;
	FViewInfo CameraCachePrivate;
};
