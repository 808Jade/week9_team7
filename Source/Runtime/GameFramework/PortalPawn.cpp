#include "EnginePCH.h"
#include "GameFramework/PortalPawn.h"
#include "GameFramework/Actor/PortalGunActor.h"
#include "GameFramework/Actor/PortalActor.h"
#include "GameFramework/PlayerStart.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "GameFramework/Actor/PortalCubeActor.h"
#include "GameFramework/Actor/PortalActor.h"
#include "GameFramework/Actor/UIManagerActor.h"
#include "Components/LuaScriptComponent.h"
#include "Collision/Ray.h"
#include "Collision/HitResult.h"
#include "UObject/Casts.h"

APortalPawn::APortalPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorEnableCollision(true);

	// 1. 루트 구 콜라이더
	//SphereComponent = CreateDefaultSubobject<USphereComponent>("SphereComponent");
	//SetRootComponent(SphereComponent);
	//SphereComponent->SetSphereRadius(CollisionRadius);

	BoxComponent = CreateDefaultSubobject<UBoxComponent>("BoxComponet");
	SetRootComponent(BoxComponent);
	BoxComponent->SetBoxExtent(FVector(0.5f, 0.5f, 1.5f));

	// 2. 1인칭 카메라 컴포넌트
	CameraComponent = CreateDefaultSubobject<UCameraComponent>("CameraComponent");
	CameraComponent->SetupAttachment(GetRootComponent());
	CameraComponent->SetRelativeLocation({ 0.0f, 0.0f, EyeHeight });
	CameraComponent->SetRelativeRotation({ 0.0f, 0.0f, 0.0f });

	CameraComponent->SetExternalInputManaged(true);
}

void APortalPawn::BeginPlay()
{
	Super::BeginPlay();
	Velocity = FVector::ZeroVector;
	PendingInputVector = FVector::ZeroVector;
	bIsGrounded = false;
	bIsCrouched = false;
	bIsSprinting = false;
	bIsZooming = false;
	CurrentEyeHeight = EyeHeight;
	CurrentLandingDip = 0.0f;
	CurrentBobX = 0.0f;
	CurrentBobZ = 0.0f;
	BobTimer = 0.0f;
	MaxPreservedFlingSpeed = SprintSpeed;
	TeleportCooldownTimer = 0.0f;

	InitialSpawnLocation = GetActorLocation();
	InitialSpawnRotation = GetActorRotation();

	// 씬에 PlayerStart가 존재하면 시작 위치와 회전을 PlayerStart에 맞춤
	if(GetWorld() && GetWorld()->GetPersistentLevel())
	{
		for(AActor* Actor : GetWorld()->GetPersistentLevel()->GetActors())
		{
			if(APlayerStart* Start = Cast<APlayerStart>(Actor))
			{
				InitialSpawnLocation = Start->GetSpawnLocation();
				InitialSpawnRotation = Start->GetSpawnRotation();
				if(GetRootComponent())
				{
					GetRootComponent()->SetWorldLocationAndRotation(InitialSpawnLocation, FRotator(0.0f, InitialSpawnRotation.Yaw, 0.0f), false);
				}
				if(CameraComponent)
				{
					CameraComponent->SetRelativeRotation(FRotator(InitialSpawnRotation.Pitch, 0.0f, 0.0f));
				}
				break;
			}
		}
	}

	if(CameraComponent)
	{
		DefaultFOV = CameraComponent->GetFieldOfView();
		if(DefaultFOV <= 0.0f)
		{
			DefaultFOV = 60.0f;
		}
		CurrentFOV = DefaultFOV;
	}

	// 게임 시작 시 포탈건 액터 스폰 및 카메라에 장착
	if(GetWorld() && !PortalGun)
	{
		PortalGun = GetWorld()->SpawnActor<APortalGunActor>();
		if(PortalGun)
		{
			PortalGun->AttachToPawn(this);
		}
	}

	if(BoxComponent)
	{
		BoxComponent->SetCurrentCollisionResponse(ECollisionResponse::Block);
		BoxComponent->OnComponentBeginOverlap.AddDynamic(this, &APortalPawn::OnBeginOverlap);
		BoxComponent->OnComponentEndOverlap.AddDynamic(this, &APortalPawn::OnEndOverlap);
	}

	// 크로스헤어 상태 관리 루아 스크립트가 아직 부착되어 있지 않다면 자동 부착
	bool bHasScript = false;
	for (UActorComponent* Comp : GetComponents())
	{
		if (Comp && Comp->IsA<ULuaScriptComponent>())
		{
			bHasScript = true;
			break;
		}
	}
	if (!bHasScript)
	{
		if (ULuaScriptComponent* LuaComp = Cast<ULuaScriptComponent>(AddComponent(ULuaScriptComponent::StaticClass(), "PortalCrosshairScript")))
		{
			LuaComp->SetScriptPath("Scripts/PortalCrosshairController.lua");
		}
	}
}

