#pragma once
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"

class APortalCameraActor : public AActor
{
    DECLARE_CLASS(APortalCameraActor, AActor)

    REFLECT_START(APortalCameraActor)
        PROPERTY(bTrackPlayer)
        PROPERTY(bAutoSweep)
        PROPERTY(TrackingSpeed)
        PROPERTY(DetectionRadius)
        PROPERTY(MaxYawAngle)
        PROPERTY(MaxPitchAngle)
        PROPERTY(SweepRange)
        PROPERTY(SweepSpeed)
    REFLECT_END()

public:
    APortalCameraActor();
    virtual ~APortalCameraActor() override = default;

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // 카메라 조준 수동 제어
    void SetTargetAim(float InYaw, float InPitch);
    void SetTrackingEnabled(bool bEnabled) { bTrackPlayer = bEnabled; }
    bool IsTrackingEnabled() const { return bTrackPlayer; }

    FVector GetHeadWorldLocation() const;

    UStaticMeshComponent* GetBaseMesh() const { return BaseMesh; }
    UStaticMeshComponent* GetArmMesh() const { return ArmMesh; }
    UStaticMeshComponent* GetHeadMesh() const { return HeadMesh; }
    UBoxComponent* GetCollisionBox() const { return CollisionBox; }

protected:
    void UpdateTracking(float DeltaTime);
    void UpdateSweep(float DeltaTime);
    AActor* FindTargetPlayer();

protected:
    // [컴포넌트 구조]
    // BaseMesh (벽면 마운트 베이스, 루트)
    //  └── ArmMesh (좌우 회전/Pan 조인트, Base에 부착)
    //       └── HeadMesh (상하 회전/Tilt 카메라 헤드, Arm에 부착)
    UStaticMeshComponent* BaseMesh = nullptr;
    UStaticMeshComponent* ArmMesh = nullptr;
    UStaticMeshComponent* HeadMesh = nullptr;
    UBoxComponent* CollisionBox = nullptr;

    // [회전/추적 파라미터]
    bool bTrackPlayer = true;          // 플레이어 감지 시 시선 추적 여부
    bool bAutoSweep = true;            // 플레이어가 없을 때 CCTV처럼 좌우 감시 스위핑 여부
    float TrackingSpeed = 4.0f;        // 추적 회전 속도
    float DetectionRadius = 15.0f;     // 플레이어 감지 반경 (미터)

    float MaxYawAngle = 65.0f;         // 좌우 최대 회전각 (도)
    float MaxPitchAngle = 45.0f;       // 상하 최대 회전각 (도)

    float SweepRange = 35.0f;          // 스위핑 회전 범위 (도)
    float SweepSpeed = 1.0f;           // 스위핑 주기 속도
    float SweepTimer = 0.0f;

    float CurrentYaw = 0.0f;
    float CurrentPitch = 0.0f;
    float TargetYaw = 0.0f;
    float TargetPitch = -10.0f;        // 기본으로 살짝 아래를 응시

    static const FVector ArmRelativeOffset;
    static const FVector HeadRelativeOffset;
};
