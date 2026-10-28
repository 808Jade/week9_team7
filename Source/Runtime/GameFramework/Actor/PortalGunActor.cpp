#include "EnginePCH.h"
#include "GameFramework/Actor/PortalGunActor.h"
#include "GameFramework/PortalPawn.h"
#include "GameFramework/Actor/PortalCubeActor.h"
#include "GameFramework/Actor/PortalActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Camera/CameraComponent.h"
#include "Asset/AssetManager.h"
#include "Engine/World.h"
#include "Collision/Ray.h"
#include "Collision/HitResult.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraModifier_Recoil.h"
#include "GameFramework/PlayerController.h"

#include "Audio/AudioDevice.h"
#include "Audio/Sound/SoundWave.h"
#include "Asset/AssetManager.h"

APortalGunActor::APortalGunActor()
{
	PrimaryActorTick.bCanEverTick = true;	
	SetActorEnableCollision(false);

	// 1. 루트 메시 컴포넌트
	GunMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("GunMeshComponent");
	SetRootComponent(GunMeshComponent);

	// 2. 포탈건 3D 메시 연결
	UStaticMesh* GunMesh = UAssetManager::GetAssetByPath<UStaticMesh>("Models/PortalGun/portal_gun.obj");
	if (GunMesh)
	{
		GunMeshComponent->SetStaticMesh(GunMesh);
	}

	// 3. 홀드 레이저 (코어 + 외곽)
	for (int32 i = 0; i < LaserSegmentCount; ++i)
	{
		LaserGlowSegments.Add(CreateLaserComponent("LaserGlow" + std::to_string(i), "LaserGlowMaterial"));
		LaserCoreSegments.Add(CreateLaserComponent("LaserCore" + std::to_string(i), "LaserCoreMaterial"));
	}

	// 4. 레이저 조명
	MuzzleLight = CreateLaserLight("LaserMuzzleLight", 1.5f);
	BeamLight   = CreateLaserLight("LaserBeamLight",   2.5f);
	CubeLight   = CreateLaserLight("LaserCubeLight",   2.0f);
	// 5. 발사 빔 머티리얼 (색별)
	ShotCoreMaterials[(int32)EPortalType::Blue] = UAssetManager::GetAssetByPath<UMaterial>("PortalShotBlueCoreMaterial");
	ShotGlowMaterials[(int32)EPortalType::Blue] = UAssetManager::GetAssetByPath<UMaterial>("PortalShotBlueGlowMaterial");
	ShotCoreMaterials[(int32)EPortalType::Orange] = UAssetManager::GetAssetByPath<UMaterial>("PortalShotOrangeCoreMaterial");
	ShotGlowMaterials[(int32)EPortalType::Orange] = UAssetManager::GetAssetByPath<UMaterial>("PortalShotOrangeGlowMaterial");

	// 6. 발사 빔 (직선이라 원기둥 1개씩). 머티리얼은 발사 시 색에 맞게 교체
	ShotGlow = CreateLaserComponent("ShotGlow", "PortalShotBlueGlowMaterial");
	ShotCore = CreateLaserComponent("ShotCore", "PortalShotBlueCoreMaterial");
}

UPointLightComponent* APortalGunActor::CreateLaserLight(const char* Name, float AttenuationRadius)
{
	UPointLightComponent* Light = CreateDefaultSubobject<UPointLightComponent>(Name);
	Light->SetupAttachment(GunMeshComponent);
	Light->SetLightColor(FColor(60, 160, 255, 255));   // 레이저 외곽색과 같은 포탈 파란색
	Light->SetAttenuationRadius(AttenuationRadius);
	Light->SetCastShadows(true);                      // 움직이는 라이트 3개라 그림자는 끈다
	Light->SetIntensity(0.0f);
	Light->SetVisibility(false);
	return Light;
}

void APortalGunActor::TurnOffLaserLights()
{
	for (UPointLightComponent* Light : { MuzzleLight, BeamLight, CubeLight })
	{
		if (!Light) continue;
		Light->SetIntensity(0.0f);
		Light->SetVisibility(false);
	}
}