void APortalPawn::EndPlay()
{
	Super::EndPlay();
	if (PortalGun)
	{
		PortalGun->Destroy();
		PortalGun = nullptr;
	}
}

void APortalPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 0. 추락사(KillZ) 감지 및 리스폰
	if (GetActorLocation().Z < KillZ)
	{
		Respawn();
		return;
	}

	// 0-A. 포탈 재진입 핑퐁 방지 쿨다운 타이머 감소
	if (TeleportCooldownTimer > 0.0f)
	{
		TeleportCooldownTimer -= DeltaTime;
	}

	// 1. 카메라 FOV 줌 보간
	if (CameraComponent)
	{
		const float TargetFOV = bIsZooming ? ZoomedFOV : DefaultFOV;
		CurrentFOV += (TargetFOV - CurrentFOV) * FMath::Clamp(DeltaTime * 15.0f, 0.0f, 1.0f);
		CameraComponent->SetFieldOfView(CurrentFOV);
	}

	// 1-A. 카메라 Roll 수평 복구 (Roll Uprighting: 포탈 통과 후 기울어진 시야를 수평으로 부드럽게 세움)
	if (CameraComponent)
	{
		FRotator CamRot = CameraComponent->GetRelativeRotation();
		if (std::abs(CamRot.Roll) > 0.01f)
		{
			CamRot.Roll += (0.0f - CamRot.Roll) * FMath::Clamp(DeltaTime * 10.0f, 0.0f, 1.0f);
			if (std::abs(CamRot.Roll) < 0.01f)
			{
				CamRot.Roll = 0.0f;
			}
			CameraComponent->SetRelativeRotation(CamRot);
		}
	}

	// 2. 웅크리기(Crouch) 높이 및 콜라이더 반경 보간
	const float TargetEyeHeight = bIsCrouched ? CrouchEyeHeight : EyeHeight;
	const float TargetRadius = bIsCrouched ? CrouchRadius : CollisionRadius;
	CurrentEyeHeight += (TargetEyeHeight - CurrentEyeHeight) * FMath::Clamp(DeltaTime * 12.0f, 0.0f, 1.0f);

	if (SphereComponent)
	{
		SphereComponent->SetSphereRadius(TargetRadius);
	}

	// 3. 착지 충격 반동 (Landing Dip) 복구
	if (CurrentLandingDip > 0.001f)
	{
		CurrentLandingDip += (0.0f - CurrentLandingDip) * FMath::Clamp(DeltaTime * 12.0f, 0.0f, 1.0f);
	}
	else
	{
		CurrentLandingDip = 0.0f;
	}

	// 4. 헤드 밥빙 (Head Bobbing) 계산
	const float HorizSpeedSq = Velocity.X * Velocity.X + Velocity.Y * Velocity.Y;
	if (bEnableHeadBob && bIsGrounded && HorizSpeedSq > 0.25f)
	{
		const float HorizSpeed = sqrtf(HorizSpeedSq);
		BobTimer += DeltaTime * 10.0f * (HorizSpeed / MoveSpeed);
		if (BobTimer > 6.2831853f)
		{
			BobTimer -= 6.2831853f;
		}

		const float TargetBobZ = sinf(BobTimer * 2.0f) * BobAmountVertical;
		const float TargetBobX = cosf(BobTimer) * BobAmountHorizontal;

		CurrentBobZ += (TargetBobZ - CurrentBobZ) * FMath::Clamp(DeltaTime * 15.0f, 0.0f, 1.0f);
		CurrentBobX += (TargetBobX - CurrentBobX) * FMath::Clamp(DeltaTime * 15.0f, 0.0f, 1.0f);
	}
	else
	{
		CurrentBobZ += (0.0f - CurrentBobZ) * FMath::Clamp(DeltaTime * 10.0f, 0.0f, 1.0f);
		CurrentBobX += (0.0f - CurrentBobX) * FMath::Clamp(DeltaTime * 10.0f, 0.0f, 1.0f);
	}

	// 카메라 최종 상대 위치 갱신 (기본 눈높이 - 착지 충격 반동 + 헤드 밥빙)
	if (CameraComponent)
	{
		CameraComponent->SetRelativeLocation({ CurrentBobX, 0.0f, CurrentEyeHeight - CurrentLandingDip + CurrentBobZ });
	}

	// 5. 목표 이동 속도 및 방향 계산
	float TargetMaxSpeed = MoveSpeed;
	if (bIsCrouched)
	{
		TargetMaxSpeed = CrouchSpeed;
	}
	else if (bIsSprinting)
	{
		TargetMaxSpeed = SprintSpeed;
	}

	FVector WishDir = PendingInputVector;
	WishDir.Z = 0.0f;
	const float WishSpeedSq = WishDir.SizeSquared();
	if (WishSpeedSq > 1.0f)
	{
		WishDir = WishDir.GetSafeNormal();
	}
	PendingInputVector = FVector::ZeroVector;

	// 6. 지상 vs 공중 물리 가속 및 감속 (마찰력 & 브레이킹)
	if (bIsGrounded)
	{
		// 착지 시 공중 플링 제한 속도를 기본 달리기 속도로 리셋
		MaxPreservedFlingSpeed = SprintSpeed;

		const float GroundAcceleration = 25.0f;
		const FVector DesiredVelocity = WishDir * TargetMaxSpeed;

		const float HorizSpeed = sqrtf(HorizSpeedSq);

		if (WishSpeedSq < 0.0001f)
		{
			// 입력 없음 -> 브레이킹 감속으로 즉시 정지 (칼정지)
			if (HorizSpeed > 0.001f)
			{
				const float Drop = (BrakingDeceleration + HorizSpeed * GroundFriction) * DeltaTime;
				const float NewSpeed = FMath::Max(0.0f, HorizSpeed - Drop);
				if (NewSpeed > 0.0f)
				{
					const float Scale = NewSpeed / HorizSpeed;
					Velocity.X *= Scale;
					Velocity.Y *= Scale;
				}
				else
				{
					Velocity.X = 0.0f;
					Velocity.Y = 0.0f;
				}
			}
			else
			{
				Velocity.X = 0.0f;
				Velocity.Y = 0.0f;
			}
		}
		else
		{
			// 키 입력 방향으로 신속하게 가속
			const float AccelFactor = FMath::Clamp(GroundAcceleration * DeltaTime, 0.0f, 1.0f);
			Velocity.X += (DesiredVelocity.X - Velocity.X) * AccelFactor;
			Velocity.Y += (DesiredVelocity.Y - Velocity.Y) * AccelFactor;
		}

		// 지상 유지 중 바닥 스냅 하향 속도
		if (bEnableGravity)
		{
			Velocity.Z = -2.0f;
		}
	}
	else
	{
		// 공중: 중력 적용
		if (bEnableGravity)
		{
			Velocity.Z -= Gravity * DeltaTime;
		}

		// 공중 제어 (Air Control): Quake/Source/Unreal 표준 공중 가속
		// 목표 최고 속도(TargetMaxSpeed)를 초과하여 비정상 가속되는 현상을 엄격히 방지
		if (WishSpeedSq >= 0.0001f)
		{
			const FVector HorizVel(Velocity.X, Velocity.Y, 0.0f);
			const float CurrentSpeedInDir = FVector::Dot(HorizVel, WishDir);
			const float AddSpeed = TargetMaxSpeed - CurrentSpeedInDir;

			// 이미 해당 방향 속도가 TargetMaxSpeed 이상이면 추가 가속 금지!
			if (AddSpeed > 0.0f)
			{
				float AccelSpeed = AirAcceleration * TargetMaxSpeed * DeltaTime;
				if (AccelSpeed > AddSpeed)
				{
					AccelSpeed = AddSpeed;
				}
				Velocity.X += WishDir.X * AccelSpeed;
				Velocity.Y += WishDir.Y * AccelSpeed;
			}
		}

		// 일반 점프 시 공중 수평 속도가 최대 허용 속도(SprintSpeed 또는 포탈 플링 속도)를 넘지 않도록 제한
		const float CurrentHorizSpeedSq = Velocity.X * Velocity.X + Velocity.Y * Velocity.Y;
		const float MaxAllowedSpeed = FMath::Max(TargetMaxSpeed, MaxPreservedFlingSpeed);
		if (CurrentHorizSpeedSq > (MaxAllowedSpeed * MaxAllowedSpeed))
		{
			const float CurrentHorizSpeed = sqrtf(CurrentHorizSpeedSq);
			if (CurrentHorizSpeed > 0.001f)
			{
				const float Scale = MaxAllowedSpeed / CurrentHorizSpeed;
				Velocity.X *= Scale;
				Velocity.Y *= Scale;
			}
		}
	}

	// 7. 물리 이동 및 충돌 해결 (Sweep, Step-Up & Wall Slide)
	if (GetRootComponent())
	{
		// 7-A. 수평 이동, 계단 넘기(Step-Up), 벽면 미끄러짐(Wall Slide)
		const FVector PreMoveLocation = GetActorLocation();
		APortalActor* TargetPortal = (CurrentOverlappingPortal && CurrentOverlappingPortal->IsActive()) ? CurrentOverlappingPortal : nullptr;

		const bool bInPortal = (TargetPortal != nullptr);

		bool bInWallPortal = false;
		bool bInFloorPortal = false;

		if(TargetPortal)
		{
			const FVector Normal = TargetPortal->GetPortalNormal();
			if(std::abs(Normal.Z) > 0.7f)
			{
				bInFloorPortal = true; // 바닥 또는 천장 포탈
			}
			else
			{
				bInWallPortal = true;  // 일반 벽 포탈
			}
		}

		const FVector HorizontalDelta(Velocity.X * DeltaTime, Velocity.Y * DeltaTime, 0.0f);
		if (HorizontalDelta.SizeSquared() > 1e-8f)
		{
			FHitResult Hit;
			const bool bSweepHoriz = !bInWallPortal;
			GetRootComponent()->MoveComponent(HorizontalDelta, GetActorRotation(), !bInPortal, &Hit);

			if (bSweepHoriz && Hit.bBlockingHit)
			{
				bool bSteppedUp = false;

				// 지상 상태이고 수직 계단/문턱(Hit.ImpactNormal.Z < 0.5f)을 만났을 때 Step-Up 시도
				if (bIsGrounded && MaxStepHeight > 0.0f && Hit.ImpactNormal.Z < 0.5f)
				{
					const FVector PreStepLocation = GetActorLocation();

					// 1) 위로 MaxStepHeight 만큼 올라가기
					FHitResult UpHit;
					GetRootComponent()->MoveComponent(FVector(0.0f, 0.0f, MaxStepHeight), GetActorRotation(), true, &UpHit);

					// 2) 전방으로 HorizontalDelta 만큼 전진
					FHitResult FwdHit;
					GetRootComponent()->MoveComponent(HorizontalDelta, GetActorRotation(), true, &FwdHit);

					// 3) 다시 아래로 MaxStepHeight 만큼 내려앉아 계단 윗면에 착지
					FHitResult DownHit;
					GetRootComponent()->MoveComponent(FVector(0.0f, 0.0f, -MaxStepHeight), GetActorRotation(), true, &DownHit);

					// 4) 아래로 내려앉았을 때 걸을 수 있는 바닥(ImpactNormal.Z > 0.5f)에 착지했는지 검증
					if (DownHit.bBlockingHit && DownHit.ImpactNormal.Z > 0.5f)
					{
						bSteppedUp = true;
					}
					else
					{
						// 계단이 아니거나 위가 막혀 있음 -> 원래 위치로 롤백
						GetRootComponent()->SetWorldLocation(PreStepLocation, false);
					}
				}

				// Step-Up이 아니거나 실패한 경우 일반 벽면 미끄러짐(Wall Slide) 수행
				if (!bSteppedUp)
				{
					const float RemainingTime = 1.0f - Hit.Time;
					if (RemainingTime > 0.0f)
					{
						const FVector SlideDelta = FVector::VectorPlaneProject(HorizontalDelta * RemainingTime, Hit.ImpactNormal);
						FHitResult SlideHit;
						GetRootComponent()->MoveComponent(SlideDelta, GetActorRotation(), true, &SlideHit);
					}

					// 속도 벡터를 벽면과 평행하도록 투영하되, 벽면 마찰력(WallFriction)을 적용해 감속
					const FVector ProjectedVel = FVector::VectorPlaneProject(FVector(Velocity.X, Velocity.Y, 0.0f), Hit.ImpactNormal);
					const float FrictionScale = FMath::Clamp(1.0f - WallFriction, 0.0f, 1.0f);
					Velocity.X = ProjectedVel.X * FrictionScale;
					Velocity.Y = ProjectedVel.Y * FrictionScale;
				}
			}
		}

		// 7-B. 수직 이동 및 바닥/천장 충돌 (착지 충격 반동 감지)
		const FVector VerticalDelta(0.0f, 0.0f, Velocity.Z * DeltaTime);
		if (VerticalDelta.SizeSquared() > 1e-8f)
		{
			FHitResult Hit;
			const bool bSweepVert = !bInFloorPortal;
			GetRootComponent()->MoveComponent(VerticalDelta, GetActorRotation(), bSweepVert, &Hit);

			if (bSweepVert && Hit.bBlockingHit)
			{
				if (Velocity.Z <= 0.0f && Hit.ImpactNormal.Z > 0.5f)
				{
					// 착지 충격 반동 (Landing Dip): 낙하 속도에 비례해 카메라를 아래로 살짝 쿵 내려앉힘
					const float ImpactSpeed = -Velocity.Z;
					if (ImpactSpeed > 2.0f)
					{
						const float TargetDip = FMath::Clamp((ImpactSpeed - 2.0f) * 0.025f, 0.04f, 0.2f);
						CurrentLandingDip = FMath::Max(CurrentLandingDip, TargetDip);
					}

					// 바닥 착지 성공!
					bIsGrounded = true;
					Velocity.Z = 0.0f;
				}
				else if (Velocity.Z > 0.0f && Hit.ImpactNormal.Z < -0.5f)
				{
					// 천장 충돌 시 상승 속도 중단
					Velocity.Z = 0.0f;
				}
			}
			else if(bInFloorPortal)
			{
				// 바닥 포탈 구멍에 들어섰으므로 착지 상태 해제 (낙하 가속)
				bIsGrounded = false;
			}
			else
			{
				bIsGrounded = false;
			}
		}

		if(TargetPortal && TargetPortal->IsActive() && CanTeleport())
		{
			const FVector PostMoveLocation = GetActorLocation();

			if(TargetPortal->Checkcrossing(PreMoveLocation, PostMoveLocation))
			{
				APortalActor* ExitPortal = TargetPortal->GetLinkedPortal();
				const FMatrix TelportMatrix = TargetPortal->GetTeleportMatrix();

				FVector NewLocation = TelportMatrix.TransformPosition(PostMoveLocation);
				NewLocation += ExitPortal->GetPortalNormal() * 0.10f;

				FVector NewVelocity = TelportMatrix.TransformVector(Velocity);
				
				FRotator CamWorldRot = GetActorRotation() + CameraComponent->GetRelativeRotation();
				FMatrix NewCamWorldMat = CamWorldRot.Quaternion().ToFMatrix() * TelportMatrix;
				FRotator NewCamRotation = MatrixToRotator(NewCamWorldMat);

				TeleportThroughPortal(NewLocation, NewCamRotation, NewVelocity);
				SetTeleportCooldown(0.15f);
				CurrentOverlappingPortal = nullptr;
			}
		}
	}
}

