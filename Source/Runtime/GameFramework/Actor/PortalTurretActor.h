#pragma once
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/LuaScriptComponent.h"

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

    UBoxComponent* GetCollisionBox() const { return CollisionBox; }
    UStaticMeshComponent* GetBodyMesh() const { return BodyMesh; }
    UStaticMeshComponent* GetLeftWingMesh() const { return LeftWingMesh; }
    UStaticMeshComponent* GetRightWingMesh() const { return RightWingMesh; }
    UStaticMeshComponent* GetLeftGunsMesh() const { return LeftGunsMesh; }
    UStaticMeshComponent* GetRightGunsMesh() const { return RightGunsMesh; }
    ULuaScriptComponent* GetLuaScriptComponent() const { return LuaScriptComponent; }

    // 피격 및 타격 반응 (Hit Reaction)
    void OnHit(const FVector& Direction, const FVector& ImpactPoint);
    void ApplyHitSquash(const FVector& HitDirection);
    FVector GetDirectionalSquashScale(const FVector& HitDirection) const;

    void SetIsHit(bool bInIsHit) { bIsHit = bInIsHit; }
    bool GetIsHit() const { return bIsHit; }

protected:
    void UpdateAnimation(float DeltaTime);
    void UpdateDetection(float DeltaTime);
    void CheckTippedState();
    AActor* FindTargetPlayer();

protected:
    // [컴포넌트 구조]
    // CollisionBox (충돌 및 물리 스위프용 루트 컴포넌트)
    //  └── BodyMesh (본체 프레임, 삼각 다리, 눈, 안테나 - 스쿼시/스트레치 적용 대상)
    //       ├── LeftWingMesh (좌측 날개 커버)
    //       ├── RightWingMesh (우측 날개 커버)
    //       ├── LeftGunsMesh (좌측 기관총)
    //       └── RightGunsMesh (우측 기관총)
    // LuaScriptComponent (루아 스크립트 컴포넌트)
    UBoxComponent* CollisionBox = nullptr;
    UStaticMeshComponent* BodyMesh = nullptr;
    UStaticMeshComponent* LeftWingMesh = nullptr;
    UStaticMeshComponent* RightWingMesh = nullptr;
    UStaticMeshComponent* LeftGunsMesh = nullptr;
    UStaticMeshComponent* RightGunsMesh = nullptr;
    ULuaScriptComponent* LuaScriptComponent = nullptr;

    // [전개 및 애니메이션 파라미터]
    bool bStartDeployed = false;
    bool bAutoDeployOnAlert = true;
    float DeploySpeed = 12.0f;
    float RetractSpeed = 4.0f;
    float HoldOpenDuration = 2.0f;
    float MaxWingOpenAngle = 30.0f;
    float GunsExtendDistance = 0.10f;

    float DetectionRadius = 20.0f;
    float DetectionAngle = 50.0f;

    float WingOpenAlpha = 0.0f;
    float TargetAlpha = 0.0f;
    float AlertTimer = 0.0f;

    ETurretState State = ETurretState::Searching;

    // [피격 및 물리 파라미터]
    bool bIsHit = false;               // 피격 상태 플래그
    bool bIsGrounded = false;          // 바닥 착지 여부
    FVector Velocity = FVector::ZeroVector; // 넉백 속도

    // 컴포넌트 초기 상대 위치
    static const FVector InitialLeftWingLoc;
    static const FVector InitialRightWingLoc;
    static const FVector InitialLeftGunsLoc;
    static const FVector InitialRightGunsLoc;
    static const FVector EyeOffset;
};