void APortalGunActor::UpdateLaserLights(const FVector& P0, const FVector& P1, const FVector& P2, float Pulse)
{
	if (!MuzzleLight || !BeamLight || !CubeLight)
	{
		return;
	}

	// 큐브 중심에 라이트를 두면 큐브 안쪽이라 표면이 안 밝아진다.
	// 곡선의 T=0.85 지점(큐브 바로 앞)에 둬서 총을 향한 면과 주변 바닥을 비춘다.
	const FVector BeamMid = QuadBezier(P0, P1, P2, 0.5f);
	const FVector NearCube = QuadBezier(P0, P1, P2, 0.85f);

	MuzzleLight->SetWorldLocation(P0, false);
	BeamLight->SetWorldLocation(BeamMid, false);
	CubeLight->SetWorldLocation(NearCube, false);

	const float Base = LaserLightIntensity * LaserAlpha * Pulse;
	MuzzleLight->SetIntensity(0.8f * Base);
	BeamLight->SetIntensity(0.6f * Base);
	CubeLight->SetIntensity(1.0f * Base);

	MuzzleLight->SetVisibility(true);
	BeamLight->SetVisibility(true);
	CubeLight->SetVisibility(true);
}

UStaticMeshComponent* APortalGunActor::CreateLaserComponent(const std::string& Name, const char* MaterialKey)
{
	UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(Name.c_str());
	Comp->SetupAttachment(GunMeshComponent);
	Comp->SetStaticMesh(UAssetManager::GetAssetByPath<UStaticMesh>("Cylinder"));

	UMaterial* Mat = UAssetManager::GetAssetByPath<UMaterial>(MaterialKey);
	if (!Mat)
	{
		HTR_LOG(Warning, "PortalGun: {} not found", MaterialKey);
	}
	Comp->SetMaterial(0, Mat);
	Comp->SetCastShadow(false);
	Comp->SetReceiveShadow(false);
	Comp->SetVisibility(false);
	return Comp;
}

void APortalGunActor::BeginPlay()
{
	Super::BeginPlay();
	HeldCube = nullptr;

	// 포탈 2개를 미리 만들어 두고(숨김), 쏠 때마다 이동만 시킨다
	if (!BluePortal && GetWorld())
	{
		APortalActor::SpawnPair(GetWorld(), BluePortal, OrangePortal);
	}
}

void APortalGunActor::EndPlay()
{
	Super::EndPlay();
	StopHoldSound();
	if (HeldCube)
	{
		HeldCube = nullptr;
	}
}

void APortalGunActor::AttachToPawn(APortalPawn* InOwnerPawn)
{
	OwnerPawn = InOwnerPawn;
	if (InOwnerPawn && GunMeshComponent)
	{
		UCameraComponent* Cam = InOwnerPawn->GetCameraComponent();
		if (Cam)
		{
			GunMeshComponent->SetupAttachment(Cam, EAttachmentRule::KeepRelative);
			GunMeshComponent->SetRelativeLocation(GunOffsetLocation);
			GunMeshComponent->SetRelativeRotation(GunOffsetRotation);
			GunMeshComponent->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
		}
	}
}

void APortalGunActor::Hold(APortalCubeActor* CubeToGrab)
{
	if(!CubeToGrab)
	{
		return;
	}

	HTR_LOG(Warning, "Hold");

	HeldCube = CubeToGrab;
	HeldCube->OnGrabbed();
	StartHoldSound();
}

void APortalGunActor::Unhold()
{
	StopHoldSound();
	if(HeldCube)
	{
		HTR_LOG(Warning, "Unhold");

		HeldCube->OnDropped(FVector::ZeroVector);
		HeldCube = nullptr;
	}
}

void APortalGunActor::ResetPortals()
{
	if (BluePortal)
	{
		BluePortal->ResetPortal();
	}
	if (OrangePortal)
	{
		OrangePortal->ResetPortal();
	}
}

void APortalGunActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsSwinging && GunMeshComponent)
	{
		SwingTimer += DeltaTime;
		const float StrikeTime = SwingDuration * 0.35f;
		const FVector StrikeLocation = GunOffsetLocation + FVector(0.25f, -0.12f, -0.05f);
		const FRotator StrikeRotation = GunOffsetRotation + FRotator(-15.0f, -25.0f, 35.0f);
		if (SwingTimer < StrikeTime)
		{
			// 진행률
			const float Alpha = SwingTimer / StrikeTime;
			const float EasedAlpha = sinf(Alpha * 1.5707963f); // sin(0 ~ PI/2)
			GunMeshComponent->SetRelativeLocation(GunOffsetLocation + (StrikeLocation - GunOffsetLocation) * EasedAlpha);
			GunMeshComponent->SetRelativeRotation(FRotator(
				GunOffsetRotation.Pitch + (StrikeRotation.Pitch - GunOffsetRotation.Pitch) * EasedAlpha,
				GunOffsetRotation.Yaw + (StrikeRotation.Yaw - GunOffsetRotation.Yaw) * EasedAlpha,
				GunOffsetRotation.Roll + (StrikeRotation.Roll - GunOffsetRotation.Roll) * EasedAlpha
			));
		}
		else if (SwingTimer < SwingDuration)
		{
			const float Alpha = (SwingTimer - StrikeTime) / (SwingDuration - StrikeTime);
			const float EasedAlpha = Alpha * Alpha * (3.0f - 2.0f * Alpha);
			GunMeshComponent->SetRelativeLocation(StrikeLocation + (GunOffsetLocation - StrikeLocation) * EasedAlpha);
			GunMeshComponent->SetRelativeRotation(FRotator(
				StrikeRotation.Pitch + (GunOffsetRotation.Pitch - StrikeRotation.Pitch) * EasedAlpha,
				StrikeRotation.Yaw + (GunOffsetRotation.Yaw - StrikeRotation.Yaw) * EasedAlpha,
				StrikeRotation.Roll + (GunOffsetRotation.Roll - StrikeRotation.Roll) * EasedAlpha
			));
		}
		else
		{
			bIsSwinging = false;
			SwingTimer = 0.0f;
			GunMeshComponent->SetRelativeLocation(GunOffsetLocation);
			GunMeshComponent->SetRelativeRotation(GunOffsetRotation);
		}
	}

	if (IsHoldingCube() && OwnerPawn)
	{
		UCameraComponent* Cam = OwnerPawn->GetCameraComponent();
		if (Cam)
		{
			FVector CamLoc = Cam->GetWorldLocation();
			FVector CamForward = Cam->GetWorldRotation().Quaternion().GetForwardVector().Normalized();
			FVector TargetLocation = CamLoc + (CamForward * HoldDistance);
			FRotator TargetRotation = Cam->GetWorldRotation();

			float DistanceToTarget = FVector::Distance(HeldCube->GetActorLocation(), TargetLocation);
			if (DistanceToTarget > MaxBreakDistance)
			{
				Unhold();
			}
			else
			{
				HeldCube->UpdateHoldPhysics(TargetLocation, TargetRotation, DeltaTime);
			}
		}
	}

	// 들고 있든 아니든 매 프레임 갱신해야 켜짐/꺼짐 보간이 동작한다
	UpdateLaser(DeltaTime);
	UpdateShotBeam(DeltaTime);
}

void APortalGunActor::SetLaserVisibility(bool bVisible)
{
	for (UStaticMeshComponent* Comp : LaserCoreSegments) Comp->SetVisibility(bVisible);
	for (UStaticMeshComponent* Comp : LaserGlowSegments) Comp->SetVisibility(bVisible);
}

FVector APortalGunActor::QuadBezier(const FVector& P0, const FVector& P1, const FVector& P2, float T)
{
	// B(t) = (1-t)^2 P0 + 2(1-t)t P1 + t^2 P2
	const float U = 1.0f - T;
	return P0 * (U * U) + P1 * (2.0f * U * T) + P2 * (T * T);
}

