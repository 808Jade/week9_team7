#pragma once

#include "Audio/Backend/IAudioBackend.h"
#include "Containers/Map.h"

#define MAX_VOICE_COUNT 32

namespace FMOD
{
	class System;
	class Sound;
	class Channel;
}

class FFMODCoreBackend : public IAudioBackend
{
public:
	virtual bool Initialize() override;
	virtual void Shutdown() override;
	virtual void Update() override;

	virtual int32 LoadSound(const FString& FilePath) override;
	virtual int32 Play(int32 SoundID, bool bLoop, float Volume, float Pitch, bool bIs3D, const FVector& Position, float MinDistance, float MaxDistance) override;
	virtual void Stop(int32 VoiceID) override;
	virtual void StopAll() override;
	virtual void SetPaused(int32 VoiceID, bool bPaused) override;
	virtual bool IsPlaying(int32 VoiceID) override;

	virtual void SetVolume(int32 VoiceID, float Volume) override;
	virtual void SetPitch(int32 VoiceID, float Pitch) override;
	virtual void Set3DPosition(int32 VoiceID, const FVector& Position) override;
	virtual void SetMinMaxDistance(int32 VoiceID, float MinDistance, float MaxDistance) override;
	virtual void SetListner(const FVector& Position, const FVector& Forward, const FVector& Up) override;

private:
	FMOD::Channel* FindActiveVoice(int32 VoiceID) const;

	FMOD::System* CoreSystem = nullptr;

	TMap<FString, int32> PathToSoundID; // Mapping Sound Path
	TMap<int32, FMOD::Sound*> SoundMap;
	TMap<int32, FMOD::Channel*> VoiceMap;

	int32 NextSoundID = 1;
	int32 NextVoiceID = 1;
};