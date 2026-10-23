#pragma once

#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Delegate.h"

class AStaticMeshActor;

// Delegate 테스트용 로거 클래스
class UCntLogger
{
public:
	void OnActorPicked(AStaticMeshActor* Actor, int Count);
	void OnActorPickedTooMuch(AStaticMeshActor* Actor, int Count);
};

DECLARE_DELEGATE(FOnPicked, AStaticMeshActor*, int);
DECLARE_DELEGATE(FOnPickedTooMuch, AStaticMeshActor*, int);

class AStaticMeshActor : public AActor
{
	DECLARE_CLASS(AStaticMeshActor, AActor)

public:
	FOnPicked OnPicked;
	FOnPickedTooMuch OnPickedTooMuch;
	UCntLogger CntLogger;
	int pickedcnt = 0;

	void Picked()
	{
		pickedcnt++;
		OnPicked.Broadcast(this, pickedcnt);

		if (pickedcnt > 5)
		{
			OnPickedTooMuch.Broadcast(this, pickedcnt);
		}
	}

	AStaticMeshActor();
	virtual ~AStaticMeshActor() = default;

	virtual void BeginPlay() override;

	UStaticMeshComponent* GetStaticMeshComponent() const { return StaticMeshComponent; }
	void SetPrimitiveType(EPrimitiveType Type);

private:
	UStaticMeshComponent* StaticMeshComponent;
};

// AStaticMeshActor 정의 이후 구현 (Actor->GetName() 호출 가능)
inline void UCntLogger::OnActorPicked(AStaticMeshActor* Actor, int Count)
{
	const FString ActorName = Actor ? Actor->GetName() : "None";
	HTR_LOG(Info, "[UCntLogger] {} picked! (Current count: {})", ActorName, Count);
}

inline void UCntLogger::OnActorPickedTooMuch(AStaticMeshActor* Actor, int Count)
{
	const FString ActorName = Actor ? Actor->GetName() : "None";
	HTR_LOG(Warning, "[UCntLogger] Warning: {} picked too much! (Count: {})", ActorName, Count);
}
