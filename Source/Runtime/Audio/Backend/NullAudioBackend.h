#pragma once

#include "IAudioBackend.h"

class FNullAudioBackend : public IAudioBackend
{
public:
	virtual ~FNullAudioBackend() = default;

	virtual bool Initialize() override { return true; }
	virtual void Shutdown() override {}
	virtual void Update() override {}

	virtual int32 LoadSound(const FString& FilePath) override { return INVALID_SOUND_ID; }
	virtual int32 Play(int32 SoundID, bool bLoop, float Volume, float Pitch, bool bIs3D, const FVector& Position, float MinDistance, float MaxDistance) override { return INVALID_VOICE_ID; }
	virtual void Stop(int32 VoiceID) override {}
	virtual void StopAll() override {}
	virtual void SetPaused(int32 VoiceID, bool bPaused) override {}
	virtual bool IsPlaying(int32 VoiceID) override { return false; }

	virtual void SetVolume(int32 VoiceID, float Volume) override {}
	virtual void SetPitch(int32 VoiceID, float Pitch) override {}
	virtual void Set3DPosition(int32 VoiceID, const FVector& Position) override {}
	virtual void SetMinMaxDistance(int32 VoiceID, float MinDistance, float MaxDistance) override {}
	virtual void SetListner(const FVector& Position, const FVector& Forward, const FVector& Up) override {}
};