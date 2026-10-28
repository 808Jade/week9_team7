#include "EnginePCH.h"
#include "SequencePlayer.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "GameFramework/Actor.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

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

	if (World)
	{
		if (APlayerController* PC = World->GetPlayerController())
		{
			if (APlayerCameraManager* PCM = PC->GetPlayerCameraManager())
			{
				PCM->SetManualFade(0.0f); // ?섏씠??珥덇린??
			}
		}
	}

	if (GEngine && World)
	{
		if (FWorldContext* Context = GEngine->GetWorldContextFromWorld(World))
		{
			if (Context->GameViewport)
			{
				Context->GameViewport->SetLetterBox(false, 0.0f);
			}
		}
	}
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
			Evaluate(World);
			RestoreCameraCut(World);
			return;
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
		if (Track.Keys.IsEmpty()) continue;

		if (Track.Type == ETrackType::Fade)
		{
			float FadeAlpha = 0.0f;
			if (Track.EvaluateFloat(CurrentTime, FadeAlpha))
			{
				if (APlayerController* PC = World->GetPlayerController())
				{
					if (APlayerCameraManager* PCM = PC->GetPlayerCameraManager())
					{
						PCM->SetManualFade(FadeAlpha);
					}
				}
			}
			continue;
		}

		if (Track.Type == ETrackType::Transform)
		{
			if (Track.ActorName.empty()) continue;

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

		if (Track.Type == ETrackType::LetterBox)
		{
			float LetterBoxVal = 0.0f;
			if (Track.EvaluateFloat(CurrentTime, LetterBoxVal))
			{
				if (GEngine)
				{
					if (FWorldContext* Context = GEngine->GetWorldContextFromWorld(World))
					{
						if (Context->GameViewport)
						{
							bool bEnable = (LetterBoxVal > 0.0f);
							// 매 프레임 호출하면 블렌드가 계속 리셋되어 띠가 거의 안 보이므로 상태가 바뀔 때만 호출
							if (bEnable != Context->GameViewport->IsLetterBoxEnabled())
							{
								Context->GameViewport->SetLetterBox(bEnable, 0.5f, 2.39f);
							}
						}
					}
				}
			}
			continue;
		}

		if (Track.Type == ETrackType::CameraShake)
		{
			float ShakeIntensity = 0.0f;
			if (Track.EvaluateFloat(CurrentTime, ShakeIntensity))
			{
				if (ShakeIntensity > 0.1f)
				{
					if (APlayerController* PC = World->GetPlayerController())
					{
						if (APlayerCameraManager* PCM = PC->GetPlayerCameraManager())
						{
							float ShakeScale = FMath::Clamp(ShakeIntensity, 0.0f, 1.0f);
							float ShakeDuration = 0.5f; // 湲곕낯 吏???쒓컙
							PCM->PlayCameraShake(ShakeScale, ShakeDuration);
						}
					}
				}
			}
			continue;
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
				// 諛깆뾽?대몦 ?먮옒 ?뚮젅?댁뼱濡?蹂듦뎄
				AActor* RestoreTarget = PC->GetPawn() ? (OriginalViewTarget ? OriginalViewTarget : Cast<AActor>(PC->GetPawn())) : nullptr;
				if (RestoreTarget)
				{
					PCM->SetViewTarget(RestoreTarget);
				}

				PCM->SetManualFade(0.0f); // ?섏씠??珥덇린??
			}
		}

		if (GEngine)
		{
			if (FWorldContext* Context = GEngine->GetWorldContextFromWorld(World))
			{
				if (Context->GameViewport)
				{
					Context->GameViewport->SetLetterBox(false, 0.5f);
				}
			}
		}


	}

	OriginalViewTarget = nullptr;
	bHasCameraCutActive = false;
}