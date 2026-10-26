#pragma once

#include "Actor.h"
#include "Components/BillboardComponent.h"

class APlayerStart : public AActor
{
    DECLARE_CLASS(APlayerStart, AActor)
    REFLECT_START(APlayerStart)
        PROPERTY(StartTag)
        PROPERTY(StartIndex)
    REFLECT_END()

public:
    APlayerStart();
    virtual ~APlayerStart() override = default;

    UBillboardComponent* GetBillboardComponent() const { return BillboardComponent; }

    inline FVector GetSpawnLocation() const { return GetActorLocation(); }
    inline FRotator GetSpawnRotation() const { return GetActorRotation(); }

    int32 GetStartIndex() const { return StartIndex; }
    void SetStartIndex(int32 InIndex) { StartIndex = InIndex; }

private:
    UBillboardComponent* BillboardComponent = nullptr;

    FString StartTag = "Default";
    int32 StartIndex = 0;
};