#pragma once

#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"

class ATriggerActor : public AActor
{
	DECLARE_CLASS(ATriggerActor, AActor)
	REFLECT_START(ClassName)
		PROPERTY(TriggerExtent)
		PROPERTY(bTriggerOnce)
	REFLECT_END()

public:
	ATriggerActor();
	virtual ~ATriggerActor() override = default;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, bool bFromSweep, const FHitResult& SweepResult);

	void SetTriggerExtent(const FVector& InExtent)
	{
		TriggerExtent = InExtent;
		if (TriggerBox)
		{
			TriggerBox->SetBoxExtent(TriggerExtent);
		}
	}
	const FVector& GetTriggerExtent() const { return TriggerExtent; }

	void SetTriggerOnce(bool bOnce) { bTriggerOnce = bOnce; }
	bool IsTriggerOnce() const { return bTriggerOnce; }

	bool HasTriggered() const { return bTriggered; }
	void ResetTrigger() { bTriggered = false; }

	UBoxComponent* GetTriggerBox() const { return TriggerBox; }

protected:
	void ExecuteTrigger(AActor* OtherActor);

	UBoxComponent* TriggerBox = nullptr;
	FVector TriggerExtent = FVector(2.0f, 2.0f, 2.0f);
	bool bTriggerOnce = true;
	bool bTriggered = false;
};
