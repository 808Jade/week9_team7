#include "EnginePCH.h"
#include "ButtonActorBase.h"

AButtonActorBase::AButtonActorBase()
{ 
    BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>("BaseMesh");
    ButtonMesh = CreateDefaultSubobject<UStaticMeshComponent>("ButtonMesh");
    TriggerBox = CreateDefaultSubobject<UBoxComponent>("TriggerBox");

    BaseMesh->SetupAttachment(GetRootComponent());
    ButtonMesh->SetupAttachment(GetRootComponent());
    TriggerBox->SetupAttachment(GetRootComponent());

    TriggerBox->SetBoxExtent(FVector(50.0f, 50.0f, 20.0f));
    TriggerBox->SetRelativeLocation(FVector(0.0f, 0.0f, 10.0f));

    PrimaryActorTick.bCanEverTick = true;
}

void AButtonActorBase::BeginPlay()
{
    Super::BeginPlay();

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
    bIsPressed = true;
}

void AButtonActorBase::ReleaseButton()
{
    bIsPressed = false;
}
