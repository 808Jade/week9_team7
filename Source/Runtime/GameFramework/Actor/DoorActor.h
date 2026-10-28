#pragma once
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"

class ADoorActor : public AActor
{
    DECLARE_CLASS(ADoorActor, AActor)

    REFLECT_START(ADoorActor)
        PROPERTY(bIsOpen)
        PROPERTY(OpenSpeed)
        PROPERTY(SlideDistance)
        PROPERTY(bAutoOpenOnOverlap)
    REFLECT_END()

public:
    ADoorActor();
    virtual ~ADoorActor() override = default;

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    void OpenDoor();
    void CloseDoor();
    void ToggleDoor();
    void SetOpen(bool bOpen);
    bool IsOpen() const { return bIsOpen; }

    void PlayDoorOpenSound();

    void SetAutoOpenOnOverlap(bool bAuto) { bAutoOpenOnOverlap = bAuto; }
    bool IsAutoOpenOnOverlap() const { return bAutoOpenOnOverlap; }

    UStaticMeshComponent* GetBaseMesh() const { return BaseMesh; }
    UStaticMeshComponent* GetLeftDoorMesh() const { return LeftDoorMesh; }
    UStaticMeshComponent* GetRightDoorMesh() const { return RightDoorMesh; }
    UBoxComponent* GetLeftDoorCollision() const { return LeftDoorCollision; }
    UBoxComponent* GetRightDoorCollision() const { return RightDoorCollision; }

    // 좌/우 문짝 메시의 AABB(최소외접육면체)를 계산하여 충돌 박스에 할당
    void UpdateCollisionBoxes();

protected:
    void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, bool bFromSweep, const FHitResult& SweepResult);
    void OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp);

protected:
    UStaticMeshComponent* BaseMesh = nullptr;           // 문틀 프레임
    UStaticMeshComponent* LeftDoorMesh = nullptr;       // 좌문
    UStaticMeshComponent* RightDoorMesh = nullptr;      // 우문
    UBoxComponent* LeftDoorCollision = nullptr;         // 좌문 물리 충돌 박스 (LeftDoorMesh에 부착)
    UBoxComponent* RightDoorCollision = nullptr;        // 우문 물리 충돌 박스 (RightDoorMesh에 부착)

    bool bIsOpen = false;
    float OpenSpeed = 5.0f;            // 문 개폐 보간 속도
    float SlideDistance = 0.5f;        // 문짝 슬라이딩 이동 거리 (Y축 기준)
    bool bAutoOpenOnOverlap = true;    // 플레이어 감지 시 자동 개폐 여부

    FVector InitialLeftDoorLoc = FVector::ZeroVector;
    FVector InitialRightDoorLoc = FVector::ZeroVector;

    int32 OverlappingActorCount = 0;
};
