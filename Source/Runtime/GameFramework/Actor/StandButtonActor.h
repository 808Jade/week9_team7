#pragma once
#include "ButtonActorBase.h"

class AStandButtonActor : public AButtonActorBase
{
    DECLARE_CLASS(AStandButtonActor, AButtonActorBase)
    REFLECT_START(AStandButtonActor)
        PROPERTY(ResetDelay)
    REFLECT_END()

public:
    AStandButtonActor();
    virtual ~AStandButtonActor() override = default;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // 플레이어가 조준하고 E키 등으로 상호작용할 때 호출할 함수
    virtual void Interact(AActor* Interactor);

    virtual void PressButton() override;
    virtual void ReleaseButton() override;

protected:
    float ResetDelay = 3.0f;
    float CurrentResetTimer = 0.0f;
};
