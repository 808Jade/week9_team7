#include "EnginePCH.h"
#include "SequenceData.h"
#include <algorithm>

static float LerpAngle(float A, float B, float Alpha)
{
	float Delta = std::fmod(B - A, 360.0f);
	if (Delta > 180.0f)
	{
		Delta -= 360.0f;
	}
	else if (Delta < -180.0f)
	{
		Delta += 360.0f;
	}
	return A + Delta * Alpha;
}

static FRotator LerpRotator(const FRotator& A, const FRotator& B, float Alpha)
{
	return FRotator(
		LerpAngle(A.Pitch, B.Pitch, Alpha),
		LerpAngle(A.Yaw, B.Yaw, Alpha),
		LerpAngle(A.Roll, B.Roll, Alpha)
	);
}

bool FSequencerTrack::Evaluate(float Time, FVector& OutLoc, FRotator& OutRot, FVector& OutScale) const
{
    if (Keys.IsEmpty()) return false;
    if (Keys.size() == 1 || Time <= Keys.Front().Time)
    {
        OutLoc = Keys.Front().Location;
        OutRot = Keys.Front().Rotation;
        OutScale = Keys.Front().Scale;
        return true;
    }
    if (Time >= Keys.Last().Time)
    {
        OutLoc = Keys.Last().Location;
        OutRot = Keys.Last().Rotation;
        OutScale = Keys.Last().Scale;
        return true;
    }

    for (size_t i = 0; i < Keys.size() - 1; ++i)
    {
        const auto& K0 = Keys[i];
        const auto& K1 = Keys[i + 1];
        if (Time >= K0.Time && Time <= K1.Time)
        {
            const float Duration = K1.Time - K0.Time;
            const float Alpha = (Duration > 1e-5f) ? (Time - K0.Time) / Duration : 0.0f;

            OutLoc = K0.Location + (K1.Location - K0.Location) * Alpha;
            OutScale = K0.Scale + (K1.Scale - K0.Scale) * Alpha;
            OutRot = LerpRotator(K0.Rotation, K1.Rotation, Alpha);
            return true;
        }
    }
    return false;
}

void FSequencerTrack::SortKeys()
{
	std::sort(Keys.begin(), Keys.end());
}

void FSequencerTrack::AddOrUpdateKey(float Time, const FVector& Location, const FRotator& Rotation, const FVector& Scale)
{
	for (int32 i = 0; i < Keys.Num(); ++i)
	{
		if (std::abs(Keys[i].Time - Time) < 0.02f)
		{
			Keys[i].Time = Time;
			Keys[i].Location = Location;
			Keys[i].Rotation = Rotation;
			Keys[i].Scale = Scale;
			return;
		}
	}

	Keys.Add({ Time, Location, Rotation, Scale });
	SortKeys();
}

bool FSequencerTrack::RemoveKey(int32 Index)
{
	if (Index < 0 || Index >= Keys.Num()) return false;
	Keys.RemoveAt(static_cast<uint32>(Index));
	return true;
}

FSequencerTrack* FLevelSequence::FindTrack(const FString& InActorName)
{
	for (int32 i = 0; i < Tracks.Num(); ++i)
	{
		if (Tracks[i].ActorName == InActorName)
			return &Tracks[i];
	}
	return nullptr;
}

FSequencerTrack* FLevelSequence::AddTrack(const FString& InActorName)
{
	if (FSequencerTrack* ExistingTrack = FindTrack(InActorName))
		return ExistingTrack;
	Tracks.Add({ InActorName, { } });
	return &Tracks.Last();
}

bool FLevelSequence::RemoveTrack(const FString& InActorName)
{
	for (int32 i = 0; i < Tracks.Num(); ++i)
	{
		if (Tracks[i].ActorName == InActorName)
		{
			Tracks.RemoveAt(static_cast<uint32>(i));
			return true;
		}
	}
	return false;
}