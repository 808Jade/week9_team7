#include "EnginePCH.h"
#include "PointLightActor.h"
#include "GameFramework/Actor/StaticMeshActor.h"
#include "Engine/World.h"
#include "Engine/Level.h"

APointLightActor::APointLightActor()
{
    PointLightComponent = CreateDefaultSubobject<UPointLightComponent>("UPointLightComponent");
	SetupLightRoot(PointLightComponent, "Assets/Icons/Point.png");
}

APointLightActor::~APointLightActor()
{
	// 내가(조명이) 소멸할 때 남(ObservedMesh)의 델리게이트에서 나(this)를 반드시 해제!
	UnwatchMesh();
}

void APointLightActor::BeginPlay()
{
	Super::BeginPlay();

	// 게임 시작 시 씬에 StaticMeshActor가 있다면 첫 번째 것을 찾아 자동 구독
	if (UWorld* World = GetWorld())
	{
		if (ULevel* Level = World->GetPersistentLevel())
		{
			for (AActor* Actor : Level->GetActors())
			{
				if (AStaticMeshActor* Mesh = Cast<AStaticMeshActor>(Actor))
				{
					WatchMesh(Mesh);
					break;
				}
			}
		}
	}
}

void APointLightActor::WatchMesh(AStaticMeshActor* TargetMesh)
{
	if (ObservedMesh == TargetMesh) return;
	UnwatchMesh();

	ObservedMesh = TargetMesh;
	if (ObservedMesh)
	{
		// 남(TargetMesh)의 델리게이트에 나(this)를 바인딩!
		ObservedMesh->OnPicked.AddDynamic(this, &APointLightActor::OnTargetMeshPicked);
		HTR_LOG(Info, "[APointLightActor] Subscribed to '{}' OnPicked delegate! (AddDynamic)", ObservedMesh->GetName());
	}
}

void APointLightActor::UnwatchMesh()
{
	if (ObservedMesh)
	{
		// 남의 델리게이트에서 나(this)를 완전히 제거!
		int32_t RemovedCount = ObservedMesh->OnPicked.RemoveAll(this);
		HTR_LOG(Info, "[APointLightActor] Unsubscribed from '{}' OnPicked delegate! (RemoveAll - Removed: {})",
			ObservedMesh->GetName(), RemovedCount);
		ObservedMesh = nullptr;
	}
}

void APointLightActor::OnTargetMeshPicked(AStaticMeshActor* Actor, int Count)
{
	HTR_LOG(Info, "[APointLightActor] Detected pick on observed '{}'! (Count: {}) -> Reacting with light color/intensity change!",
		Actor ? Actor->GetName() : "None", Count);

	if (PointLightComponent)
	{
		// 피킹할 때마다 색상과 밝기를 순환 변경
		static const FColor Colors[] = {
			FColor(255, 80, 80, 255),   // 빨강
			FColor(80, 255, 80, 255),   // 초록
			FColor(80, 150, 255, 255),  // 파랑
			FColor(255, 220, 50, 255),  // 노랑
			FColor(255, 100, 255, 255), // 보라
		};
		PointLightComponent->SetLightColor(Colors[Count % 5]);
		PointLightComponent->SetIntensity(3.0f + (Count % 5) * 2.0f);
	}
}
