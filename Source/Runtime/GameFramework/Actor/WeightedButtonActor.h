#pragma once
#include "ButtonActorBase.h"

class AWeightedButtonActor : public AButtonActorBase
{
    DECLARE_CLASS(AWeightedButtonActor, AButtonActorBase)
    REFLECT_START(AWeightedButtonActor)
    REFLECT_END()

public:
    AWeightedButtonActor();
    virtual ~AWeightedButtonActor() override = default;
    virtual void BeginPlay() override;
    virtual void PressButton() override;
    virtual void ReleaseButton() override;

    int32 GetOverlappingActorCount() const { return OverlappingActors.Num(); }

protected:
    // TriggerBox 오버랩 콜백
    void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, bool bFromSweep, const FHitResult& SweepResult);
    void OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp);

    // 오버랩된 액터 수에 따라 버튼 눌림/해제 상태를 갱신
    void UpdateButtonState();

protected:
    // 현재 발판 위에 올라와 있는 액터 목록
    TArray<AActor*> OverlappingActors;
};
