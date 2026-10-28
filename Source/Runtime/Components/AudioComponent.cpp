#include "EnginePCH.h"
#include "AudioComponent.h"

#include "Audio/AudioDevice.h"
#include "Audio/Sound/SoundWave.h"
#include "Asset/AssetManager.h"

UAudioComponent::UAudioComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bAutoActivate = true;
}

UAudioComponent::~UAudioComponent()
{
	Stop();
}

void UAudioComponent::BeginPlay()
{
	Super::BeginPlay();

	if(bAutoActivate)
	{
		Play();
	}
}

void UAudioComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);

	if(CurrentVoiceID != INVALID_VOICE_ID && bIs3D)
	{
		IAudioBackend* Backend = FAudioDevice::GetInstance().GetBackend();

		if(Backend && Backend->IsPlaying(CurrentVoiceID))
		{
			Backend->Set3DPosition(CurrentVoiceID, GetWorldLocation());
		}
		else
		{
			CurrentVoiceID = INVALID_VOICE_ID;
		}
	}
}

void UAudioComponent::OnUnregister()
{
	Super::OnUnregister();

	Stop();
}

void UAudioComponent::PostEditChangeProperty(const FProperty& Property)
{
	if(CurrentVoiceID != INVALID_VOICE_ID)
	{
		if(IAudioBackend* Backend = FAudioDevice::GetInstance().GetBackend())
		{
			Backend->SetVolume(CurrentVoiceID, Volume);
			Backend->SetPitch(CurrentVoiceID, Pitch);
			Backend->SetMinMaxDistance(CurrentVoiceID, MinDistance, MaxDistance);
		}
	}
}

void UAudioComponent::Play()
{
	IAudioBackend* Backend = FAudioDevice::GetInstance().GetBackend();
	if(!Backend)
	{
		return;
	}

	if(Backend->IsPlaying(CurrentVoiceID))
	{
		Stop();
	}

	USoundWave* Wave = UAssetManager::GetAssetByPath<USoundWave>(SoundPath);
	if(Wave && Wave->GetSoundID() != INVALID_SOUND_ID)
	{
		CurrentVoiceID = Backend->Play(Wave->GetSoundID(), bLoop, Volume, Pitch, bIs3D, GetWorldLocation(), MinDistance, MaxDistance);
	}
}

void UAudioComponent::Stop()
{
	IAudioBackend* Backend = FAudioDevice::GetInstance().GetBackend();
	if(Backend && CurrentVoiceID != INVALID_VOICE_ID && Backend->IsPlaying(CurrentVoiceID))
	{
		Backend->Stop(CurrentVoiceID);
		CurrentVoiceID = INVALID_VOICE_ID;
	}
}

void UAudioComponent::SetPaused(bool bPause)
{
	IAudioBackend* Backend = FAudioDevice::GetInstance().GetBackend();
	if(Backend && CurrentVoiceID != INVALID_VOICE_ID && Backend->IsPlaying(CurrentVoiceID))
	{
		Backend->SetPaused(CurrentVoiceID, bPause);
	}
}

bool UAudioComponent::IsPlaying() const
{
	IAudioBackend* Backend = FAudioDevice::GetInstance().GetBackend();
	if(Backend && CurrentVoiceID != INVALID_VOICE_ID && Backend->IsPlaying(CurrentVoiceID))
	{
		return true;
	}
	else
	{
		return false;
	}
}

void UAudioComponent::SetVolume(float InVolume)
{
	Volume = InVolume;
}

void UAudioComponent::SetPitch(float InPitch)
{
	Pitch = InPitch;
}
