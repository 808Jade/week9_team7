#pragma once

#include "Asset/RenderAsset.h"
#include "Audio/Backend/IAudioBackend.h"

class USoundWave : public URenderAsset
{
	DECLARE_CLASS(USoundWave, URenderAsset)

public:
	const int32 GetSoundID() const { return SoundID; }
	void SetSoundID(int32 InSoundID) { SoundID = InSoundID; }

private:
	int32 SoundID = INVALID_SOUND_ID;
};