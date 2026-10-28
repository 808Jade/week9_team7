#include "EnginePCH.h"
#include "PortalTurretActor.h"
#include "Asset/AssetManager.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/CombatConfig.h"
#include "Core/EngineLog.h"
#include <cmath>

namespace
{
    float FInterpTo(float Current, float Target, float DeltaTime, float InterpSpeed)
    {
        if (InterpSpeed <= 0.0f) return Target;
        const float Dist = Target - Current;
        if (std::abs(Dist) < 1.e-4f) return Target;
        return Current + Dist * (1.0f - std::exp(-InterpSpeed * DeltaTime));
    }
}

const FVector APortalTurretActor::InitialLeftWingLoc  = FVector(0.1414f, -0.3594f, 0.9334f);
const FVector APortalTurretActor::InitialRightWingLoc = FVector(0.1414f,  0.3594f, 0.9334f);
const FVector APortalTurretActor::InitialLeftGunsLoc  = FVector(0.3356f, -0.2210f, 0.9791f);
const FVector APortalTurretActor::InitialRightGunsLoc = FVector(0.3356f,  0.2210f, 0.9791f);
const FVector APortalTurretActor::EyeOffset           = FVector(0.2847f,  0.0000f, 0.9334f);

APortalTurretActor::APortalTurretActor()
{
    PrimaryActorTick.bCanEverTick = true;
    SetActorEnableCollision(true);

    // 1. Collision Box (터렛 전체 충돌체 - 루트 컴포넌트, 높이 1.74m 중심 Z=0)
    CollisionBox = CreateDefaultSubobject<UBoxComponent>("CollisionBox");
    CollisionBox->SetBoxExtent(FVector(0.45f, 0.35f, 0.87f));
    CollisionBox->SetCurrentCollisionResponse(ECollisionResponse::Block);
    SetRootComponent(CollisionBox);

    // 2. Body Mesh (본체 프레임, 다리, 눈, 안테나 - CollisionBox 바닥 Z=-0.87m에 정렬)
    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>("BodyMesh");
    BodyMesh->SetupAttachment(CollisionBox);
    BodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -0.87f));

    // 3. Left Wing Mesh (좌측 날개 커버 - 힌지 피벗 위치 부착)
    LeftWingMesh = CreateDefaultSubobject<UStaticMeshComponent>("LeftWingMesh");
    LeftWingMesh->SetupAttachment(BodyMesh);
    LeftWingMesh->SetRelativeLocation(InitialLeftWingLoc);

    // 4. Right Wing Mesh (우측 날개 커버 - 힌지 피벗 위치 부착)
    RightWingMesh = CreateDefaultSubobject<UStaticMeshComponent>("RightWingMesh");
    RightWingMesh->SetupAttachment(BodyMesh);
    RightWingMesh->SetRelativeLocation(InitialRightWingLoc);

    // 5. Left Guns Mesh (좌측 기관총 유닛)
    LeftGunsMesh = CreateDefaultSubobject<UStaticMeshComponent>("LeftGunsMesh");
    LeftGunsMesh->SetupAttachment(BodyMesh);
    LeftGunsMesh->SetRelativeLocation(InitialLeftGunsLoc);

    // 6. Right Guns Mesh (우측 기관총 유닛)
    RightGunsMesh = CreateDefaultSubobject<UStaticMeshComponent>("RightGunsMesh");
    RightGunsMesh->SetupAttachment(BodyMesh);
    RightGunsMesh->SetRelativeLocation(InitialRightGunsLoc);

    // 7. Lua Script Component (터렛 스크립트 연결)
    LuaScriptComponent = CreateDefaultSubobject<ULuaScriptComponent>("LuaScriptComponent");
    LuaScriptComponent->SetScriptPath("Scripts/PortalTurret.lua");

    // 8. 기본 3D 메시 로드
    if (UStaticMesh* Mesh = UAssetManager::LoadObjStaticMesh("Assets/Models/PortalTurret/portal_turret_body.obj"))
    {
        BodyMesh->SetStaticMesh(Mesh);
    }
    if (UStaticMesh* Mesh = UAssetManager::LoadObjStaticMesh("Assets/Models/PortalTurret/portal_turret_wing_left.obj"))
    {
        LeftWingMesh->SetStaticMesh(Mesh);
    }
    if (UStaticMesh* Mesh = UAssetManager::LoadObjStaticMesh("Assets/Models/PortalTurret/portal_turret_wing_right.obj"))
    {
        RightWingMesh->SetStaticMesh(Mesh);
    }
    if (UStaticMesh* Mesh = UAssetManager::LoadObjStaticMesh("Assets/Models/PortalTurret/portal_turret_guns_left.obj"))
    {
        LeftGunsMesh->SetStaticMesh(Mesh);
    }
    if (UStaticMesh* Mesh = UAssetManager::LoadObjStaticMesh("Assets/Models/PortalTurret/portal_turret_guns_right.obj"))
    {
        RightGunsMesh->SetStaticMesh(Mesh);
    }

    SetWingOpenAlpha(bStartDeployed ? 1.0f : 0.0f);
}

