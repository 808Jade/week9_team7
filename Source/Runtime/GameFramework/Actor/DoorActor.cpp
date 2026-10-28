#include "EnginePCH.h"
#include "DoorActor.h"
#include "Asset/AssetManager.h"
#include "Audio/AudioDevice.h"
#include "Audio/Sound/SoundWave.h"
#include <algorithm>
#include <cmath>

ADoorActor::ADoorActor()
{
    PrimaryActorTick.bCanEverTick = true;

    BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>("BaseMesh");
    LeftDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>("LeftDoorMesh");
    RightDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>("RightDoorMesh");

    // 좌문/우문 물리 충돌 박스 생성
    LeftDoorCollision = CreateDefaultSubobject<UBoxComponent>("LeftDoorCollision");
    RightDoorCollision = CreateDefaultSubobject<UBoxComponent>("RightDoorCollision");

    // 계층 구조 설정
    BaseMesh->SetupAttachment(GetRootComponent());
    LeftDoorMesh->SetupAttachment(GetRootComponent());
    RightDoorMesh->SetupAttachment(GetRootComponent());

    // 충돌 박스는 각 문짝 메시에 직접 부착하여, 문이 열리고 닫힐 때 함께 이동하도록 설정
    LeftDoorCollision->SetupAttachment(LeftDoorMesh);
    RightDoorCollision->SetupAttachment(RightDoorMesh);

    // 충돌 응답 설정
    LeftDoorCollision->SetCurrentCollisionResponse(ECollisionResponse::Block);
    RightDoorCollision->SetCurrentCollisionResponse(ECollisionResponse::Block);

    // 기본 메시 로드 및 할당
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

    // 좌/우 문짝 메시의 AABB(최소외접육면체)를 계산하여 충돌 박스에 각각 할당
    UpdateCollisionBoxes();
}

void ADoorActor::BeginPlay()
{
    Super::BeginPlay();

    // 씬 로드 후 메시가 비어있다면 다시 할당
    if (BaseMesh && !BaseMesh->GetStaticMesh())
    {
        BaseMesh->SetStaticMesh(UAssetManager::LoadObjStaticMesh("Assets/Models/PortalDoor/portal_door_base.obj"));
    }
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

    // 씬 로드 후에도 정확한 메시 AABB 기반으로 충돌 박스 갱신
    UpdateCollisionBoxes();
}

void ADoorActor::UpdateCollisionBoxes()
{
    // 왼쪽 문짝의 최소외접육면체(AABB) 계산 및 좌문 충돌 박스에 할당
    if (LeftDoorMesh && LeftDoorCollision)
    {
        FBox LeftBounds = LeftDoorMesh->CalcLocalBounds();
        FVector Extent = LeftBounds.GetExtent();
        if (Extent.X > 0.001f && Extent.Y > 0.001f && Extent.Z > 0.001f)
        {
            // 두께(X)는 얇은 문짝 통과(터널링) 방지를 위해 최소 0.15f 보장
            Extent.X = std::max(Extent.X, 0.15f);
            LeftDoorCollision->SetBoxExtent(Extent);
            LeftDoorCollision->SetRelativeLocation(LeftBounds.GetCenter());
        }
    }

    // 오른쪽 문짝의 최소외접육면체(AABB) 계산 및 우문 충돌 박스에 할당
    if (RightDoorMesh && RightDoorCollision)
    {
        FBox RightBounds = RightDoorMesh->CalcLocalBounds();
        FVector Extent = RightBounds.GetExtent();
        if (Extent.X > 0.001f && Extent.Y > 0.001f && Extent.Z > 0.001f)
        {
            Extent.X = std::max(Extent.X, 0.15f);
            RightDoorCollision->SetBoxExtent(Extent);
            RightDoorCollision->SetRelativeLocation(RightBounds.GetCenter());
        }
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
    if (std::abs(CurrentLeft.Y - TargetLeftLoc.Y) > 0.0001f)
    {
        CurrentLeft.Y += (TargetLeftLoc.Y - CurrentLeft.Y) * Alpha;
        if (std::abs(CurrentLeft.Y - TargetLeftLoc.Y) < 0.001f)
        {
            CurrentLeft.Y = TargetLeftLoc.Y;
        }
        LeftDoorMesh->SetRelativeLocation(CurrentLeft);
    }

    FVector CurrentRight = RightDoorMesh->GetRelativeLocation();
    if (std::abs(CurrentRight.Y - TargetRightLoc.Y) > 0.0001f)
    {
        CurrentRight.Y += (TargetRightLoc.Y - CurrentRight.Y) * Alpha;
        if (std::abs(CurrentRight.Y - TargetRightLoc.Y) < 0.001f)
        {
            CurrentRight.Y = TargetRightLoc.Y;
        }
        RightDoorMesh->SetRelativeLocation(CurrentRight);
    }

    // 자식 컴포넌트인 LeftDoorCollision과 RightDoorCollision은
    // 부모인 LeftDoorMesh / RightDoorMesh를 따라 자동으로 함께 좌우로 물러납니다.
}

void ADoorActor::OpenDoor()
{
    if (!bIsOpen)
    {
        PlayDoorOpenSound();
    }
    bIsOpen = true;
}

void ADoorActor::PlayDoorOpenSound()
{
    IAudioBackend* AudioBackend = FAudioDevice::GetInstance().GetBackend();
    if (!AudioBackend)
    {
        return;
    }

    USoundWave* SoundWave = UAssetManager::GetAssetByPath<USoundWave>("Audio/Door_Open.mp3");
    if (SoundWave && SoundWave->GetSoundID() != INVALID_SOUND_ID)
    {
        AudioBackend->Play(SoundWave->GetSoundID(), false, 1.0f, 1.0f, true, GetActorLocation(), 5.0f, 100.0f);
    }
}

void ADoorActor::CloseDoor()
{
    bIsOpen = false;
}

void ADoorActor::ToggleDoor()
{
    SetOpen(!bIsOpen);
}

void ADoorActor::SetOpen(bool bOpen)
{
    if (bOpen && !bIsOpen)
    {
        PlayDoorOpenSound();
    }
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
