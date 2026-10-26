#pragma once
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"

class AElevatorTriggerActor : public AActor
{
	DECLARE_CLASS(AElevatorTriggerActor, AActor)
	REFLECT_START(ClassName)
		PROPERTY(TriggerExtent)
	REFLECT_END()

public:
	AElevatorTriggerActor();
	virtual ~AElevatorTriggerActor() override = default;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, bool bFromSweep, const FHitResult& SweepResult);

	void TriggerEnding();

	void SetTriggerExtent(const FVector& InExtent)
	{
		TriggerExtent = InExtent;
		if (TriggerBox)
		{
			TriggerBox->SetBoxExtent(TriggerExtent);
		}
	}
	const FVector& GetTriggerExtent() const { return TriggerExtent; }

protected:
	UBoxComponent* TriggerBox = nullptr;
	FVector TriggerExtent = FVector(2.5f, 2.5f, 3.0f);
	bool bTriggered = false;
};
