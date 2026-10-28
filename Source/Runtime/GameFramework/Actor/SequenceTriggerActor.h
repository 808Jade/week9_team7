#pragma once

#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "Sequencer/SequencePlayer.h"

class ASequenceTriggerActor : public AActor
{
	DECLARE_CLASS(ASequenceTriggerActor, AActor)
	REFLECT_START(ClassName)
		PROPERTY(SequencePath)
		PROPERTY(TriggerExtent)
		PROPERTY(bAutoPlay)
		PROPERTY(bLoop)
		PROPERTY(bTriggerOnce)
		PROPERTY(bDisablePlayerInputOnPlay)
	REFLECT_END()

public:
	ASequenceTriggerActor();
	virtual ~ASequenceTriggerActor() override = default;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	void Play();
	void Pause();
	void Stop();

	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, bool bFromSweep, const FHitResult& SweepResult);

	void SetSequencePath(const FString& InPath) { SequencePath = InPath; }
	const FString& GetSequencePath() const { return SequencePath; }

	void SetTriggerExtent(const FVector& InExtent)
	{
		TriggerExtent = InExtent;
		if (TriggerBox)
		{
			TriggerBox->SetBoxExtent(TriggerExtent);
		}
	}
	const FVector& GetTriggerExtent() const { return TriggerExtent; }

	TSharedPtr<FSequencePlayer> GetSequencePlayer() const { return SequencePlayer; }

protected:
	UBoxComponent* TriggerBox = nullptr;
	TSharedPtr<FSequencePlayer> SequencePlayer;

	FString SequencePath = "Assets/Sequences/EndingCake.seq";
	FVector TriggerExtent = FVector(3.0f, 3.0f, 3.0f);
	bool bAutoPlay = false;
	bool bLoop = false;
	bool bTriggerOnce = true;
	bool bDisablePlayerInputOnPlay = false;

	bool bTriggered = false;
};
