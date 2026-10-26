#include "EnginePCH.h"
#include "MapCubeActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Asset/AssetManager.h"

AMapCubeActor::AMapCubeActor()
{
	PrimaryActorTick.bCanEverTick = false; // 정적 맵 타일이므로 틱 불필요 (성능 최적화)
	SetActorEnableCollision(true);

	// 1. 루트 박스 콜라이더 (기본 1m x 1m x 1m 큐브, Extent = 0.5m)
	BoxComponent = CreateDefaultSubobject<UBoxComponent>("BoxComponent");
	SetRootComponent(BoxComponent);
	BoxComponent->SetBoxExtent(FVector(0.5f, 0.5f, 0.5f));
	BoxComponent->SetCurrentCollisionResponse(ECollisionResponse::Block);

	// 2. 큐브 3D 스태틱 메시 컴포넌트
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("StaticMeshComponent");
	StaticMeshComponent->SetupAttachment(BoxComponent);

	// 엔진 내장 1x1x1 큐브 메시 설정
	UStaticMesh* CubeMesh = UAssetManager::GetAssetByPath<UStaticMesh>("Cube");
	if (CubeMesh)
	{
		StaticMeshComponent->SetStaticMesh(CubeMesh);
	}
}

void AMapCubeActor::BeginPlay()
{
	Super::BeginPlay();
}

void AMapCubeActor::SetBoxExtent(const FVector& InExtent)
{
	if (BoxComponent)
	{
		BoxComponent->SetBoxExtent(InExtent);
	}
}