void APortalTurretActor::BeginPlay()
{
    Super::BeginPlay();

    // 씬 로드 후 메시 재할당 검증
    if (BodyMesh && !BodyMesh->GetStaticMesh())
    {
        BodyMesh->SetStaticMesh(UAssetManager::LoadObjStaticMesh("Assets/Models/PortalTurret/portal_turret_body.obj"));
    }
    if (LeftWingMesh && !LeftWingMesh->GetStaticMesh())
    {
        LeftWingMesh->SetStaticMesh(UAssetManager::LoadObjStaticMesh("Assets/Models/PortalTurret/portal_turret_wing_left.obj"));
    }
    if (RightWingMesh && !RightWingMesh->GetStaticMesh())
    {
        RightWingMesh->SetStaticMesh(UAssetManager::LoadObjStaticMesh("Assets/Models/PortalTurret/portal_turret_wing_right.obj"));
    }
    if (LeftGunsMesh && !LeftGunsMesh->GetStaticMesh())
    {
        LeftGunsMesh->SetStaticMesh(UAssetManager::LoadObjStaticMesh("Assets/Models/PortalTurret/portal_turret_guns_left.obj"));
    }
    if (RightGunsMesh && !RightGunsMesh->GetStaticMesh())
    {
        RightGunsMesh->SetStaticMesh(UAssetManager::LoadObjStaticMesh("Assets/Models/PortalTurret/portal_turret_guns_right.obj"));
    }

    // 기본 전개 상태 초기화
    WingOpenAlpha = bStartDeployed ? 1.0f : 0.0f;
    TargetAlpha = WingOpenAlpha;
    State = bStartDeployed ? ETurretState::Alert : ETurretState::Searching;

    SetWingOpenAlpha(WingOpenAlpha);
}

void APortalTurretActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    const FCombatConfig& Config = FCombatConfig::Get();

    // 1. 중력 적용 (공중에 떠 있거나 넉백 중일 때)
    if (!bIsGrounded || Velocity.Z > 0.0f)
    {
        Velocity.Z -= Config.Gravity * DeltaTime;
    }

    // 2. 넉백 이동 & 충돌 스위프 (벽/바닥 충돌 감지)
    const FVector DeltaMove = Velocity * DeltaTime;
    if (DeltaMove.SizeSquared() > 1e-8f && GetRootComponent())
    {
        FHitResult Hit;
        GetRootComponent()->MoveComponent(DeltaMove, GetActorRotation(), true, &Hit);

        if (Hit.bBlockingHit)
        {
            // 바닥 충돌 (아래로 떨어지다 바닥에 닿음)
            if (Hit.ImpactNormal.Z > 0.5f && Velocity.Z <= 0.0f)
            {
                bIsGrounded = true;
                Velocity.Z = 0.0f;
            }
            // 천장 충돌
            else if (Hit.ImpactNormal.Z < -0.5f && Velocity.Z > 0.0f)
            {
                Velocity.Z = 0.0f;
            }

            // 벽면 충돌 (수평 이동 중 벽에 부딪힘)
            if (Hit.ImpactNormal.Z <= 0.5f)
            {
                Velocity.X = 0.0f;
                Velocity.Y = 0.0f;
            }
        }
        else
        {
            bIsGrounded = false;
        }
    }

    // 지상 마찰 감속
    if (bIsGrounded)
    {
        Velocity.X *= FMath::Clamp(1.0f - Config.GroundFriction * DeltaTime, 0.0f, 1.0f);
        Velocity.Y *= FMath::Clamp(1.0f - Config.GroundFriction * DeltaTime, 0.0f, 1.0f);
    }

    // 스쿼시 크기 원래대로 복원 (BodyMesh 기준)
    if (BodyMesh)
    {
        FVector CurrentScale = BodyMesh->GetRelativeScale3D();
        const FVector TargetScale = FVector(1.0f, 1.0f, 1.0f);
        const float Alpha = FMath::Clamp(Config.SquashRecoverySpeed * DeltaTime, 0.0f, 1.0f);
        FVector NewScale = CurrentScale + (TargetScale - CurrentScale) * Alpha;
        BodyMesh->SetRelativeScale3D(NewScale);

        // 넉백과 스쿼시가 모두 끝나면 피격 플래그 리셋 (다시 맞을 수 있도록)
        if (bIsHit && bIsGrounded && Velocity.SizeSquared() < 0.01f && (CurrentScale - TargetScale).SizeSquared() < 0.001f)
        {
            BodyMesh->SetRelativeScale3D(TargetScale);
            Velocity = FVector::ZeroVector;
            bIsHit = false;
        }
    }

    CheckTippedState();

    if (State != ETurretState::Tipped)
    {
        if (bAutoDeployOnAlert)
        {
            UpdateDetection(DeltaTime);
        }
    }

    UpdateAnimation(DeltaTime);
}

