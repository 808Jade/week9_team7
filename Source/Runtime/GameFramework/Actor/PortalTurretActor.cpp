#include "EnginePCH.h"
#include "PortalTurretActor.h"
#include "Asset/AssetManager.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
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

    // 1. Body Mesh (본체 프레임, 다리, 눈, 안테나 - 루트 컴포넌트)
    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>("BodyMesh");
    SetRootComponent(BodyMesh);

    // 2. Left Wing Mesh (좌측 날개 커버 - 힌지 피벗 위치 부착)
    LeftWingMesh = CreateDefaultSubobject<UStaticMeshComponent>("LeftWingMesh");
    LeftWingMesh->SetupAttachment(BodyMesh);
    LeftWingMesh->SetRelativeLocation(InitialLeftWingLoc);

    // 3. Right Wing Mesh (우측 날개 커버 - 힌지 피벗 위치 부착)
    RightWingMesh = CreateDefaultSubobject<UStaticMeshComponent>("RightWingMesh");
    RightWingMesh->SetupAttachment(BodyMesh);
    RightWingMesh->SetRelativeLocation(InitialRightWingLoc);

    // 4. Left Guns Mesh (좌측 기관총 유닛)
    LeftGunsMesh = CreateDefaultSubobject<UStaticMeshComponent>("LeftGunsMesh");
    LeftGunsMesh->SetupAttachment(BodyMesh);
    LeftGunsMesh->SetRelativeLocation(InitialLeftGunsLoc);

    // 5. Right Guns Mesh (우측 기관총 유닛)
    RightGunsMesh = CreateDefaultSubobject<UStaticMeshComponent>("RightGunsMesh");
    RightGunsMesh->SetupAttachment(BodyMesh);
    RightGunsMesh->SetRelativeLocation(InitialRightGunsLoc);

    // 6. Collision Box (터렛 전체 충돌체, 높이 1.74m 중심 Z=0.87m)
    CollisionBox = CreateDefaultSubobject<UBoxComponent>("CollisionBox");
    CollisionBox->SetupAttachment(BodyMesh);
    CollisionBox->SetBoxExtent(FVector(0.60f, 0.40f, 0.87f));
    CollisionBox->SetRelativeLocation(FVector(0.0f, 0.0f, 0.87f));

    // 7. 기본 3D 메시 로드
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
    return GetActorTransform().TransformPosition(EyeOffset);
}
