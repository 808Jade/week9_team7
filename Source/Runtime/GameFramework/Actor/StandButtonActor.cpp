#include "EnginePCH.h"
#include "StandButtonActor.h"
#include "Asset/AssetManager.h"

AStandButtonActor::AStandButtonActor()
{
    if (UStaticMesh* Base = UAssetManager::LoadObjStaticMesh("Assets/Models/Pedestal Button/pedestal_button_base.obj"))
    {
        BaseMesh->SetStaticMesh(Base);
    }
    if (UStaticMesh* Button = UAssetManager::LoadObjStaticMesh("Assets/Models/Pedestal Button/pedestal_button_switch.obj"))
    {
        ButtonMesh->SetStaticMesh(Button);
    }

    // 버튼 메시의 최소 외접 큐브(AABB) 계산 및 TriggerBox 설정
    FBox LocalBounds = ButtonMesh ? ButtonMesh->CalcLocalBounds() : FBox();
    FVector Extent = LocalBounds.GetExtent();

    if (Extent.X > 0.001f && Extent.Y > 0.001f && Extent.Z > 0.001f)
    {
        TriggerBox->SetBoxExtent(Extent);
        TriggerBox->SetRelativeLocation(LocalBounds.GetCenter());
    }
    else
    {
        TriggerBox->SetBoxExtent(FVector(50.0f, 50.0f, 20.0f));
        TriggerBox->SetRelativeLocation(FVector(0.0f, 0.0f, 10.0f));
    }
}

void AStandButtonActor::BeginPlay()
{
    Super::BeginPlay();

    if (BaseMesh && !BaseMesh->GetStaticMesh())
    {
        BaseMesh->SetStaticMesh(UAssetManager::LoadObjStaticMesh("Assets/Models/Pedestal Button/pedestal_button_base.obj"));
    }
    if (ButtonMesh && !ButtonMesh->GetStaticMesh())
    {
        ButtonMesh->SetStaticMesh(UAssetManager::LoadObjStaticMesh("Assets/Models/Pedestal Button/pedestal_button_switch.obj"));
    }
}

void AStandButtonActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsPressed && ResetDelay > 0.0f)
    {
        CurrentResetTimer += DeltaTime;
        if (CurrentResetTimer >= ResetDelay)
        {
            ReleaseButton();
            CurrentResetTimer = 0.0f;
        }
    }
}

void AStandButtonActor::Interact(AActor* Interactor)
{
    if (!bIsPressed)
    {
        PressButton();
        CurrentResetTimer = 0.0f;
    }
}

void AStandButtonActor::PressButton()
{
    Super::PressButton();
    HTR_LOG(Info, "[StandButton] Pressed!");
}

void AStandButtonActor::ReleaseButton()
{
    Super::ReleaseButton();
    HTR_LOG(Info, "[StandButton] Released (Reset)!");
}
