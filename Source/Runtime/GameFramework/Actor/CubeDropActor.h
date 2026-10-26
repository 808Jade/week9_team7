#pragma once
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"

class APortalCubeActor;

class ACubeDropActor : public AActor
{
    DECLARE_CLASS(ACubeDropActor, AActor)

    REFLECT_START(ACubeDropActor)
        PROPERTY(bAutoDropOnBeginPlay)
        PROPERTY(bDestroyOldCubeOnDrop)
        PROPERTY(SpawnOffset)
    REFLECT_END()

public:
    ACubeDropActor();
    virtual ~ACubeDropActor() override = default;

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // 큐브 드롭 실행 (기존 큐브가 있으면 설정에 따라 제거 후 새로 스폰)
    APortalCubeActor* DropCube();

    // 현재 스폰된 큐브 제거 및 리셋
    void ResetCube();

    // 현재 활성화된 큐브 반환
    APortalCubeActor* GetCurrentCube() const { return CurrentCube; }
    bool HasActiveCube() const;

    UStaticMeshComponent* GetDropperMesh() const { return DropperMesh; }
    USceneComponent* GetSpawnPoint() const { return SpawnPoint; }

protected:
    UStaticMeshComponent* DropperMesh = nullptr;       // 큐브 드로퍼 외형 메시
    USceneComponent* SpawnPoint = nullptr;             // 큐브 스폰 포인트

    bool bAutoDropOnBeginPlay = true;                  // 씬 시작 시 자동으로 큐브 1개 드롭 여부
    bool bDestroyOldCubeOnDrop = true;                 // 재호출 시 기존 큐브 파괴 여부
    FVector SpawnOffset = FVector(0.0f, 0.0f, -2.0f);  // 드로퍼 하단 출구 스폰 오프셋

    APortalCubeActor* CurrentCube = nullptr;           // 현재 드롭되어 있는 큐브 참조
};
