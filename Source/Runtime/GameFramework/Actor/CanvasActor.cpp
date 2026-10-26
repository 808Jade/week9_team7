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

void ACanvasActor::SetCanvasVisibility(bool bVisible)
{
    bCanvasVisible = bVisible;
    // 캔버스 액터가 소유한 모든 컴포넌트들의 표시/숨김을 일괄 설정합니다.
    for (UActorComponent* Comp : Components)
    {
        if (USceneComponent* SceneComp = Cast<USceneComponent>(Comp))
        {
            SceneComp->SetVisibility(bVisible);
        }
    }
}