void APortalTurretActor::OnHit(const FVector& Direction, const FVector& ImpactPoint)
{
    if (bIsHit) return;
    bIsHit = true;
    bIsGrounded = false;

    const FCombatConfig& Config = FCombatConfig::Get();

    // 1. 히트스탑
    if (GetWorld())
    {
        GetWorld()->SetTimeDilation(Config.HitStopDuration, Config.HitStopTimeDilation);
    }

    // 2. 히트 스쿼시
    ApplyHitSquash(Direction);

    // 3. 넉백 속도 계산
    FVector HorizDir = FVector(Direction.X, Direction.Y, 0.0f);
    if (HorizDir.SizeSquared() > 1e-4f)
    {
        HorizDir = HorizDir.Normalized();
    }
    else
    {
        HorizDir = GetActorQuat().GetForwardVector();
    }

    Velocity = (HorizDir * Config.KnockbackHorizontal) + FVector(0.0f, 0.0f, Config.KnockbackVertical);

/*    // 4. 피격 시 충격으로 살짝 회전 틸트 (넘어짐 유도)
    FRotator CurrentRot = GetActorRotation();
    CurrentRot.Pitch += 15.0f;
    SetActorRotation(CurrentRot);*/

    // 5. 루아 스크립트 컴포넌트 호출
    if (LuaScriptComponent && LuaScriptComponent->IsLoaded())
    {
        LuaScriptComponent->OnHit(nullptr);
    }
}

void APortalTurretActor::ApplyHitSquash(const FVector& HitDirection)
{
    if (!BodyMesh) return;
    FVector SquashScale = GetDirectionalSquashScale(HitDirection);
    BodyMesh->SetRelativeScale3D(SquashScale);
}

FVector APortalTurretActor::GetDirectionalSquashScale(const FVector& HitDirection) const
{
    const FCombatConfig& Config = FCombatConfig::Get();

    FVector HorizHitDir = FVector(HitDirection.X, HitDirection.Y, 0.0f);
    if (HorizHitDir.SizeSquared() > 1e-6f)
    {
        HorizHitDir = HorizHitDir.Normalized();
    }
    else
    {
        HorizHitDir = GetActorQuat().GetForwardVector();
    }

    const FQuat Quat = GetActorQuat();
    const FVector Forward = Quat.GetForwardVector();
    const FVector Right = Quat.GetRightVector();

    const float ForwardDot = std::abs(FVector::Dot(HorizHitDir, Forward));
    const float RightDot = std::abs(FVector::Dot(HorizHitDir, Right));

    const float TotalDot = ForwardDot + RightDot;
    const float ForwardWeight = (TotalDot > 1e-4f) ? (ForwardDot / TotalDot) : 1.0f;
    const float RightWeight = (TotalDot > 1e-4f) ? (RightDot / TotalDot) : 0.0f;

    const float MinScale = Config.SquashMinScale;
    const float MaxScale = Config.SquashMaxScale;
    const float UpScale = Config.SquashUpScale;

    const float ScaleX = MaxScale - (MaxScale - MinScale) * ForwardWeight;
    const float ScaleY = MaxScale - (MaxScale - MinScale) * RightWeight;
    const float ScaleZ = UpScale;

    return FVector(ScaleX, ScaleY, ScaleZ);
}

void APortalTurretActor::Deploy()
{
    if (State == ETurretState::Tipped) return;

    TargetAlpha = 1.0f;
    State = ETurretState::Deploying;
}

void APortalTurretActor::Retract()
{
    if (State == ETurretState::Tipped) return;

    TargetAlpha = 0.0f;
    State = ETurretState::Retracting;
}

void APortalTurretActor::ToggleDeploy()
{
    if (TargetAlpha > 0.5f)
    {
        Retract();
    }
    else
    {
        Deploy();
    }
}