void APortalPawn::Respawn()
{
	FVector TargetLocation = InitialSpawnLocation;
	FRotator TargetRotation = InitialSpawnRotation;

	// 씬에서 PlayerStart 탐색
	if (GetWorld() && GetWorld()->GetPersistentLevel())
	{
		for (AActor* Actor : GetWorld()->GetPersistentLevel()->GetActors())
		{
			if (APlayerStart* Start = Cast<APlayerStart>(Actor))
			{
				TargetLocation = Start->GetSpawnLocation();
				TargetRotation = Start->GetSpawnRotation();
				break;
			}
		}
	}

	// 위치 및 회전 재설정
	if (GetRootComponent())
	{
		GetRootComponent()->SetWorldLocationAndRotation(TargetLocation, FRotator(0.0f, TargetRotation.Yaw, 0.0f), false);
	}
	if (CameraComponent)
	{
		CameraComponent->SetRelativeRotation(FRotator(TargetRotation.Pitch, 0.0f, 0.0f));
	}

	// 리스폰 시 포탈 설치 상태 초기화 및 크로스헤어 리셋
	ResetPortals();
	if (AUIManagerActor::Get())
	{
		AUIManagerActor::Get()->SetCrosshairState("unfilled");
	}

	// 물리 및 연출 상태 초기화
	Velocity = FVector::ZeroVector;
	PendingInputVector = FVector::ZeroVector;
	bIsGrounded = false;
	CurrentLandingDip = 0.0f;
	CurrentBobX = 0.0f;
	CurrentBobZ = 0.0f;
	BobTimer = 0.0f;
	MaxPreservedFlingSpeed = SprintSpeed;
	TeleportCooldownTimer = 0.0f;

	// 들고 있던 큐브가 있다면 내려놓기
	if (PortalGun && PortalGun->IsHoldingCube())
	{
		PortalGun->Unhold();
	}
}

