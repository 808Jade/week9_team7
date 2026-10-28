#include "EnginePCH.h"
#include "PlayerCameraManager.h"
#include "CameraComponent.h"
#include "GameFramework/PlayerController.h"

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
	// 1. 현재 뷰타겟 정보 업데이트
	UpdateViewTarget(ViewTarget, DeltaTime);

	// 2. 최종 렌더 캐시에 저장
	CameraCachePrivate = ViewTarget.ViewInfo;
}
