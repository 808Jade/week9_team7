#pragma once

#include "IAudioBackend.h"

class FNullAudioBackend : public IAudioBackend
{
public:
	virtual ~FNullAudioBackend() = default;

	virtual bool Initialize() { return true; }
	virtual void Shutdown() {}
	virtual void Update() {}

	virtual int32 LoadSound(const FString& FilePath) { return INVALID_SOUND_ID; }
	virtual int32 Play(int32 SoundID, bool bLoop, float Volume, float Pitch, bool bIs3D, const FVector& Position, float MinDistance, float MaxDistance) { return INVALID_VOICE_ID; }
	virtual void Stop(int32 VoiceID) {}
	virtual void StopAll() {}
	virtual void SetPaused(int32 VoiceID, bool bPaused) {}
	virtual bool IsPlaying(int32 VoiceID) { return false; }

	virtual void SetVolume(int32 VoiceId, float Volume) {}
	virtual void SetPitch(int32 VoiceID, float Pitch) {}
	virtual void Set3DPosition(int32 VoiceID, const FVector& Position) {}
	virtual void SetMinMaxDistance(int32 VoiceID, float MinDistance, float MaxDistance) {}
	virtual void SetListner(const FVector& Position, const FVector& Forward, const FVector& Up) {}
};