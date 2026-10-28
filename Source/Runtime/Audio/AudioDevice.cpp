#include "EnginePCH.h"
#include "AudioDevice.h"

#include "Backend/NullAudioBackend.h"
#include "Backend/FMOD/FMODCoreBackend.h"

bool FAudioDevice::Init(EAudioBackendType Type)
{
    SwitchBackend(Type);
    return true;
}

void FAudioDevice::Shutdown()
{
    if(Backend)
    {
        Backend->Shutdown();
        Backend.reset();
    }
}

void FAudioDevice::Update()
{
    if(Backend)
    {
        Backend->Update();
    }
}

void FAudioDevice::SwitchBackend(EAudioBackendType NewType)
{
    if(Backend)
    {
        Backend->Shutdown();
    }

    if(NewType == EAudioBackendType::FMOD)
    {
        Backend = MakeUnique<FFMODCoreBackend>();

        if(!Backend->Initialize())
        {
            Backend = MakeUnique<FNullAudioBackend>();
        }
    }
}