void APortalGunActor::UpdateLaser(float DeltaTime)
{
	if (LaserCoreSegments.Num() != LaserSegmentCount || LaserGlowSegments.Num() != LaserSegmentCount)
	{
		return;
	}

	// 잡으면 0→1로 빠르게 보간해 굵기에 곱한다 (레이저가 가늘게 시작해 확 켜지는 느낌)
	const float Target = IsHoldingCube() ? 1.0f : 0.0f;
	LaserAlpha += (Target - LaserAlpha) * FMath::Min(1.0f, DeltaTime * 20.0f);
	LaserTime += DeltaTime;

	UCameraComponent* Cam = OwnerPawn ? OwnerPawn->GetCameraComponent() : nullptr;
	const bool bVisible = IsHoldingCube() && Cam && LaserAlpha > 0.01f;
	SetLaserVisibility(bVisible);
	if (!bVisible)
	{
		TurnOffLaserLights();
		return;
	}

	// 베지어 세 점
	// P0: 총구, P2: 큐브 중심
	// P1: 총구에서 카메라 정면으로 뻗은 점. 큐브가 조준선에서 늦게 따라올수록 곡선이 휜다
	const FVector CamForward = Cam->GetWorldRotation().Quaternion().GetForwardVector().Normalized();
	const FVector P0 = GetMuzzleLocation();
	const FVector P2 = HeldCube->GetActorLocation();
	const FVector P1 = P0 + CamForward * (FVector::Distance(P0, P2) * LaserBendRatio);

	// 레이저 특유의 미세한 떨림
	const float Flicker = 0.85f + 0.15f * std::sin(LaserTime * 45.0f);
	const float CoreRadius = LaserCoreRadius * LaserAlpha;
	const float GlowRadius = LaserGlowRadius * LaserAlpha * Flicker;

	// 곡선을 구간으로 나눠 구간마다 원기둥 배치
	FVector Prev = P0;
	for (int32 i = 0; i < LaserSegmentCount; ++i)
	{
		const float T = float(i + 1) / LaserSegmentCount;
		const FVector Next = QuadBezier(P0, P1, P2, T);

		// 꺾이는 이음새에 틈이 보이지 않도록 구간 양 끝을 반지름만큼 살짝 늘린다
		FVector SegDir = Next - Prev;
		const float SegLen = SegDir.Size();
		if (SegLen > 1e-4f)
		{
			SegDir = SegDir / SegLen;
		}
		const FVector Pad = SegDir * GlowRadius;
		const FVector SegStart = (i == 0) ? Prev : Prev - Pad;
		const FVector SegEnd = (i == LaserSegmentCount - 1) ? Next : Next + Pad;

		PlaceCylinder(LaserCoreSegments[i], SegStart, SegEnd, CoreRadius);
		PlaceCylinder(LaserGlowSegments[i], SegStart, SegEnd, GlowRadius);

		Prev = Next;
	}

	// 조명: 떨림보다 느린 맥동으로 은은하게 숨쉬듯
	const float Pulse = 0.85f + 0.15f * std::sin(LaserTime * 10.0f);
	UpdateLaserLights(P0, P1, P2, Pulse);
}

void APortalGunActor::PlaceCylinder(UStaticMeshComponent* Comp, const FVector& Start, const FVector& End, float Radius)
{
	FVector Dir = End - Start;
	const float Length = Dir.Size();
	if (Length < 1e-4f)
	{
		return;
	}
	Dir = Dir / Length;

	// Dir에 수직인 두 축을 만든다 (X × Y = Dir 이 되도록)
	const FVector Ref = (std::abs(Dir.Z) < 0.99f) ? FVector(0.0f, 0.0f, 1.0f) : FVector(1.0f, 0.0f, 0.0f);
	const FVector AxisX = FVector::Cross(Ref, Dir).Normalized();
	const FVector AxisY = FVector::Cross(Dir, AxisX);

	const FVector Mid = (Start + End) * 0.5f;
	const float R = FMath::Max(Radius, 1e-4f);

	// 행벡터 규약: 0~2행 = 스케일이 곱해진 축, 3행 = 위치
	// 원기둥 메시는 로컬 Z가 길이(높이 1, 중심 원점) → Z축을 Dir * Length로
	const FMatrix World(
		AxisX.X * R,     AxisX.Y * R,     AxisX.Z * R,     0.0f,
		AxisY.X * R,     AxisY.Y * R,     AxisY.Z * R,     0.0f,
		Dir.X * Length,  Dir.Y * Length,  Dir.Z * Length,  0.0f,
		Mid.X,           Mid.Y,           Mid.Z,           1.0f);

	Comp->SetWorldTransform(World, false);
}

void APortalGunActor::PlayFireSound(EPortalType Type)
{
	IAudioBackend* AudioBackend = FAudioDevice::GetInstance().GetBackend();
	if(!AudioBackend)
	{
		return;
	}

	const FString SoundPath = (Type == EPortalType::Blue) ? "Audio/Portal2_sfx_portal_gun_fire_blue.mp3" : "Audio/Portal2_sfx_portal_gun_fire_orange.mp3";
	USoundWave* SoundWave = UAssetManager::GetAssetByPath<USoundWave>(SoundPath);
	if(SoundWave && SoundWave->GetSoundID() != INVALID_SOUND_ID)
	{		
		AudioBackend->Play(SoundWave->GetSoundID(), false, 1.0f, 1.0f, false, FVector::ZeroVector, 0.0f, 0.0f);
	}
}

FVector APortalGunActor::GetMuzzleLocation() const
{
	return GunMeshComponent->GetWorldMatrix().TransformPosition(MuzzleLocalOffset);
}