void APortalPawn::AddMovementInput(const FVector& WorldDirection, float ScaleValue)
{
	PendingInputVector += WorldDirection * ScaleValue;
}

void APortalPawn::AddControllerYawInput(float Value)
{
	if (!GetRootComponent()) return;

	const float ZoomScale = (DefaultFOV > 0.0f) ? (CurrentFOV / DefaultFOV) : 1.0f;
	FRotator Rotation = GetActorRotation();
	Rotation.Yaw += Value * LookSensitivity * ZoomScale;
	GetRootComponent()->SetRelativeRotation(Rotation);
}

void APortalPawn::AddControllerPitchInput(float Value)
{
	if (!CameraComponent) return;

	const float ZoomScale = (DefaultFOV > 0.0f) ? (CurrentFOV / DefaultFOV) : 1.0f;
	FRotator CamRot = CameraComponent->GetRelativeRotation();
	CamRot.Pitch = FMath::Clamp(CamRot.Pitch + Value * LookSensitivity * ZoomScale, -89.0f, 89.0f);
	CameraComponent->SetRelativeRotation(CamRot);
}

void APortalPawn::Jump()
{
	if (bIsGrounded)
	{
		Velocity.Z = JumpVelocity;
		bIsGrounded = false;
		if (GetRootComponent())
		{
			GetRootComponent()->MoveComponent(FVector(0.0f, 0.0f, 0.05f), GetActorRotation(), false);
		}
	}
}