void APortalTurretActor::SetWingOpenAlpha(float Alpha)
{
    WingOpenAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);

    const float ClosedWingY = 0.1930f;
    const float OpenWingY = 0.3594f;
    const float CurrentWingY = ClosedWingY + (OpenWingY - ClosedWingY) * WingOpenAlpha;

    if (LeftWingMesh)
    {
        LeftWingMesh->SetRelativeLocation(FVector(InitialLeftWingLoc.X, -CurrentWingY, InitialLeftWingLoc.Z));
        LeftWingMesh->SetRelativeRotation(FRotator(0.0f, -MaxWingOpenAngle * WingOpenAlpha, 0.0f));
    }
    if (RightWingMesh)
    {
        RightWingMesh->SetRelativeLocation(FVector(InitialRightWingLoc.X, CurrentWingY, InitialRightWingLoc.Z));
        RightWingMesh->SetRelativeRotation(FRotator(0.0f, MaxWingOpenAngle * WingOpenAlpha, 0.0f));
    }

    const float ClosedGunY = 0.1000f;
    const float OpenGunY = 0.2210f;
    const float CurrentGunY = ClosedGunY + (OpenGunY - ClosedGunY) * WingOpenAlpha;

    const float ClosedGunX = 0.1800f;
    const float OpenGunX = InitialLeftGunsLoc.X + GunsExtendDistance;
    const float CurrentGunX = ClosedGunX + (OpenGunX - ClosedGunX) * WingOpenAlpha;

    if (LeftGunsMesh)
    {
        LeftGunsMesh->SetRelativeLocation(FVector(CurrentGunX, -CurrentGunY, InitialLeftGunsLoc.Z));
    }
    if (RightGunsMesh)
    {
        RightGunsMesh->SetRelativeLocation(FVector(CurrentGunX, CurrentGunY, InitialRightGunsLoc.Z));
    }
}

void APortalTurretActor::UpdateAnimation(float DeltaTime)
{
    float CurrentSpeed = (TargetAlpha > WingOpenAlpha) ? DeploySpeed : RetractSpeed;

    if (std::abs(WingOpenAlpha - TargetAlpha) > 0.001f)
    {
        float NewAlpha = FInterpTo(WingOpenAlpha, TargetAlpha, DeltaTime, CurrentSpeed);
        SetWingOpenAlpha(NewAlpha);

        if (std::abs(WingOpenAlpha - TargetAlpha) <= 0.001f)
        {
            SetWingOpenAlpha(TargetAlpha);
            if (TargetAlpha >= 0.99f)
            {
                State = ETurretState::Alert;
            }
            else if (TargetAlpha <= 0.01f)
            {
                State = ETurretState::Searching;
            }
        }
    }
}

void APortalTurretActor::UpdateDetection(float DeltaTime)
{
    AActor* Target = FindTargetPlayer();
    if (Target)
    {
        if (State != ETurretState::Alert && State != ETurretState::Deploying)
        {
            Deploy();
        }
    }
    else
    {
        if (State != ETurretState::Searching && State != ETurretState::Retracting)
        {
            Retract();
        }
    }
}

void APortalTurretActor::CheckTippedState()
{
    // 액터의 상단 벡터(Up)의 Z성분이 0.55 미만이면 넘어진 것으로 판정
    FVector Up = GetActorTransform().GetUp();
    if (Up.Z < 0.55f && State != ETurretState::Tipped)
    {
        State = ETurretState::Tipped;
        TargetAlpha = 1.0f; // 넘어진 상태에서는 날개가 무력하게 열려 발버둥침
        HTR_LOG(Info, "[PortalTurret] Turret tipped over! Disabled.");
    }
}

AActor* APortalTurretActor::FindTargetPlayer()
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    APawn* PlayerPawn = World->GetPlayerPawn();
    if (!PlayerPawn) return nullptr;

    FVector EyePos = GetEyeWorldLocation();
    FVector PawnPos = PlayerPawn->GetActorLocation();
    float DistSq = (PawnPos - EyePos).SizeSquared();

    if (DistSq > DetectionRadius * DetectionRadius)
    {
        return nullptr;
    }

    // 전방 원뿔 각도(FOV) 검사
    FVector Forward = GetActorTransform().GetForward();
    FVector DirToPlayer = (PawnPos - EyePos).Normalized();
    float Dot = Forward.Dot(DirToPlayer);

    float MinDot = std::cos(FMath::DegreesToRadians(DetectionAngle));
    if (Dot < MinDot)
    {
        return nullptr;
    }

    return PlayerPawn;
}

FVector APortalTurretActor::GetEyeWorldLocation() const
{
    if (BodyMesh)
    {
        return BodyMesh->GetTransform().TransformPosition(EyeOffset);
    }
    return GetActorTransform().TransformPosition(EyeOffset);
}
