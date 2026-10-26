#include "EnginePCH.h"
#include "DoorActor.h"
#include "Asset/AssetManager.h"
#include <algorithm>
#include <cmath>

ADoorActor::ADoorActor()
{
    PrimaryActorTick.bCanEverTick = true;

    BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>("BaseMesh");
    LeftDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>("LeftDoorMesh");
    RightDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>("RightDoorMesh");
    TriggerBox = CreateDefaultSubobject<UBoxComponent>("TriggerBox");

    BaseMesh->SetupAttachment(GetRootComponent());
    LeftDoorMesh->SetupAttachment(GetRootComponent());
    RightDoorMesh->SetupAttachment(GetRootComponent());
    TriggerBox->SetupAttachment(GetRootComponent());

    // 기본 메쉬 로드 및 할당
    if (UStaticMesh* Base = UAssetManager::LoadObjStaticMesh("Assets/Models/PortalDoor/portal_door_base.obj"))
    {
        BaseMesh->SetStaticMesh(Base);
    }
    if (UStaticMesh* Left = UAssetManager::LoadObjStaticMesh("Assets/Models/PortalDoor/portal_door_left.obj"))
    {
        LeftDoorMesh->SetStaticMesh(Left);
    }
    if (UStaticMesh* Right = UAssetManager::LoadObjStaticMesh("Assets/Models/PortalDoor/portal_door_right.obj"))
    {
        RightDoorMesh->SetStaticMesh(Right);
    }

    // 트리거 박스 기본 크기 및 위치 (문 전체를 넉넉하게 감싸도록)
    TriggerBox->SetBoxExtent(FVector(1.0f, 1.0f, 0.8f));
    TriggerBox->SetRelativeLocation(FVector(0.0f, 0.0f, 0.6f));
}

void ADoorActor::BeginPlay()
{
    Super::BeginPlay();

    // 초기 상대 위치 저장 (닫힌 상태)
    if (LeftDoorMesh)
    {
        if (!LeftDoorMesh->GetStaticMesh())
        {
            LeftDoorMesh->SetStaticMesh(UAssetManager::LoadObjStaticMesh("Assets/Models/PortalDoor/portal_door_left.obj"));
        }
        InitialLeftDoorLoc = LeftDoorMesh->GetRelativeLocation();
    }
    if (RightDoorMesh)
    {
        if (!RightDoorMesh->GetStaticMesh())
        {
            RightDoorMesh->SetStaticMesh(UAssetManager::LoadObjStaticMesh("Assets/Models/PortalDoor/portal_door_right.obj"));
        }
        InitialRightDoorLoc = RightDoorMesh->GetRelativeLocation();
    }
    if (BaseMesh && !BaseMesh->GetStaticMesh())
    {
        BaseMesh->SetStaticMesh(UAssetManager::LoadObjStaticMesh("Assets/Models/PortalDoor/portal_door_base.obj"));
    }

    if (TriggerBox)
    {
        TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ADoorActor::OnTriggerBeginOverlap);
        TriggerBox->OnComponentEndOverlap.AddDynamic(this, &ADoorActor::OnTriggerEndOverlap);
    }
}

void ADoorActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!LeftDoorMesh || !RightDoorMesh) return;

    // 좌문은 -Y 방향으로, 우문은 +Y 방향으로 슬라이딩
    FVector TargetLeftLoc = InitialLeftDoorLoc;
    FVector TargetRightLoc = InitialRightDoorLoc;

    if (bIsOpen)
    {
        TargetLeftLoc.Y -= SlideDistance;
        TargetRightLoc.Y += SlideDistance;
    }

    // 부드러운 보간
    float Alpha = std::clamp(OpenSpeed * DeltaTime, 0.0f, 1.0f);

    FVector CurrentLeft = LeftDoorMesh->GetRelativeLocation();
    CurrentLeft.Y += (TargetLeftLoc.Y - CurrentLeft.Y) * Alpha;
    if (std::abs(CurrentLeft.Y - TargetLeftLoc.Y) < 0.001f)
    {
        CurrentLeft.Y = TargetLeftLoc.Y;
    }
    LeftDoorMesh->SetRelativeLocation(CurrentLeft);

    FVector CurrentRight = RightDoorMesh->GetRelativeLocation();
    CurrentRight.Y += (TargetRightLoc.Y - CurrentRight.Y) * Alpha;
    if (std::abs(CurrentRight.Y - TargetRightLoc.Y) < 0.001f)
    {
        CurrentRight.Y = TargetRightLoc.Y;
    }
    RightDoorMesh->SetRelativeLocation(CurrentRight);
}

void ADoorActor::OpenDoor()
{
    bIsOpen = true;
}

void ADoorActor::CloseDoor()
{
    bIsOpen = false;
}

void ADoorActor::ToggleDoor()
{
    bIsOpen = !bIsOpen;
}

void ADoorActor::SetOpen(bool bOpen)
{
    bIsOpen = bOpen;
}

void ADoorActor::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == this) return;

    ++OverlappingActorCount;
    if (bAutoOpenOnOverlap)
    {
        OpenDoor();
    }
}

void ADoorActor::OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp)
{
    if (!OtherActor || OtherActor == this) return;

    --OverlappingActorCount;
    if (OverlappingActorCount <= 0)
    {
        OverlappingActorCount = 0;
        if (bAutoOpenOnOverlap)
        {
            CloseDoor();
        }
    }
}