void APortalPawn::TeleportThroughPortal(const FVector& NewLocation, const FRotator& NewRotation, const FVector& NewVelocity)
{
	if (GetRootComponent())
	{
		GetRootComponent()->SetWorldLocationAndRotation(NewLocation, FRotator(0.0f, NewRotation.Yaw, 0.0f), false);
	}
	if (CameraComponent)
	{
		CameraComponent->SetRelativeRotation(FRotator(NewRotation.Pitch, 0.0f, NewRotation.Roll));
	}
	Velocity = NewVelocity;

	// 포탈 플링으로 획득한 고속 관성을 공중에서 보존할 수 있도록 한도 갱신
	const float FlingHorizSpeed = sqrtf(NewVelocity.X * NewVelocity.X + NewVelocity.Y * NewVelocity.Y);
	MaxPreservedFlingSpeed = FMath::Max(SprintSpeed, FlingHorizSpeed);

	if (Velocity.Z > 0.0f)
	{
		bIsGrounded = false;
	}

	// 핑퐁 재진입 방지 쿨다운 가동
	TeleportCooldownTimer = TeleportCooldownDuration;
}

void APortalPawn::PostEditChangeProperty(const FProperty& Property)
{
	Super::PostEditChangeProperty(Property);

	if (SphereComponent)
	{
		SphereComponent->SetSphereRadius(CollisionRadius);
	}
	if (CameraComponent)
	{
		CameraComponent->SetRelativeLocation({ 0.0f, 0.0f, EyeHeight });
	}
}

