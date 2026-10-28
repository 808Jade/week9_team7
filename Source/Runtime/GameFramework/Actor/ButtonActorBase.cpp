#include "EnginePCH.h"
#include "ButtonActorBase.h"
#include "Audio/AudioDevice.h"
#include "Audio/Sound/SoundWave.h"
#include "Asset/AssetManager.h"

AButtonActorBase::AButtonActorBase()
{ 
    BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>("BaseMesh");
    ButtonMesh = CreateDefaultSubobject<UStaticMeshComponent>("ButtonMesh");
    TriggerBox = CreateDefaultSubobject<UBoxComponent>("TriggerBox");
    BaseCollisionBox = CreateDefaultSubobject<UBoxComponent>("BaseCollisionBox");

    BaseMesh->SetupAttachment(GetRootComponent());
    ButtonMesh->SetupAttachment(GetRootComponent());
    TriggerBox->SetupAttachment(GetRootComponent());
    BaseCollisionBox->SetupAttachment(GetRootComponent());  


    TriggerBox->SetCurrentCollisionResponse(ECollisionResponse::Overlap);

    BaseCollisionBox->SetCurrentCollisionResponse(ECollisionResponse::Block);

    PrimaryActorTick.bCanEverTick = true;
}

void AButtonActorBase::BeginPlay()
{
    Super::BeginPlay();

    if (TriggerBox)
    {
        TriggerBox->SetCurrentCollisionResponse(ECollisionResponse::Overlap);
    }
    if (BaseCollisionBox)
    {
        BaseCollisionBox->SetCurrentCollisionResponse(ECollisionResponse::Block);
    }

    // 시작 시점의 버튼 메쉬 원래 로컬 위치 저장
    if (ButtonMesh)
    {
        InitialButtonLocation = ButtonMesh->GetRelativeLocation();
    }
}

void AButtonActorBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!ButtonMesh) return;

    FVector CurrentLoc = ButtonMesh->GetRelativeLocation();

    // 목표 Z 높이: 눌렸으면 (원래높이 - PressDepth), 안 눌렸으면 원래높이
    float TargetZ = bIsPressed ? (InitialButtonLocation.Z - PressDepth) : InitialButtonLocation.Z;

    // 지수 감속 보간 (FInterpTo 방식: 목표 지점에 다다를수록 부드럽게 감속)
    float Alpha = std::clamp(InterpSpeed * DeltaTime, 0.0f, 1.0f);
    CurrentLoc.Z += (TargetZ - CurrentLoc.Z) * Alpha;

    // 거의 다 도달했으면 정확한 위치로 스냅
    if (std::abs(CurrentLoc.Z - TargetZ) < 0.001f)
    {
        CurrentLoc.Z = TargetZ;
    }

    // 계산된 보간 위치를 메시에 적용
    ButtonMesh->SetRelativeLocation(CurrentLoc);
}

void AButtonActorBase::PressButton()
{
    if (!bIsPressed)
    {
        bIsPressed = true;
        PlayButtonPressedSound();
        OnButtonPressed.Broadcast(true);
    }
}

void AButtonActorBase::PlayButtonPressedSound()
{
    IAudioBackend* AudioBackend = FAudioDevice::GetInstance().GetBackend();
    if (!AudioBackend)
    {
        return;
    }

    USoundWave* SoundWave = UAssetManager::GetAssetByPath<USoundWave>("Audio/button_pressed.mp3");
    if (SoundWave && SoundWave->GetSoundID() != INVALID_SOUND_ID)
    {
        AudioBackend->Play(SoundWave->GetSoundID(), false, 1.0f, 1.0f, true, GetActorLocation(), 3.0f, 60.0f);
    }
}

void AButtonActorBase::ReleaseButton()
{
    if (bIsPressed)
    {
        bIsPressed = false;
        OnButtonPressed.Broadcast(false);
    }
}