bool APortalGunActor::FirePortal(const EPortalType PortalToFire, const FHitResult& HitResult, const FVector& ViewerForward)
{
	APortalActor* Portal = GetPortal(PortalToFire);
	if (!Portal || !HitResult.bBlockingHit)
	{
		PlayFailSound();
		return false;
	}

	FVector Forward = ViewerForward;
	if (Forward.SizeSquared() < 1e-4f && OwnerPawn && OwnerPawn->GetCameraComponent())
	{
		Forward = OwnerPawn->GetCameraComponent()->GetWorldRotation().Quaternion().GetForwardVector().Normalized();
	}

	if (Portal->PlaceAt(HitResult, Forward))
	{
		PlayFireSound(PortalToFire);
		return true;
	}

	PlayFailSound();
	return false;
}

void APortalGunActor::PlayFireEffect(const FPortalShotInfo& InShot)
{
	CurrentShot = InShot;
	CurrentShot.Start = GetMuzzleLocation();
	ShotElapsed = 0.0f;
	bShotActive = true;

	ShotCore->SetMaterial(0, GetShotCoreMaterial(CurrentShot.Type));
	ShotGlow->SetMaterial(0, GetShotGlowMaterial(CurrentShot.Type));
	SetShotBeamVisibility(true);
	UpdateShotBeam(0.0f);   // 이번 프레임에 바로 보이게

	// 반동: 포탈 성공 여부와 상관없이 쏠 때마다
	if (!RecoilModifier && GetWorld())
	{
		if (APlayerController* PC = GetWorld()->GetPlayerController())
		{
			if (APlayerCameraManager* PCM = PC->GetPlayerCameraManager())
			{
				RecoilModifier = PCM->AddNewCameraModifier<UCameraModifier_Recoil>();
			}
		}
	}
	if (RecoilModifier)
	{
		RecoilModifier->AddRecoil(2.0f, -0.2f, 0);
	}

	HTR_LOG(Warning, "Shot: Type={} Hit={} Placed={} Dist={}",
		(int32)CurrentShot.Type, CurrentShot.bHit, CurrentShot.bPortalPlaced,
		FVector::Distance(CurrentShot.Start, CurrentShot.End));
}

void APortalGunActor::SetShotBeamVisibility(bool bVisible)
{
	ShotCore->SetVisibility(bVisible);
	ShotGlow->SetVisibility(bVisible);
}

void APortalGunActor::UpdateShotBeam(float DeltaTime)
{
	if (!bShotActive)
	{
		return;
	}

	ShotElapsed += DeltaTime;
	if (ShotElapsed >= ShotDuration)
	{
		bShotActive = false;
		SetShotBeamVisibility(false);
		return;
	}

	PlaceCylinder(ShotGlow, CurrentShot.Start, CurrentShot.End, ShotGlowRadius);
	PlaceCylinder(ShotCore, CurrentShot.Start, CurrentShot.End, ShotCoreRadius);
}

void APortalGunActor::PlayFailSound()
{
	IAudioBackend* AudioBackend = FAudioDevice::GetInstance().GetBackend();
	if (!AudioBackend)
	{
		return;
	}

	USoundWave* SoundWave = UAssetManager::GetAssetByPath<USoundWave>("Audio/Portal_Fail.mp3");
	if (SoundWave && SoundWave->GetSoundID() != INVALID_SOUND_ID)
	{
		AudioBackend->Play(SoundWave->GetSoundID(), false, 1.0f, 1.0f, false, FVector::ZeroVector, 0.0f, 0.0f);
	}
}

void APortalGunActor::StartHoldSound()
{
	IAudioBackend* AudioBackend = FAudioDevice::GetInstance().GetBackend();
	if (!AudioBackend)
	{
		return;
	}

	if (HoldVoiceID != -1 && AudioBackend->IsPlaying(HoldVoiceID))
	{
		return;
	}

	USoundWave* SoundWave = UAssetManager::GetAssetByPath<USoundWave>("Audio/hold.mp3");
	if (SoundWave && SoundWave->GetSoundID() != INVALID_SOUND_ID)
	{
		HoldVoiceID = AudioBackend->Play(SoundWave->GetSoundID(), true, 0.8f, 1.0f, false, FVector::ZeroVector, 0.0f, 0.0f);
	}
}

void APortalGunActor::StopHoldSound()
{
	if (HoldVoiceID != -1)
	{
		if (IAudioBackend* AudioBackend = FAudioDevice::GetInstance().GetBackend())
		{
			AudioBackend->Stop(HoldVoiceID);
		}
		HoldVoiceID = -1;
	}
}


void APortalGunActor::Swing()
{
	if (bIsSwinging || IsHoldingCube())
	{
		return;
	}
	bIsSwinging = true;
	SwingTimer = 0.0f;
}
