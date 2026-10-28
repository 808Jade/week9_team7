#include "EnginePCH.h"
#include "SequenceData.h"
#include <algorithm>
#include <fstream>
#include <filesystem>

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

bool FSequencerTrack::EvaluateTransform(float Time, FVector& OutLoc, FRotator& OutRot, FVector& OutScale) const
{
    if (Keys.IsEmpty()) return false;
    if (Keys.Num() == 1 || Time <= Keys.Front().Time)
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

    for (int32 i = 0; i < Keys.Num() - 1; ++i)
    {
        const FSequencerKey& K0 = Keys[i];
        const FSequencerKey& K1 = Keys[i + 1];
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

bool FSequencerTrack::EvaluateFloat(float Time, float& OutValue) const
{
	if (Keys.IsEmpty()) return false;
	if (Keys.Num() == 1 || Time <= Keys.Front().Time)
	{
		OutValue = Keys.Front().Value;
		return true;
	}
	if (Time >= Keys.Last().Time)
	{
		OutValue = Keys.Last().Value;
		return true;
	}
	for (int32 i = 0; i < Keys.Num() - 1; ++i)
	{
		const FSequencerKey& K0 = Keys[i];
		const FSequencerKey& K1 = Keys[i + 1];
		if (Time >= K0.Time && Time <= K1.Time)
		{
			const float Duration = K1.Time - K0.Time;
			const float Alpha = (Duration > 1e-5f) ? (Time - K0.Time) / Duration : 0.0f;
			OutValue = K0.Value + (K1.Value - K0.Value) * Alpha;
			return true;
		}
	}
	return false;
}

void FSequencerTrack::SortKeys()
{
	std::sort(Keys.begin(), Keys.end());
}

void FSequencerTrack::AddOrUpdateTransformKey(float Time, const FVector& Location, const FRotator& Rotation, const FVector& Scale)
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

void FSequencerTrack::AddOrUpdateFloatKey(float Time, float Value)
{
	for (int32 i = 0; i < Keys.Num(); ++i)
	{
		if (std::abs(Keys[i].Time - Time) < 0.02f)
		{
			Keys[i].Time = Time;
			Keys[i].Value = Value;
			return;
		}
	}

	Keys.Add({ Time, FVector::ZeroVector, FRotator::ZeroRotator, FVector(1.0f, 1.0f, 1.0f), Value });
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

FSequencerTrack* FLevelSequence::AddTrack(const FString& InActorName, ETrackType InType)
{
	if (FSequencerTrack* ExistingTrack = FindTrack(InActorName))
		return ExistingTrack;
	FSequencerTrack NewTrack;
	NewTrack.ActorName = InActorName;
	NewTrack.Type = InType;
	Tracks.Add(NewTrack);
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

bool FLevelSequence::SaveToFile(const FString& FilePath) const
{
	try
	{
		std::filesystem::path Path(FilePath);
		if (Path.has_parent_path())
		{
			std::filesystem::create_directories(Path.parent_path());
		}

		json Root;
		Root["SequenceName"] = Name;
		Root["Duration"] = Duration;
		Root["FrameRate"] = FrameRate;
		Root["CameraCutTarget"] = CameraCutTarget;
		Root["CameraBlendTime"] = CameraBlendTime;

		json TracksArray = json::array();
		for (int32 i = 0; i < Tracks.Num(); ++i)
		{
			const auto& Track = Tracks[i];
			json TrackJson;
			TrackJson["ActorName"] = Track.ActorName;

			std::string TypeStr = "Transform";
			switch (Track.Type)
			{
			case ETrackType::Transform: TypeStr = "Transform"; break;
			case ETrackType::Fade: TypeStr = "Fade"; break;
			case ETrackType::LetterBox: TypeStr = "LetterBox"; break;
			case ETrackType::CameraShake: TypeStr = "CameraShake"; break;
			default: TypeStr = "None"; break;
			}
			TrackJson["TrackType"] = TypeStr;

			json KeysArray = json::array();
			for (int32 k = 0; k < Track.Keys.Num(); ++k)
			{
				const auto& Key = Track.Keys[k];
				json KeyJson;
				KeyJson["Time"] = Key.Time;
				KeyJson["Location"] = { Key.Location.X, Key.Location.Y, Key.Location.Z };
				KeyJson["Rotation"] = { Key.Rotation.Pitch, Key.Rotation.Yaw, Key.Rotation.Roll };
				KeyJson["Scale"] = { Key.Scale.X, Key.Scale.Y, Key.Scale.Z };
				KeyJson["Value"] = Key.Value;
				KeyJson["Duration"] = Key.Duration;
				KeysArray.push_back(KeyJson);
			}
			TrackJson["Keys"] = KeysArray;
			TracksArray.push_back(TrackJson);
		}
		Root["Tracks"] = TracksArray;

		std::ofstream File(FilePath);
		if (!File.is_open()) return false;
		File << Root.dump(4);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool FLevelSequence::LoadFromFile(const FString& FilePath)
{
	try
	{
		std::ifstream File(FilePath);
		if (!File.is_open()) return false;

		json Root;
		File >> Root;

		Name = Root.value("SequenceName", Root.value("Name", "NewSequence"));
		Duration = Root.value("Duration", 10.0f);
		FrameRate = Root.value("FrameRate", 30.0f);
		CameraCutTarget = Root.value("CameraCutTarget", "");
		CameraBlendTime = Root.value("CameraBlendTime", 0.0f);

		Tracks.Empty();

		if (Root.contains("Tracks") && Root["Tracks"].is_array())
		{
			for (const auto& TrackJson : Root["Tracks"])
			{
				FSequencerTrack Track;
				Track.ActorName = TrackJson.value("ActorName", "");

				std::string TypeStr = TrackJson.value("TrackType", "Transform");
				if (TypeStr == "Fade") Track.Type = ETrackType::Fade;
				else if (TypeStr == "LetterBox") Track.Type = ETrackType::LetterBox;
				else if (TypeStr == "CameraShake") Track.Type = ETrackType::CameraShake;
				else Track.Type = ETrackType::Transform;

				if (TrackJson.contains("Keys") && TrackJson["Keys"].is_array())
				{
					for (const auto& KeyJson : TrackJson["Keys"])
					{
						FSequencerKey Key;
						Key.Time = KeyJson.value("Time", 0.0f);
						Key.Value = KeyJson.value("Value", 0.0f);
						Key.Duration = KeyJson.value("Duration", 0.5f);

						if (KeyJson.contains("Location") && KeyJson["Location"].is_array() && KeyJson["Location"].size() >= 3)
						{
							Key.Location = FVector(KeyJson["Location"][0], KeyJson["Location"][1], KeyJson["Location"][2]);
						}
						if (KeyJson.contains("Rotation") && KeyJson["Rotation"].is_array() && KeyJson["Rotation"].size() >= 3)
						{
							Key.Rotation = FRotator(KeyJson["Rotation"][0], KeyJson["Rotation"][1], KeyJson["Rotation"][2]);
						}
						if (KeyJson.contains("Scale") && KeyJson["Scale"].is_array() && KeyJson["Scale"].size() >= 3)
						{
							Key.Scale = FVector(KeyJson["Scale"][0], KeyJson["Scale"][1], KeyJson["Scale"][2]);
						}
						Track.Keys.Add(Key);
					}
					Track.SortKeys();
				}
				Tracks.Add(Track);
			}
		}
		return true;
	}
	catch (...)
	{
		return false;
	}
}