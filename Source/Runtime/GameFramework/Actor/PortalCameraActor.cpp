#include "EnginePCH.h"
#include "PortalCameraActor.h"
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

const FVector APortalCameraActor::ArmRelativeOffset = FVector(0.6649f, -0.0061f, 0.1933f);
const FVector APortalCameraActor::HeadRelativeOffset = FVector(0.0057f, 0.0493f, -0.4901f);

APortalCameraActor::APortalCameraActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // 1. Base Mesh (벽면 마운트 베이스 - 루트 컴포넌트)
    BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>("BaseMesh");
    SetRootComponent(BaseMesh);

    // 2. Arm Mesh (좌우 회전/Pan 조인트 - Base에 부착)
    ArmMesh = CreateDefaultSubobject<UStaticMeshComponent>("ArmMesh");
    ArmMesh->SetupAttachment(BaseMesh);
    ArmMesh->SetRelativeLocation(ArmRelativeOffset);

    // 3. Head Mesh (상하 회전/Tilt 카메라 헤드 - Arm에 부착)
    HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>("HeadMesh");
    HeadMesh->SetupAttachment(ArmMesh);
    HeadMesh->SetRelativeLocation(HeadRelativeOffset);

    // 4. Collision Box (카메라 전체를 아우르는 외곽 충돌체)
    CollisionBox = CreateDefaultSubobject<UBoxComponent>("CollisionBox");
    CollisionBox->SetupAttachment(BaseMesh);
    CollisionBox->SetBoxExtent(FVector(0.55f, 0.25f, 0.45f));
    CollisionBox->SetRelativeLocation(FVector(0.50f, 0.0f, -0.05f));

    // 5. 기본 3D 메시 로드
    if (UStaticMesh* Mesh = UAssetManager::LoadObjStaticMesh("Assets/Models/PortalCamera/portal_camera_base.obj"))
    {
        BaseMesh->SetStaticMesh(Mesh);
    }
    if (UStaticMesh* Mesh = UAssetManager::LoadObjStaticMesh("Assets/Models/PortalCamera/portal_camera_arm.obj"))
    {
        ArmMesh->SetStaticMesh(Mesh);
    }
    if (UStaticMesh* Mesh = UAssetManager::LoadObjStaticMesh("Assets/Models/PortalCamera/portal_camera_head.obj"))
    {
        HeadMesh->SetStaticMesh(Mesh);
    }
}

void APortalCameraActor::BeginPlay()
{
    Super::BeginPlay();

    // 씬 로드 후 메시가 유실되었을 경우 재할당
    if (BaseMesh && !BaseMesh->GetStaticMesh())
    {
        BaseMesh->SetStaticMesh(UAssetManager::LoadObjStaticMesh("Assets/Models/PortalCamera/portal_camera_base.obj"));
    }
    if (ArmMesh && !ArmMesh->GetStaticMesh())
    {
        ArmMesh->SetStaticMesh(UAssetManager::LoadObjStaticMesh("Assets/Models/PortalCamera/portal_camera_arm.obj"));
    }
    if (HeadMesh && !HeadMesh->GetStaticMesh())
    {
        HeadMesh->SetStaticMesh(UAssetManager::LoadObjStaticMesh("Assets/Models/PortalCamera/portal_camera_head.obj"));
    }

    if (ArmMesh)
    {
        ArmMesh->SetRelativeLocation(ArmRelativeOffset);
    }
    if (HeadMesh)
    {
        HeadMesh->SetRelativeLocation(HeadRelativeOffset);
    }

    CurrentYaw = 0.0f;
    CurrentPitch = 10.0f;   // Pitch(+) = 하향
    TargetYaw = 0.0f;
    TargetPitch = 10.0f;
}

void APortalCameraActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    AActor* Target = bTrackPlayer ? FindTargetPlayer() : nullptr;

    if (Target)
    {
        // 1. 플레이어 위치를 바라보는 방향 계산
        FVector HeadWorldPos = GetHeadWorldLocation();
        FVector TargetPos = Target->GetActorLocation();
        // 플레이어 몸통 중심(약 Z+0.9m)을 조준
        TargetPos.Z += 0.8f;

        FVector Dir = (TargetPos - HeadWorldPos).Normalized();

        // 베이스 액터 로컬 좌표계로 변환
        FTransform ActorXform = GetActorTransform();
        FVector LocalDir = ActorXform.InverseTransformVector(Dir);

        // LocalDir: X=Forward, Y=Right, Z=Up
        // Yaw: 수평각 (Y / X)
        float DesiredYaw = FMath::RadiansToDegrees(std::atan2(LocalDir.Y, LocalDir.X));
        // Pitch: 수직각 (Z / 거리)
        // 주의: 이 엔진의 FRotator는 Pitch(+)가 아래를 향함 (Forward.Z = -sin(Pitch)).
        //       따라서 위쪽(LocalDir.Z > 0)을 보려면 음수 Pitch가 필요하므로 부호를 반전한다.
        float HorizDist = std::sqrt(LocalDir.X * LocalDir.X + LocalDir.Y * LocalDir.Y);
        float DesiredPitch = -FMath::RadiansToDegrees(std::atan2(LocalDir.Z, HorizDist));

        // 가동 범위 제한 (Clamping)
        TargetYaw = FMath::Clamp(DesiredYaw, -MaxYawAngle, MaxYawAngle);
        TargetPitch = FMath::Clamp(DesiredPitch, -MaxPitchAngle, MaxPitchAngle);

        // 보간 속도로 추적
        CurrentYaw = FInterpTo(CurrentYaw, TargetYaw, DeltaTime, TrackingSpeed);
        CurrentPitch = FInterpTo(CurrentPitch, TargetPitch, DeltaTime, TrackingSpeed);
    }
    else if (bAutoSweep)
    {
        // 2. 플레이어가 없을 때 부드러운 사인파 스위핑 (CCTV 탐색)
        SweepTimer += DeltaTime * SweepSpeed;
        TargetYaw = std::sin(SweepTimer) * SweepRange;
        TargetPitch = 12.0f; // 기본 하향 응시 (이 엔진은 Pitch(+)가 아래)

        CurrentYaw = FInterpTo(CurrentYaw, TargetYaw, DeltaTime, TrackingSpeed * 0.5f);
        CurrentPitch = FInterpTo(CurrentPitch, TargetPitch, DeltaTime, TrackingSpeed * 0.5f);
    }

    // Arm은 좌우(Yaw) 회전, Head는 상하(Pitch) 회전 적용
    if (ArmMesh)
    {
        ArmMesh->SetRelativeRotation(FRotator(0.0f, CurrentYaw, 0.0f));
    }
    if (HeadMesh)
    {
        HeadMesh->SetRelativeRotation(FRotator(CurrentPitch, 0.0f, 0.0f));
    }
}

void APortalCameraActor::SetTargetAim(float InYaw, float InPitch)
{
    TargetYaw = FMath::Clamp(InYaw, -MaxYawAngle, MaxYawAngle);
    TargetPitch = FMath::Clamp(InPitch, -MaxPitchAngle, MaxPitchAngle);
}

FVector APortalCameraActor::GetHeadWorldLocation() const
{
    if (HeadMesh)
    {
        return HeadMesh->GetWorldLocation();
    }
    return GetActorTransform().TransformPosition(ArmRelativeOffset + HeadRelativeOffset);
}

AActor* APortalCameraActor::FindTargetPlayer()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    APawn* PlayerPawn = World->GetPlayerPawn();
    if (!PlayerPawn)
    {
        return nullptr;
    }

    FVector HeadPos = GetHeadWorldLocation();
    FVector PawnPos = PlayerPawn->GetActorLocation();
    float DistSq = (PawnPos - HeadPos).SizeSquared();

    if (DistSq > DetectionRadius * DetectionRadius)
    {
        return nullptr;
    }

    // 카메라 전방과 플레이어 방향의 각도(내적) 검사: 시야각(약 140도 콘)
    FVector Forward = GetActorTransform().GetForward();
    FVector DirToPlayer = (PawnPos - HeadPos).Normalized();
    float Dot = Forward.Dot(DirToPlayer);

    // cos(70도) = ~0.342. 전방 140도 범위 내에 있을 때만 감지
    if (Dot < 0.2f)
    {
        return nullptr;
    }

    return PlayerPawn;
}
