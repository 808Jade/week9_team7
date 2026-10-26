#include "EnginePCH.h"
#include "GameFramework/Actor/PortalActor.h"
#include "Components/StaticMeshComponent.h"
#include "Asset/AssetManager.h"
#include "Engine/World.h"
#include "Math/Frustum.h"
#include "Render/SceneRenderer.h"
#include "Render/RenderCommand.h"
#include "Engine/Level.h"
#include <cmath>

namespace
{
	constexpr int32 PortalViewIndex = 100;// 에디터 뷰포트 슬롯(0~3)과 겹치지 않게

	// 행 벡터 곱: V * M
	FVector4 MulRow(const FVector4& V, const FMatrix& M)
	{
		return FVector4(
			V.X * M.M[0][0] + V.Y * M.M[1][0] + V.Z * M.M[2][0] + V.W * M.M[3][0],
			V.X * M.M[0][1] + V.Y * M.M[1][1] + V.Z * M.M[2][1] + V.W * M.M[3][1],
			V.X * M.M[0][2] + V.Y * M.M[1][2] + V.Z * M.M[2][2] + V.W * M.M[3][2],
			V.X * M.M[0][3] + V.Y * M.M[1][3] + V.Z * M.M[2][3] + V.W * M.M[3][3]);
	}

	float Dot4(const FVector4& A, const FVector4& B)
	{
		return A.X * B.X + A.Y * B.Y + A.Z * B.Z + A.W * B.W;
	}

	// 뷰 공간 평면 C(보이는 쪽 >= 0)를 근평면으로 하는 투영 행렬 (D3D, z ∈ [0, w])
	FMatrix MakeObliqueProjection(const FMatrix& Proj, const FVector4& C)
	{
		const FMatrix InvProj = Proj.Inverse();

		// 클립 공간 평면 = InvProj * C (열 벡터 곱) → x, y 부호만 필요
		const float ClipX = InvProj.M[0][0] * C.X + InvProj.M[0][1] * C.Y + InvProj.M[0][2] * C.Z + InvProj.M[0][3] * C.W;
		const float ClipY = InvProj.M[1][0] * C.X + InvProj.M[1][1] * C.Y + InvProj.M[1][2] * C.Z + InvProj.M[1][3] * C.W;

		// 평면 반대편 먼 모서리를 뷰 공간으로
		const FVector4 Q = MulRow(FVector4(ClipX >= 0.0f ? 1.0f : -1.0f,
			ClipY >= 0.0f ? 1.0f : -1.0f, 1.0f, 1.0f), InvProj);

		const float Scale = 1.0f / Dot4(C, Q);

		FMatrix Out = Proj;
		Out.M[0][2] = C.X * Scale;
		Out.M[1][2] = C.Y * Scale;
		Out.M[2][2] = C.Z * Scale;
		Out.M[3][2] = C.W * Scale;
		return Out;
	}
}

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

bool APortalActor::CalcPortalSceneView(const FSceneView& MainView, FSceneView& OutView) const
{
	FPortalCameraPose Pose;
	if (!CalcVirtualCamera(MainView.ViewLocation, MainView.ViewRotation, Pose))
		return false;

	// 투영(FOV/Aspect/Near/Far/ProjectionMatrix), ViewRect, bGameView는 메인 뷰와 동일
	OutView = MainView;
	OutView.ViewIndex = PortalViewIndex;

	// 카메라 자세만 가상 카메라로 교체
	OutView.ViewLocation = Pose.Location;
	OutView.ViewRotation = Pose.Rotation;
	OutView.ViewForward = Pose.Forward;
	OutView.ViewMatrix = Pose.ViewMatrix;

	// 파생 값 재계산
	OutView.ViewProjectionMatrix = OutView.ViewMatrix * OutView.ProjectionMatrix;
	OutView.Frustum = ExtractFrustumPlanes(OutView.ViewProjectionMatrix);
	OutView.FrustumCorners = GenerateFrustumCorners(OutView.ViewProjectionMatrix.Inverse());

	// 짝 포탈 B가 있는 벽면 평면을 근평면으로
	const FVector N = LinkedPortal->GetPortalNormal();
	const FVector P = LinkedPortal->GetActorLocation();
	const FVector4 Nv = MulRow(FVector4(N.X, N.Y, N.Z, 0.0f), OutView.ViewMatrix);
	const FVector4 Pv = MulRow(FVector4(P.X, P.Y, P.Z, 1.0f), OutView.ViewMatrix);
	const FVector4 ClipPlane(Nv.X, Nv.Y, Nv.Z, -(Nv.X * Pv.X + Nv.Y * Pv.Y + Nv.Z * Pv.Z));

	// 카메라(뷰 원점)가 평면 뒤에 있을 때만 적용. 원점의 평면 값 = ClipPlane.W
	if (ClipPlane.W < -0.001f)
	{
		OutView.ProjectionMatrix = MakeObliqueProjection(OutView.ProjectionMatrix, ClipPlane);
		OutView.ViewProjectionMatrix = OutView.ViewMatrix * OutView.ProjectionMatrix;
	}

	return true;
}

void APortalActor::RenderPortalViews(UWorld* World, FRenderer* Renderer, FSkyboxRenderer* Skybox,
	const FSceneView& MainView, const FSceneRenderOptions& MainOptions)
{
	if (!World || !Renderer || !World->GetPersistentLevel()) return;

	FSceneRenderOptions Options = MainOptions;
	Options.bEnableFog = false;
	Options.bEnableFXAA = false;
	Options.bShowSceneDepth = false;
	Options.bShowCascade = false;
	Options.bShowWorldNormal = false;
	Options.bShowLightCulling = false;

	FGPUOcclusionSettings& Occlusion = Renderer->GetGPUOcclusion().GetSettings();
	const bool bPrevOcclusion = Occlusion.bEnabled;
	Occlusion.bEnabled = false;

	const uint32 Width = static_cast<uint32>(MainView.ViewRect.Width);
	const uint32 Height = static_cast<uint32>(MainView.ViewRect.Height);

	for (AActor* Actor : World->GetPersistentLevel()->GetActors())
	{
		APortalActor* Portal = Cast<APortalActor>(Actor);
		if (!Portal || !Portal->IsActive()) continue;
		if (!Portal->EnsureRenderTarget(Width, Height)) continue;

		FSceneView PortalView;
		if (!Portal->CalcPortalSceneView(MainView, PortalView)) continue;

		FSceneRenderer SceneRenderer(World, PortalView, Portal->GetRenderData());
		SceneRenderer.InitViews(Renderer, Options);

		FRenderGraphBuilder Graph;
		FScenePassCallbacks Callbacks;   // 파이어볼·안개·텍스트·에디터 오버레이 없음
		if (SceneRenderer.AddViewportPasses(Graph, Renderer, Portal->GetViewTargets(), Options, Skybox, Callbacks))
			Graph.Execute();
		RenderCommand::EndRenderPass();
	}

	Occlusion.bEnabled = bPrevOcclusion;
}