void APortalPawn::PrimaryAction()
{
	// 큐브를 들고 있으면 놓기 (던지기 구현 시 교체)
	if (PortalGun && PortalGun->IsHoldingCube())
	{
		PortalGun->Unhold();
		return;
	}
	FirePortal(EPortalType::Blue);
}

void APortalPawn::SecondaryAction()
{
	if (PortalGun && PortalGun->IsHoldingCube())
	{
		return;
	}
	FirePortal(EPortalType::Orange);
}

void APortalPawn::FirePortal(EPortalType Type)
{
	if (!PortalGun || !CameraComponent || !GetWorld())
	{
		return;
	}

	const FVector RayDir = CameraComponent->GetWorldRotation().Quaternion().GetForwardVector().Normalized();
	const FRay WorldRay{ CameraComponent->GetWorldLocation(), RayDir };

	// LineTraceSingle은 ImpactNormal을 채우지 않으므로 반지름 0 SweepSingle 사용
	FHitResult Hit;
	if (!GetWorld()->SweepSingle(WorldRay, 1000.0f, 0.0f, Hit, this))
	{
		return;
	}

	// 움직이는 큐브에는 포탈을 붙이지 않는다
	if (Hit.HitComponent)
	{
		if(Cast<APortalCubeActor>(Hit.HitComponent->GetOwner()) || Cast<APortalActor>(Hit.HitComponent->GetOwner()))
		{
			return;
		}
	}

	PortalGun->FirePortal(Type, Hit);
}

