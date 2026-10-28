#pragma once

#include "SceneComponent.h"
#include "Audio/Backend/IAudioBackend.h"

class UAudioComponent : public USceneComponent
{
	DECLARE_CLASS(UAudioComponent, USceneComponent)

	REFLECT_START(UAudioComponent)
		PROPERTY(SoundPath)
		PROPERTY(Volume)
		PROPERTY(Pitch)
		PROPERTY(bLoop)
		PROPERTY(bIs3D)
		PROPERTY(MinDistance)
		PROPERTY(MaxDistance)
	REFLECT_END()

public:
	UAudioComponent();
	virtual ~UAudioComponent() override;

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime) override;
	virtual void OnUnregister() override;
	virtual void PostEditChangeProperty(const FProperty& Property) override;

	void Play();
	void Stop();
	void SetPaused(bool bPause);
	bool IsPlaying() const;

	void SetVolume(float InVolume);
	void SetPitch(float InPitch);

	void SetSoundPath(const FString& InPath) { SoundPath = InPath; }
	const FString& GetSoundPath() const { return SoundPath; }

	void SetIs3D(bool bIn3D) { bIs3D = bIn3D; }
	bool GetIs3D() const { return bIs3D; }

	void SetLoop(bool bInLoop) { bLoop = bInLoop; }
	bool IsLoop() const { return bLoop; }

private:
	int32 CurrentVoiceID = INVALID_VOICE_ID;
	
	FString SoundPath;
	float Volume = 1.0f;
	float Pitch = 1.0f;
	bool bLoop = false;
	bool bIs3D = true;
	float MinDistance = 1.0f;
	float MaxDistance = 50.0f;
};
