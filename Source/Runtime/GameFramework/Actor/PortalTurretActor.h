#pragma once
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"

enum class ETurretState : uint8
{
    Searching,   // 탐색 / 대기
    Deploying,   // 날개 전개 중
    Alert,       // 경계 / 사격 대기
    Retracting,  // 날개 수납 중
    Tipped       // 쓰러짐 (무력화)
};

class APortalTurretActor : public AActor
{
    DECLARE_CLASS(APortalTurretActor, AActor)

    REFLECT_START(APortalTurretActor)
        PROPERTY(bStartDeployed)
        PROPERTY(bAutoDeployOnAlert)
        PROPERTY(DeploySpeed)
        PROPERTY(MaxWingOpenAngle)
        PROPERTY(GunsExtendDistance)
        PROPERTY(DetectionRadius)
        PROPERTY(DetectionAngle)
    REFLECT_END()

public:
    APortalTurretActor();
    virtual ~APortalTurretActor() override = default;

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // 날개 전개 / 수납 제어
    void Deploy();
    void Retract();
    void ToggleDeploy();
    void SetWingOpenAlpha(float Alpha);

    bool IsDeployed() const { return WingOpenAlpha >= 0.99f; }
    bool IsTipped() const { return State == ETurretState::Tipped; }
    ETurretState GetTurretState() const { return State; }

    FVector GetEyeWorldLocation() const;

    UStaticMeshComponent* GetBodyMesh() const { return BodyMesh; }
    UStaticMeshComponent* GetLeftWingMesh() const { return LeftWingMesh; }
    UStaticMeshComponent* GetRightWingMesh() const { return RightWingMesh; }
    UStaticMeshComponent* GetLeftGunsMesh() const { return LeftGunsMesh; }
    UStaticMeshComponent* GetRightGunsMesh() const { return RightGunsMesh; }
    UBoxComponent* GetCollisionBox() const { return CollisionBox; }

protected:
    void UpdateAnimation(float DeltaTime);
    void UpdateDetection(float DeltaTime);
    void CheckTippedState();
    AActor* FindTargetPlayer();

protected:
    // [컴포넌트 구조]
    // BodyMesh (본체 프레임, 삼각 다리, 눈, 안테나 - 루트 컴포넌트)
    //  ├── LeftWingMesh (좌측 날개 커버)
    //  ├── RightWingMesh (우측 날개 커버)
    //  ├── LeftGunsMesh (좌측 기관총)
    //  └── RightGunsMesh (우측 기관총)
    UStaticMeshComponent* BodyMesh = nullptr;
    UStaticMeshComponent* LeftWingMesh = nullptr;
    UStaticMeshComponent* RightWingMesh = nullptr;
    UStaticMeshComponent* LeftGunsMesh = nullptr;
    UStaticMeshComponent* RightGunsMesh = nullptr;
    UBoxComponent* CollisionBox = nullptr;

    // [전개 및 애니메이션 파라미터]
    bool bStartDeployed = true;        // 스폰 시 처음부터 날개가 열려있는지 여부
    bool bAutoDeployOnAlert = true;    // 플레이어 감지 시 자동 전개 여부
    float DeploySpeed = 4.0f;          // 날개 전개/수납 속도
    float MaxWingOpenAngle = 25.0f;    // 날개 최대 전개 각도 (도)
    float GunsExtendDistance = 0.08f;  // 총기 전방 돌출 거리 (미터)

    float DetectionRadius = 20.0f;     // 플레이어 감지 반경 (미터)
    float DetectionAngle = 50.0f;      // 플레이어 감지 반각 (도)

    float WingOpenAlpha = 0.0f;        // 0.0 = 완전 접힘, 1.0 = 완전 전개
    float TargetAlpha = 0.0f;
    float AlertTimer = 0.0f;           // 플레이어를 놓친 후 날개 닫을 때까지의 지연 시간

    ETurretState State = ETurretState::Searching;

    // 컴포넌트 초기 상대 위치
    static const FVector InitialLeftWingLoc;
    static const FVector InitialRightWingLoc;
    static const FVector InitialLeftGunsLoc;
    static const FVector InitialRightGunsLoc;
    static const FVector EyeOffset;
};
