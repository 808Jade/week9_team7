#include "EnginePCH.h"
#include "GameFramework/Actor/PortalActor.h"
#include "Components/StaticMeshComponent.h"
#include "Asset/AssetManager.h"
#include "Engine/World.h"
#include <cmath>

APortalActor::APortalActor()
{
	SetActorEnableCollision(false);   // 포탈건 레이가 기존 포탈에 맞지 않고 뒤의 벽에 맞도록

	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>("PortalMeshComponent");
	PortalMesh->SetupAttachment(GetRootComponent());
	PortalMesh->SetStaticMesh(UAssetManager::GetAssetByPath<UStaticMesh>("Plane"));
	PortalMesh->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f)); // 법선 +Z → +X
	PortalMesh->SetRelativeScale3D(FVector(PortalHeight, PortalWidth, 1.0f));
	PortalMesh->SetHiddenInGame(true); // 처음엔 숨김
}

void APortalActor::SpawnPair(UWorld* World, APortalActor*& OutBlue, APortalActor*& OutOrange)
{
	OutBlue = World->SpawnActor<APortalActor>();
	OutOrange = World->SpawnActor<APortalActor>();
	OutBlue->LinkedPortal = OutOrange;
	OutOrange->LinkedPortal = OutBlue;
}

void APortalActor::PlaceAt(const FVector& Location, const FVector& Normal)
{
	const float Yaw = atan2f(Normal.Y, Normal.X) * (180.0f / PI);
	const float Pitch = -asinf(FMath::Clamp(Normal.Z, -1.0f, 1.0f)) * (180.0f / PI);

	SetActorLocation(Location + Normal * 0.01f);   // 벽과 Z-fighting 방지
	SetActorRotation(FRotator(Pitch, Yaw, 0.0f));
	PortalMesh->SetHiddenInGame(false);
	bPlaced = true;
}

FMatrix APortalActor::GetPortalMatrix() const
{
	FMatrix M = GetActorQuat().ToFMatrix();
	M.SetOrigin(GetActorLocation());
	return M;
}

bool APortalActor::CalcVirtualCamera(const FVector& CamLocation, const FRotator& CamRotation,
	FPortalCameraPose& Out) const
{
	if (!LinkedPortal) return false;

	FMatrix CameraWorld = CamRotation.Quaternion().ToFMatrix();
	CameraWorld.SetOrigin(CamLocation);

	// Z축 180° 회전: 포탈 앞 ↔ 뒤, 들어가는 방향 ↔ 나오는 방향
	static const FMatrix Flip(
		-1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);

	// 행 벡터: 왼쪽부터 적용 (카메라 → A 로컬 → 뒤집기 → B 월드)
	const FMatrix VirtualWorld =
		CameraWorld * GetPortalMatrix().Inverse() * Flip * LinkedPortal->GetPortalMatrix();

	Out.Location = FVector(VirtualWorld.M[3][0], VirtualWorld.M[3][1], VirtualWorld.M[3][2]);
	Out.Forward = FVector(VirtualWorld.M[0][0], VirtualWorld.M[0][1], VirtualWorld.M[0][2]);
	Out.Rotation = MatrixToRotator(VirtualWorld);
	Out.ViewMatrix = VirtualWorld.Inverse();
	return true;
}