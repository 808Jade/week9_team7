#include "EnginePCH.h"
#include "SequencePlayer.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "GameFramework/Actor.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

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

	if (!bHasCameraCutActive && World)
	{
		ApplyCameraCut(World);
	}

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
			RestoreCameraCut(World);
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
			if (Track.EvaluateTransform(CurrentTime, Location, Rotation, Scale))
			{
				TargetActor->SetActorLocation(Location);
				TargetActor->SetActorRotation(Rotation);
				TargetActor->SetActorScale3D(Scale);
			}
		}
	}
}

void FSequencePlayer::ApplyCameraCut(UWorld* World)
{
	if (!World || !Sequence || Sequence->CameraCutTarget.empty()) return;
	if (bHasCameraCutActive) return;

	APlayerController* PC = World->GetPlayerController();
	if (!PC) return;

	APlayerCameraManager* PCM = PC->GetPlayerCameraManager();
	if (!PCM) return;

	ULevel* Level = World->GetCurrentLevel() ? World->GetCurrentLevel() : World->GetPersistentLevel();
	if (!Level) return;

	AActor* TargetCameraActor = nullptr;
	for (AActor* Actor : Level->GetActors())
	{
		if (Actor && Actor->GetName() == Sequence->CameraCutTarget)
		{
			TargetCameraActor = Actor;
			break;
		}
	}

	if (TargetCameraActor)
	{
		OriginalViewTarget = PCM->GetViewTarget().TargetActor;

		PCM->SetViewTarget(TargetCameraActor);

		bHasCameraCutActive = true;
	}
}

void FSequencePlayer::RestoreCameraCut(UWorld* World)
{
	if (!bHasCameraCutActive) return;

	if (World)
	{
		if (APlayerController* PC = World->GetPlayerController())
		{
			if (APlayerCameraManager* PCM = PC->GetPlayerCameraManager())
			{
				// 백업해둔 원래 플레이어로 복구
				AActor* RestoreTarget = OriginalViewTarget ? OriginalViewTarget : Cast<AActor>(World->GetPlayerPawn());
				if (RestoreTarget)
				{
					PCM->SetViewTarget(RestoreTarget);
				}
			}
		}
	}

	OriginalViewTarget = nullptr;
	bHasCameraCutActive = false;
}