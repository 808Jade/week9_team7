#pragma once

#include "GameFramework/Actor.h"
#include "CameraModifier.h"

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
	APlayerCameraManager();

	virtual void UpdateCamera(float DeltaTime);
	virtual void Tick(float DeltaTime) override;
	virtual void UpdateViewTarget(FViewTarget& OutVT, float DeltaTime);

	void SetViewTargetWithBlend(AActor* NewTarget, float BlendTime, /*EViewTargetBlendFunction BlendFunc = EViewTargetBlendFunction::VTBlend_Cubic,*/ float BlendExp = 2.0f, bool bLockOutgoing = false);

	void StartCameraFade(float FromAlpha, float ToAlpha, float Duration);

	void AddCameraModifier(UCameraModifier* NewModifier);
	void RemoveCameraModifier(UCameraModifier* ModifierToRemove);

	void PlayCameraShake(float ShakePower, float Duration);

	void SetOwnerController(APlayerController* InController) { PCOwner = InController; }
	APlayerController* GetOwnerController() const { return PCOwner; }
	const FViewTarget& GetViewTarget() const { return ViewTarget; }
	void SetViewTarget(AActor* NewTarget);

	const FViewInfo& GetCameraViewInfo() const { return CameraCachePrivate; }

	template<typename T>
	T* AddNewCameraModifier()
	{
		for (UCameraModifier* Existing : ModifierList)
		{
			if (T* Casted = dynamic_cast<T*>(Existing))
			{
				return Casted;
			}
		}
		T* NewMod = NewObject<T>(this);
		if (!NewMod)
		{
			NewMod = new T();
		}
		NewMod->Init(this);
		ModifierList.Add(NewMod);
		return NewMod;
	}

	void SetManualFade(float InAlpha, const FColor& InColor = FColor::Black)
	{
		FadeAlpha = InAlpha;
		FadeColor = InColor;
	}
	float GetFadeAlpha() const { return FadeAlpha; }
protected:
	APlayerController* PCOwner = nullptr;
	FViewTarget ViewTarget;
	FViewTarget PendingViewTarget;
	FViewInfo CameraCachePrivate;

	TArray<UCameraModifier*> ModifierList;

	float FadeAlpha = 0.0f;
	FColor FadeColor = FColor::Black;
};
