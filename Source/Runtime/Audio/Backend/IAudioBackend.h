#pragma once

#include "Core/Types.h"
#include "Core/EngineString.h"
#include "Math/Vector.h"

constexpr int32 INVALID_SOUND_ID = -1;
constexpr int32 INVALID_VOICE_ID = -1;

enum class EAudioBackendType
{
	Null,
	FMOD
};

class IAudioBackend
{
public:
	virtual ~IAudioBackend() = default;

	virtual bool Initialize() = 0;
	virtual void Shutdown() = 0;
	virtual void Update() = 0;

	virtual int32 LoadSound(const FString& FilePath) = 0;
	virtual int32 Play(int32 SoundID, bool bLoop, float Volume, float Pitch, bool bIs3D, const FVector& Position, float MinDistance, float MaxDistance) = 0;
	virtual void Stop(int32 VoiceID) = 0;
	virtual void StopAll() = 0;
	virtual void SetPaused(int32 VoiceID, bool bPaused) = 0;
	virtual bool IsPlaying(int32 VoiceID) = 0;

	virtual void SetVolume(int32 VoiceID, float Volume) = 0;
	virtual void SetPitch(int32 VoiceID, float Pitch) = 0;
	virtual void Set3DPosition(int32 VoiceID, const FVector& Position) = 0;
	virtual void SetMinMaxDistance(int32 VoiceID, float MinDistance, float MaxDistance) = 0;
	virtual void SetListner(const FVector& Position, const FVector& Forward, const FVector& Up) = 0;
};