void APortalPawn::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, bool bFromSweep, const FHitResult& SweepResult)
{
	if(APortalActor* Portal = Cast<APortalActor>(OtherActor))
	{
		if(Portal->IsActive())
		{
			CurrentOverlappingPortal = Portal;
		}
	}
}

void APortalPawn::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp)
{
	if(OtherActor == CurrentOverlappingPortal)
	{
		CurrentOverlappingPortal = nullptr;
	}
}

void APortalPawn::TogglePortalType()
{

}

void APortalPawn::Interact()
{
	HTR_LOG(Warning, "Start Interact");

	if (!PortalGun)
	{
		HTR_LOG(Warning, "Interact Failed: PortalGun is null");
		return;
	}

	if (PortalGun->IsHoldingCube())
	{
		PortalGun->Unhold();
		return;
	}

	if (!CameraComponent || !GetWorld())
	{
		HTR_LOG(Warning, "Interact Failed: No Camera or World");
		return;
	}

	const FVector RayDir = CameraComponent->GetWorldRotation().Quaternion().GetForwardVector().Normalized();
	const FVector RayOrigin = CameraComponent->GetWorldLocation() + (RayDir * 0.5f);
	const FRay WorldRay{ RayOrigin, RayDir };

	FHitResult HitResult;
	const bool bHit = GetWorld()->LineTraceSingle(WorldRay, HitResult, nullptr, nullptr, this);

	if (bHit && HitResult.HitComponent)
	{
		AActor* HitActor = HitResult.HitComponent->GetOwner();
		if (HitActor == PortalGun)
		{
			HTR_LOG(Warning, "Interact Hit: PortalGun itself hit, ignored");
			return;
		}

		HTR_LOG(Warning, "Interact Hit: {}", HitActor ? HitActor->GetName() : "Unknown");

		const float TotalDistance = HitResult.Distance + 0.5f;
		const float MaxRange = PortalGun->GetPickUpRange();

		if (TotalDistance <= MaxRange)
		{
			if (APortalCubeActor* Cube = Cast<APortalCubeActor>(HitActor))
			{
				HTR_LOG(Warning, "Try Hold Cube: {}", Cube->GetName());
				PortalGun->Hold(Cube);
			}
			else
			{
				HTR_LOG(Warning, "Interact Hit actor is not APortalCubeActor");
			}
		}
		else
		{
			HTR_LOG(Warning, "Interact Out of Range: Dist={} > Max={}", TotalDistance, MaxRange);
		}
	}
	else
	{
		HTR_LOG(Warning, "Interact Hit nothing");
	}
}


bool APortalPawn::IsBluePortalPlaced() const
{
	if (PortalGun && PortalGun->GetPortal(EPortalType::Blue))
	{
		return PortalGun->GetPortal(EPortalType::Blue)->IsPlaced();
	}
	return false;
}

bool APortalPawn::IsOrangePortalPlaced() const
{
	if (PortalGun && PortalGun->GetPortal(EPortalType::Orange))
	{
		return PortalGun->GetPortal(EPortalType::Orange)->IsPlaced();
	}
	return false;
}

void APortalPawn::ResetPortals()
{
	if (PortalGun)
	{
		PortalGun->ResetPortals();
	}
}
