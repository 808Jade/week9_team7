#include "EnginePCH.h"
#include "CubeDropActor.h"
#include "PortalCubeActor.h"
#include "Asset/AssetManager.h"
#include "Engine/World.h"
#include "Core/EngineLog.h"

ACubeDropActor::ACubeDropActor()
{
    PrimaryActorTick.bCanEverTick = true;

    DropperMesh = CreateDefaultSubobject<UStaticMeshComponent>("DropperMesh");
    SetRootComponent(DropperMesh);

    // 큐브(1m)와 포탈 도어(2m) 스케일에 맞춘 드로퍼 기본 크기 (너비 약 2m, 높이 약 2.2m)
    DropperMesh->SetRelativeScale3D(FVector(0.035f, 0.035f, 0.035f));

    SpawnPoint = CreateDefaultSubobject<USceneComponent>("SpawnPoint");
    SpawnPoint->SetupAttachment(GetRootComponent());
    SpawnPoint->SetRelativeLocation(SpawnOffset);

    // 기본 드로퍼 3D 메시 로드
    if (UStaticMesh* Mesh = UAssetManager::LoadObjStaticMesh("Assets/Models/CubeDropper/cube_dropper.obj"))
    {
        DropperMesh->SetStaticMesh(Mesh);
    }
}

void ACubeDropActor::BeginPlay()
{
    Super::BeginPlay();

    // 씬 로드 후 메시가 비어있다면 다시 할당
    if (DropperMesh && !DropperMesh->GetStaticMesh())
    {
        DropperMesh->SetStaticMesh(UAssetManager::LoadObjStaticMesh("Assets/Models/CubeDropper/cube_dropper.obj"));
    }

    if (SpawnPoint)
    {
        SpawnPoint->SetRelativeLocation(SpawnOffset);
    }

    // 시작 시 자동 드롭 옵션이 켜져 있으면 큐브 스폰
    if (bAutoDropOnBeginPlay)
    {
        DropCube();
    }
}

void ACubeDropActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 현재 큐브가 파괴되었는지 유효성 검사
    if (CurrentCube && (!CurrentCube->GetWorld()))
    {
        CurrentCube = nullptr;
    }
}

APortalCubeActor* ACubeDropActor::DropCube()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        HTR_LOG(Warning, "[CubeDropActor] DropCube failed: No World!");
        return nullptr;
    }

    // 이전 큐브 파괴 처리
    if (CurrentCube && bDestroyOldCubeOnDrop)
    {
        CurrentCube->Destroy();
        CurrentCube = nullptr;
    }

    // 스폰 위치 및 회전 계산 (SpawnPoint 컴포넌트 위치 또는 액터 트랜스폼 기준)
    FVector WorldSpawnLoc = SpawnPoint ? SpawnPoint->GetWorldLocation() : GetActorTransform().TransformPosition(SpawnOffset);
    FRotator WorldSpawnRot = GetActorRotation();
    FTransform SpawnTransform(WorldSpawnRot, WorldSpawnLoc, FVector::OneVector);

    CurrentCube = World->SpawnActor<APortalCubeActor>(NAME_None, &SpawnTransform);
    if (CurrentCube)
    {
        HTR_LOG(Info, "[CubeDropActor] Dropped PortalCube at ({:.2f}, {:.2f}, {:.2f})",
            WorldSpawnLoc.X, WorldSpawnLoc.Y, WorldSpawnLoc.Z);
    }

    return CurrentCube;
}

void ACubeDropActor::ResetCube()
{
    if (CurrentCube)
    {
        CurrentCube->Destroy();
        CurrentCube = nullptr;
        HTR_LOG(Info, "[CubeDropActor] ResetCube: Removed active cube");
    }
}

bool ACubeDropActor::HasActiveCube() const
{
    return CurrentCube != nullptr && CurrentCube->GetWorld() != nullptr;
}
