#include "EnginePCH.h"
#include "StaticMeshActor.h"

#include "Components/PrimitiveComponent.h"
#include "Asset/AssetManager.h"

AStaticMeshActor::AStaticMeshActor()
{
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("UStaticMeshComponent");
	StaticMeshComponent->SetupAttachment(GetRootComponent());

	// 1. UCntLogger 객체의 멤버 함수 바인딩 (AddDynamic 테스트)
	OnPicked.AddDynamic(&CntLogger, &UCntLogger::OnActorPicked);
	OnPickedTooMuch.AddDynamic(&CntLogger, &UCntLogger::OnActorPickedTooMuch);

	// 2. 람다 함수 바인딩 (Add 멀티캐스트 테스트)
	OnPicked.Add([](AStaticMeshActor* Actor, int Count) {
		const FString ActorName = Actor ? Actor->GetName() : "None";
		HTR_LOG(Info, "[Lambda] {} picked! (current picked cnt : {})", ActorName, Count);
	});
}

void AStaticMeshActor::SetPrimitiveType(EPrimitiveType Type)
{

}

void AStaticMeshActor::BeginPlay()
{
	Super::BeginPlay();
}
