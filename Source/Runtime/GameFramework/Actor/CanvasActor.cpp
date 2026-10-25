#include "EnginePCH.h"
#include "CanvasActor.h"
#include "Camera/CameraActor.h"
#include "Engine/World.h"

ACanvasActor::ACanvasActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ACanvasActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    ACameraActor* Camera = GetWorld() ? GetWorld()->GetMainCamera() : nullptr;
    if (Camera && bFollowCamera)
    {
        FVector CameraLocation = Camera->GetActorLocation();

        // 1. 카메라의 전방 벡터
        FVector CameraForward = Camera->GetActorTransform().GetForward();

        // 2. 카메라 앞 일정 거리 위치 계산
        FVector NewLocation = CameraLocation + CameraForward * CameraDistance;

        // 3. 카메라 액터의 회전값 그대로 사용
        FRotator NewRotation = Camera->GetActorRotation();

        SetActorLocation(NewLocation);
        SetActorRotation(NewRotation);
    }
}