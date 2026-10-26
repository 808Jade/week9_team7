#include "EnginePCH.h"
#include "WeightedButtonActor.h"
#include "Asset/AssetManager.h"

AWeightedButtonActor::AWeightedButtonActor()
{
    // 기본 포탈 버튼 메시 할당
    if (UStaticMesh* Base = UAssetManager::LoadObjStaticMesh("Assets/Models/PortalButton/portal_button_base.obj"))
    {
        BaseMesh->SetStaticMesh(Base);
    }
    if (UStaticMesh* Button = UAssetManager::LoadObjStaticMesh("Assets/Models/PortalButton/portal_button_switch.obj"))
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

void AWeightedButtonActor::BeginPlay()
{
    Super::BeginPlay();

    // 씬 로드 후 메시가 비어있다면 다시 할당
    if (BaseMesh && !BaseMesh->GetStaticMesh())
    {
        BaseMesh->SetStaticMesh(UAssetManager::LoadObjStaticMesh("Assets/Models/PortalButton/portal_button_base.obj"));
    }
    if (ButtonMesh && !ButtonMesh->GetStaticMesh())
    {
        ButtonMesh->SetStaticMesh(UAssetManager::LoadObjStaticMesh("Assets/Models/PortalButton/portal_button_switch.obj"));
    }

    // 외접 큐브 갱신
    if (ButtonMesh && TriggerBox)
    {
        FBox LocalBounds = ButtonMesh->CalcLocalBounds();
        FVector Extent = LocalBounds.GetExtent();
        if (Extent.X > 0.001f && Extent.Y > 0.001f && Extent.Z > 0.001f)
        {
            TriggerBox->SetBoxExtent(Extent);
            TriggerBox->SetRelativeLocation(LocalBounds.GetCenter());
        }
    }

    // 델리게이트 바인딩: 트리거 박스 오버랩 이벤트 등록
    if (TriggerBox)
    {
        TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AWeightedButtonActor::OnTriggerBeginOverlap);
        TriggerBox->OnComponentEndOverlap.AddDynamic(this, &AWeightedButtonActor::OnTriggerEndOverlap);
    }
}

void AWeightedButtonActor::PressButton()
{
    if (!bIsPressed)
    {
        Super::PressButton();
        HTR_LOG(Info, "[WeightedButton] Pressed! (Objects: {})", OverlappingActors.Num());
    }
}

void AWeightedButtonActor::ReleaseButton()
{
    if (bIsPressed)
    {
        Super::ReleaseButton();
        HTR_LOG(Info, "[WeightedButton] Released!");
    }
}

void AWeightedButtonActor::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == this) return;

    if (!OverlappingActors.Contains(OtherActor))
    {
        OverlappingActors.Add(OtherActor);
        UpdateButtonState();
    }
}

void AWeightedButtonActor::OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp)
{
    if (!OtherActor || OtherActor == this) return;

    if (OverlappingActors.Contains(OtherActor))
    {
        OverlappingActors.Remove(OtherActor);
        UpdateButtonState();
    }
}

void AWeightedButtonActor::UpdateButtonState()
{
    // 혹시 도중에 파괴된 유효하지 않은 액터 포인터 정리
    for (int32 i = OverlappingActors.Num() - 1; i >= 0; --i)
    {
        if (!OverlappingActors[i])
        {
            OverlappingActors.RemoveAt(i, 1);
        }
    }

    if (!OverlappingActors.IsEmpty())
    {
        PressButton();
    }
    else
    {
        ReleaseButton();
    }
}
