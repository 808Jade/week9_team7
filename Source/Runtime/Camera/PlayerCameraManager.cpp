#include "EnginePCH.h"
#include "PlayerCameraManager.h"
#include "CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "CameraModifier.h"

APlayerCameraManager::APlayerCameraManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void APlayerCameraManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateCamera(DeltaTime);
}

void APlayerCameraManager::SetViewTarget(AActor* NewTarget)
{
	ViewTarget.TargetActor = NewTarget;
	if (NewTarget)
	{
		UpdateViewTarget(ViewTarget, 0.0f);
		CameraCachePrivate = ViewTarget.ViewInfo;
	}
}

void APlayerCameraManager::UpdateViewTarget(FViewTarget& OutVT, float DeltaTime)
{
	if (!OutVT.TargetActor) return;

	UCameraComponent* CamComp = OutVT.TargetActor->FindComponentByClass<UCameraComponent>();
	if (CamComp)
	{
		OutVT.ViewInfo.Location = CamComp->GetWorldLocation();
		OutVT.ViewInfo.Rotation = CamComp->GetWorldRotation();
		OutVT.ViewInfo.FOV = CamComp->GetFieldOfView();
		OutVT.ViewInfo.AspectRatio = CamComp->GetAspectRatio();
		OutVT.ViewInfo.NearClip = CamComp->GetNearZ();
		OutVT.ViewInfo.FarClip = CamComp->GetFarZ();
		OutVT.ViewInfo.bIsPerspective = !CamComp->GetIsOrthogonal();
		OutVT.ViewInfo.ViewMatrix = CamComp->GetViewMatrix();
		OutVT.ViewInfo.ProjectionMatrix = CamComp->GetProjectionMatrix();
	}
	else
	{
		OutVT.ViewInfo.Location = OutVT.TargetActor->GetActorLocation();
		OutVT.ViewInfo.Rotation = OutVT.TargetActor->GetActorRotation();
		OutVT.ViewInfo.FOV = 60.0f;
		OutVT.ViewInfo.NearClip = 0.1f;
		OutVT.ViewInfo.FarClip = 10000.0f;
		OutVT.ViewInfo.bIsPerspective = true;
	}
}

void APlayerCameraManager::UpdateCamera(float DeltaTime)
{
	UpdateViewTarget(ViewTarget, DeltaTime);
	FViewInfo InOutPOV = ViewTarget.ViewInfo;

	for (UCameraModifier* Mod : ModifierList)
	{
		if (Mod && !Mod->IsDisabled())
		{
			Mod->ModifyCamera(DeltaTime, InOutPOV);
		}
	}

	FMatrix RotationMatrix = InOutPOV.Rotation.Quaternion().ToFMatrix();
	FMatrix InverseTranslation = FMatrix::MakeTranslation(InOutPOV.Location * -1.0f);
	FMatrix InverseRotation = RotationMatrix.GetTransposed();
	InOutPOV.ViewMatrix = InverseTranslation * InverseRotation;

	const float HalfFOV = FMath::DegreesToRadians(InOutPOV.FOV) * 0.5f;
	const float YScale = 1.0f / tan(HalfFOV);
	const float XScale = YScale / (InOutPOV.AspectRatio > 0.0f ? InOutPOV.AspectRatio : (16.0f / 9.0f));
	const float ZScale = InOutPOV.FarClip / (InOutPOV.FarClip - InOutPOV.NearClip);
	const float ZOffset = -InOutPOV.NearClip * InOutPOV.FarClip / (InOutPOV.FarClip - InOutPOV.NearClip);

	InOutPOV.ProjectionMatrix = FMatrix(
		0.0f, 0.0f, ZScale, 1.0f,
		XScale, 0.0f, 0.0f, 0.0f,
		0.0f, YScale, 0.0f, 0.0f,
		0.0f, 0.0f, ZOffset, 0.0f
	);

	CameraCachePrivate = InOutPOV;
}

void APlayerCameraManager::AddCameraModifier(UCameraModifier* NewModifier)
{
	if (!NewModifier) return;

	for (UCameraModifier* Existing : ModifierList)
	{
		if (Existing == NewModifier) return;
	}

	NewModifier->Init(this);
	ModifierList.Add(NewModifier);
}

void APlayerCameraManager::RemoveCameraModifier(UCameraModifier* ModifierToRemove)
{
	if (!ModifierToRemove) return;
	ModifierList.Remove(ModifierToRemove);
}
