#pragma once

#include "Backend/IAudioBackend.h"

class FAudioDevice
{
private:
	FAudioDevice() = default;
	~FAudioDevice() = default;

public:
	FAudioDevice(const FAudioDevice&) = delete;
	FAudioDevice& operator=(const FAudioDevice&) = delete;
	FAudioDevice(FAudioDevice&&) = delete;
	FAudioDevice& operator=(FAudioDevice&&) = delete;

	static FAudioDevice& GetInstance()
	{
		static FAudioDevice Instance;
		return Instance;
	}

	bool Init(EAudioBackendType Type);
	void Shutdown();
	void Update();

	void SwitchBackend(EAudioBackendType NewType);

	IAudioBackend* GetBackend() const { return Backend.get(); }

private:
	TUniquePtr<IAudioBackend> Backend;
	EAudioBackendType CurrentBackEndTpe = EAudioBackendType::Null;
};