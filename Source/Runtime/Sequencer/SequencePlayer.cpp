#include "EnginePCH.h"
#include "SequencePlayer.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "GameFramework/Actor.h"
#include <cmath>

FSequencePlayer::FSequencePlayer()
{
	Sequence = MakeShared<FLevelSequence>();
}

void FSequencePlayer::Play()
{
	bIsPlaying = true;
}

void FSequencePlayer::Pause()
{
	bIsPlaying = false;
}

void FSequencePlayer::Stop(UWorld* World)
{
	bIsPlaying = false;
	CurrentTime = 0.0f;
	Evaluate(World);
}

void FSequencePlayer::ScrubTo(float InTime, UWorld* World)
{
	if (!Sequence) return;

	CurrentTime = FMath::Clamp(InTime, 0.0f, Sequence->Duration);
	if (World)
		Evaluate(World);
}

void FSequencePlayer::Tick(float DeltaTime, UWorld* World)
{
	if (!bIsPlaying || !Sequence) return;
	CurrentTime += DeltaTime * PlayRate;
	if (CurrentTime >= Sequence->Duration)
	{
		if (bIsLooping)
		{
			CurrentTime = std::fmod(CurrentTime, Sequence->Duration);
		}
		else
		{
			CurrentTime = Sequence->Duration;
			bIsPlaying = false;
		}
	}
	Evaluate(World);
}

void FSequencePlayer::Evaluate(UWorld* World)
{
	if (!Sequence || !World) return;

	ULevel* Level = World->GetCurrentLevel() ? World->GetCurrentLevel() : World->GetPersistentLevel();
	if (!Level) return;

	const auto& Actors = Level->GetActors();

	for (int32 i = 0; i < Sequence->Tracks.size(); ++i)
	{
		const FSequencerTrack& Track = Sequence->Tracks[i];
		if (Track.ActorName.empty() || Track.Keys.IsEmpty()) continue;

		AActor* TargetActor = nullptr;
		for (AActor* Actor : Actors)
		{
			if (Actor && Actor->GetName() == Track.ActorName)
			{
				TargetActor = Actor;
				break;
			}
		}

		if (TargetActor)
		{
			FVector Location;
			FRotator Rotation;
			FVector Scale;
			if (Track.Evaluate(CurrentTime, Location, Rotation, Scale))
			{
				TargetActor->SetActorLocation(Location);
				TargetActor->SetActorRotation(Rotation);
				TargetActor->SetActorScale3D(Scale);
			}
		}
	}
}