#include "EnginePCH.h"
#include "FMODCoreBackend.h"
#include "FModHelper.h"

bool FFMODCoreBackend::Initialize()
{
    FMOD::System_Create(&CoreSystem);

    if(FMODCheck(CoreSystem->init(MAX_VOICE_COUNT, FMOD_INIT_NORMAL, nullptr)))
    {
        return true;
    }

    return false;
}

void FFMODCoreBackend::Shutdown()
{
    StopAll();

    for(std::pair<int32, FMOD::Sound*> Sound : SoundMap)
    {
        if(Sound.second != nullptr)
        {
            Sound.second->release();
        }        
    }
    SoundMap.Empty();

    if(CoreSystem)
    {
        CoreSystem->close();
        CoreSystem->release();
        CoreSystem = nullptr;
    }
}

void FFMODCoreBackend::Update()
{
    if(CoreSystem)
    {
        CoreSystem->update();
    }

    TArray<int32> StoppedVoices;
    for(std::pair<int32, FMOD::Channel*> Voice : VoiceMap)
    {
        bool bIsPlaying = false;
        if(Voice.second)
        {
            Voice.second->isPlaying(&bIsPlaying);
        }

        if(!bIsPlaying)
        {
            StoppedVoices.Add(Voice.first);
        }
    }

    for(int32 VoiceID : StoppedVoices)
    {
        VoiceMap.Remove(VoiceID);
    }
}

int32 FFMODCoreBackend::LoadSound(const FString& FilePath)
{
    int32* ExistID = PathToSoundID.Find(FilePath);
    if(ExistID != nullptr)
    {
        return *ExistID;
    }

    FMOD::Sound* NewSound = nullptr;

    if(CoreSystem)
    {
        if(FMODCheck(CoreSystem->createSound(FilePath.c_str(), FMOD_DEFAULT | FMOD_3D, nullptr, &NewSound)))
        {
            int32 ID = NextSoundID++;

            PathToSoundID.Add(FilePath, ID);
            SoundMap.Add(ID, NewSound);

            return ID;
        }
    }

    return INVALID_SOUND_ID;
}

int32 FFMODCoreBackend::Play(int32 SoundID, bool bLoop, float Volume, float Pitch, bool bIs3D, const FVector& Position, float MinDistance, float MaxDistance)
{
    FMOD::Sound** ExistSound = SoundMap.Find(SoundID);
    if(ExistSound == nullptr)
    {
        return INVALID_SOUND_ID;
    }

    FMOD::Channel* NewChannel = nullptr;
    FMOD_RESULT Result = CoreSystem->playSound(*ExistSound, nullptr, true, &NewChannel);
    if(!FMODCheck(Result) || NewChannel == nullptr)
    {
        return INVALID_VOICE_ID;
    }

    FMOD_MODE LoopFlag = bLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
    FMOD_MODE DimensionFlag = bIs3D ? FMOD_3D | FMOD_3D_LINEARROLLOFF : FMOD_2D;
    FMOD_MODE Mode = LoopFlag | DimensionFlag;

    NewChannel->setMode(Mode);
    NewChannel->setVolume(Volume);
    NewChannel->setPitch(Pitch);

    if(bIs3D)
    {
        NewChannel->set3DMinMaxDistance(MinDistance, MaxDistance);

        FMOD_VECTOR Pos = ToFMODVector(Position);
        FMOD_VECTOR Vel = { 0,0,0 };
        NewChannel->set3DAttributes(&Pos, &Vel);
    }

    NewChannel->setPaused(false);    

    int32 VoiceID = NextVoiceID++;

    VoiceMap.Add(VoiceID, NewChannel);

    return VoiceID;
}

void FFMODCoreBackend::Stop(int32 VoiceID)
{
    FMOD::Channel* Voice = FindActiveVoice(VoiceID);
    if(Voice)
    {
        Voice->stop();
        VoiceMap.Remove(VoiceID);
    }
}

void FFMODCoreBackend::StopAll()
{
    VoiceMap.Empty();
}

void FFMODCoreBackend::SetPaused(int32 VoiceID, bool bPaused)
{
    FMOD::Channel* Voice = FindActiveVoice(VoiceID);
    if(Voice)
    {
        Voice->setPaused(bPaused);
    }
}

bool FFMODCoreBackend::IsPlaying(int32 VoiceID)
{
    bool bIsPlaying = false;

    FMOD::Channel* Voice = FindActiveVoice(VoiceID);
    if(Voice)
    {
        Voice->isPlaying(&bIsPlaying);
    }

    return bIsPlaying;
}

void FFMODCoreBackend::SetVolume(int32 VoiceID, float Volume)
{
    FMOD::Channel* Voice = FindActiveVoice(VoiceID);
    if(Voice)
    {
        Voice->setVolume(Volume);
    }
}

void FFMODCoreBackend::SetPitch(int32 VoiceID, float Pitch)
{
    FMOD::Channel* Voice = FindActiveVoice(VoiceID);
    if(Voice)
    {
        Voice->setPitch(Pitch);
    }
}

void FFMODCoreBackend::Set3DPosition(int32 VoiceID, const FVector& Position)
{
    FMOD::Channel* Voice = FindActiveVoice(VoiceID);
    if(Voice)
    {
        FMOD_VECTOR Pos = ToFMODVector(Position);
        FMOD_VECTOR Vel = { 0,0,0 };
        Voice->set3DAttributes(&Pos, &Vel);
    }
}

void FFMODCoreBackend::SetMinMaxDistance(int32 VoiceID, float MinDistance, float MaxDistance)
{
    FMOD::Channel* Voice = FindActiveVoice(VoiceID);
    if(Voice)
    {
        Voice->set3DMinMaxDistance(MinDistance, MaxDistance);
    }
}

void FFMODCoreBackend::SetListner(const FVector& Position, const FVector& Forward, const FVector& Up)
{
    FMOD_VECTOR Pos = ToFMODVector(Position);
    FMOD_VECTOR F = ToFMODVector(Forward.GetSafeNormal());
    FMOD_VECTOR U = ToFMODVector(Up.GetSafeNormal());
    FMOD_VECTOR Vel = { 0,0,0 };

    if(CoreSystem)
    {
        CoreSystem->set3DListenerAttributes(0, &Pos, &Vel, &F, &U);
    }
}

FMOD::Channel* FFMODCoreBackend::FindActiveVoice(int32 VoiceID) const
{
    if(VoiceID == INVALID_VOICE_ID)
    {
        return nullptr;
    }

    FMOD::Channel* const* Found = VoiceMap.Find(VoiceID);
    return Found ? *Found : nullptr;
}
