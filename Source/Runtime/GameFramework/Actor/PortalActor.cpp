#include "EnginePCH.h"
#include "GameFramework/Actor/PortalActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Asset/AssetManager.h"
#include "Engine/World.h"
#include "Math/Frustum.h"
#include "Render/SceneRenderer.h"
#include "Render/RenderCommand.h"
#include "Render/PortalSurfaceRenderer.h"
#include "Engine/Level.h"
#include "Collision/Ray.h"
#include "Collision/HitResult.h"
#include "GameFramework/Pawn.h"
#include <cmath>

namespace
{
	constexpr int32 PortalViewIndex = 100;// 에디터 뷰포트 슬롯(0~3)과 겹치지 않게

	// 벽면에서 포탈 면을 법선 방향으로 띄우는 거리 (m).
	constexpr float PortalSurfaceOffset = 0.01f;

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
	SetActorEnableCollision(true);   // 포탈건 레이가 기존 포탈에 맞지 않고 뒤의 벽에 맞도록

	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>("PortalMeshComponent");
	PortalMesh->SetupAttachment(GetRootComponent());
	PortalMesh->SetStaticMesh(UAssetManager::GetAssetByPath<UStaticMesh>("Plane"));
	PortalMesh->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f)); // 법선 +Z → +X
	PortalMesh->SetRelativeScale3D(FVector(PortalHeight, PortalWidth, 1.0f));
	PortalMesh->SetVisibility(false);   // 일반 경로에선 영구히 안 그림

	PortalTrigger = CreateDefaultSubobject<UBoxComponent>("PortalTrigger");
	PortalTrigger->SetupAttachment(GetRootComponent());
	PortalTrigger->SetBoxExtent(FVector(1.0f, PortalWidth * 0.5f, PortalHeight * 0.5f));
	PortalTrigger->SetCurrentCollisionResponse(ECollisionResponse::Overlap);
}

void APortalActor::SpawnPair(UWorld* World, APortalActor*& OutBlue, APortalActor*& OutOrange)
{
	OutBlue = World->SpawnActor<APortalActor>();
	OutOrange = World->SpawnActor<APortalActor>();
	OutBlue->LinkedPortal = OutOrange;
	OutOrange->LinkedPortal = OutBlue;
	OutBlue->PortalColor = FVector4(0.10f, 0.45f, 1.00f, 1.0f);   // 추가
	OutOrange->PortalColor = FVector4(1.00f, 0.50f, 0.10f, 1.0f);   // 추가
}

void APortalActor::PlaceAt(const FVector& Location, const FVector& Normal)
{
	const float Yaw = atan2f(Normal.Y, Normal.X) * (180.0f / PI);
	const float Pitch = -asinf(FMath::Clamp(Normal.Z, -1.0f, 1.0f)) * (180.0f / PI);

	SetActorLocation(Location + Normal * PortalSurfaceOffset);   // 벽과 Z-fighting 방지
	SetActorRotation(FRotator(Pitch, Yaw, 0.0f));
	bPlaced = true;
}

FMatrix APortalActor::GetPortalMatrix() const
{
	FMatrix M = GetActorQuat().ToFMatrix();
	M.SetOrigin(GetActorLocation());
	return M;
}

FMatrix APortalActor::GetSurfaceMatrix() const
{
	// PortalMesh->GetWorldMatrix()는 부모 * 자식 회전을 FRotator로 되돌리는데,
	// 면을 세우는 Pitch 90° 때문에 결과가 짐벌락(Pitch = ±90°)에 걸려 Yaw/Roll이 atan2(0, 0) 노이즈가 된다.
	// → 특정 방향 벽에서 면이 틀어져 벽에 반쯤 묻힘. 로컬 행렬 * 포탈 행렬로 직접 곱해서 피한다.
	const FMatrix Local = FTransform(FRotator(90.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, 0.0f),
		FVector(PortalHeight, PortalWidth, 1.0f)).GetLocalMatrix();   // 법선 +Z → +X, 크기
	return Local * GetPortalMatrix();   // 행 벡터: 로컬 → 포탈 월드
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
	OutView.bRenderUI = false;

	// 카메라 자세만 가상 카메라로 교체
	OutView.ViewLocation = Pose.Location;
	OutView.ViewRotation = Pose.Rotation;
	OutView.ViewForward = Pose.Forward;
	OutView.ViewMatrix = Pose.ViewMatrix;

	OutView.ViewProjectionMatrix = OutView.ViewMatrix * OutView.ProjectionMatrix;

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

	OutView.Frustum = ExtractFrustumPlanes(OutView.ViewProjectionMatrix);
	OutView.FrustumCorners = GenerateFrustumCorners(OutView.ViewProjectionMatrix.Inverse());

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

	const uint32 Width = FMath::Max(1u, static_cast<uint32>(MainView.ViewRect.Width * PortalResolutionScale));
	const uint32 Height = FMath::Max(1u, static_cast<uint32>(MainView.ViewRect.Height * PortalResolutionScale));

	// 포탈 뷰에서는 포탈 면을 모두 제외
	TArray<const UPrimitiveComponent*> HiddenPortalMeshes;
	for (AActor* Actor : World->GetPersistentLevel()->GetActors())
		if (APortalActor* Portal = Cast<APortalActor>(Actor))
			HiddenPortalMeshes.Add(Portal->GetPortalMesh());

	for (AActor* Actor : World->GetPersistentLevel()->GetActors())
	{
		APortalActor* Portal = Cast<APortalActor>(Actor);
		if (!Portal || !Portal->IsActive()) continue;

		if (Portal->GetDistanceToPlane(MainView.ViewLocation) < -0.05f)
		{
			continue;
		}

		const float HalfWidth = Portal->GetPortalWidth() * 0.5f;
		const float HalfHeight = Portal->GetPortalHeight() * 0.5f;
		const float Radius = sqrtf(HalfWidth * HalfWidth + HalfHeight * HalfHeight) + 0.1f;
		const FAABB PortalBounds{ Portal->GetActorLocation(), FVector(Radius, Radius, Radius) };
		if (!IsAABBInFrustum(PortalBounds, MainView.Frustum))
		{
			continue;
		}

		if (!Portal->EnsureRenderTarget(Width, Height)) continue;

		FSceneView PortalView;
		if (!Portal->CalcPortalSceneView(MainView, PortalView)) continue;
		PortalView.ViewRect = FRect(0.0f, 0.0f, static_cast<float>(Width), static_cast<float>(Height));
		PortalView.bRenderUI = false;
		PortalView.HiddenComponents = &HiddenPortalMeshes;

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

void APortalActor::DrawPortalSurfaces(UWorld* World, const FSceneView& MainView)
{
	if (!World || !World->GetPersistentLevel()) return;

	for (AActor* Actor : World->GetPersistentLevel()->GetActors())
	{
		APortalActor* Portal = Cast<APortalActor>(Actor);
		if (!Portal || !Portal->bPlaced) continue;

		FTexture2D* Texture = Portal->IsActive() ? Portal->GetPortalTexture() : nullptr;
		FPortalSurfaceRenderer::Get().Draw(
			Portal->PortalMesh->GetStaticMesh(),
			Portal->GetSurfaceMatrix(),
			MainView.ViewProjectionMatrix,
			Texture,
			Portal->PortalColor,
			PortalResolutionScale);
	}
}

FMatrix APortalActor::GetTeleportMatrix() const
{
	if(!LinkedPortal)
	{
		return FMatrix::Identity;
	}

	static const FMatrix Flip(
		-1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);

	return GetPortalMatrix().Inverse() * Flip * LinkedPortal->GetPortalMatrix();
}

bool APortalActor::IsPointWithinPortalBounds(const FVector& Point, float Margin) const
{
	const FVector Local = GetPortalMatrix().Inverse().TransformPosition(Point);

	return (std::abs(Local.Y) <= (PortalWidth * 0.5f + Margin)) &&
		(std::abs(Local.Z) <= (PortalHeight * 0.5f + Margin));
}

bool APortalActor::Checkcrossing(const FVector& PrevPos, const FVector& CurPos, float Margin) const
{
	if(!IsActive())
	{
		return false;
	}

	const float DPrev = GetDistanceToPlane(PrevPos);
	const float DCur = GetDistanceToPlane(CurPos);

	if(DPrev > 0.0f && DCur <= 0.0f)
	{
		// 평면과 교차한 지점 계산
		const float T = DPrev / (DPrev - DCur);
		const FVector IntersectPoint = PrevPos + (CurPos - PrevPos) * T;
		return IsPointWithinPortalBounds(IntersectPoint, Margin);
	}

	return false;
}

float APortalActor::GetDistanceToPlane(const FVector& Point) const
{
	const FVector Normal = GetPortalNormal();
	return FVector::Dot(Point - GetActorLocation(), Normal);
}

bool APortalActor::PlaceAt(const FHitResult& InHitResult, const FVector& ViewerForward)
{
	if (!InHitResult.bBlockingHit)
	{
		HTR_LOG(Warning, "PlaceAt Failed: No blocking hit");
		return false;
	}

	if (!IsPortableSurface(InHitResult.HitComponent))
	{
		HTR_LOG(Warning, "PlaceAt Failed: Non-portable surface");
		return false;
	}

	const FVector SurfaceNormal = InHitResult.ImpactNormal;
	FVector TargetLocation = InHitResult.ImpactPoint;
	FRotator TargetRotation;

	if (!CalculatePortalRotation(SurfaceNormal, ViewerForward, TargetRotation))
	{
		HTR_LOG(Warning, "PlaceAt Failed: CalculatePortalRotation");
		return false;
	}

	if (!AdjustLocationForEdges(TargetLocation, TargetRotation, SurfaceNormal))
	{
		HTR_LOG(Warning, "PlaceAt Failed: AdjustLocationForEdges");
		return false;
	}

	if (IsOverlappingLinkedPortal(TargetLocation, SurfaceNormal))
	{
		HTR_LOG(Warning, "PlaceAt Failed: IsOverlappingLinkedPortal");
		return false;
	}

	if (!HasEntryClearance(TargetLocation, TargetRotation, SurfaceNormal))
	{
		HTR_LOG(Warning, "PlaceAt Failed: HasEntryClearance");
		return false;
	}

	SetActorLocation(TargetLocation + SurfaceNormal * PortalSurfaceOffset);   // 벽과 Z-fighting 방지
	SetActorRotation(TargetRotation);
	bPlaced = true;
	HTR_LOG(Warning, "PlaceAt Success!");

	return true;
}

bool APortalActor::IsPortableSurface(UPrimitiveComponent* HitComponent) const
{
	// 이름이 "Tile_P_"로 시작하는 액터에만 포탈을 붙일 수 있다
	if (HitComponent)
	{
		if (AActor* HitOwner = HitComponent->GetOwner())
		{
			if (HitOwner->GetName().rfind("Tile_P_", 0) == 0)
			{
				return true;
			}
		}
	}
	return false;
}

bool APortalActor::CalculatePortalRotation(const FVector& SurfaceNormal, const FVector& ViewerForward, FRotator& OutRotation) const
{
	const FVector Forward = SurfaceNormal.GetSafeNormal();

	// 바닥 또는 천장 (거의 수평인 면만). 0.707(=45도)이면 45도 경사 패널이 바닥으로 분류돼
	// 포탈이 눕혀지고 모서리 검사에서 실패하므로, 약 25도 이내로 기울어진 면만 바닥/천장으로 본다.
	if (std::abs(Forward.Z) >= 0.9f)
	{
		float ViewerYaw = atan2f(ViewerForward.Y, ViewerForward.X) * (180.0f / PI);
		if (std::isnan(ViewerYaw))
		{
			ViewerYaw = 0.0f;
		}

		float Pitch = 0.0f;
		float Yaw = 0.0f;

		if (Forward.Z > 0.0f) // 바닥 (Normal = +Z)
		{
			Pitch = -90.0f;
			Yaw = ViewerYaw + 180.0f;
		}
		else // 천장 (Normal = -Z)
		{
			Pitch = 90.0f;
			Yaw = ViewerYaw;
		}

		while (Yaw > 180.0f) Yaw -= 360.0f;
		while (Yaw < -180.0f) Yaw += 360.0f;

		OutRotation = FRotator(Pitch, Yaw, 0.0f);
	}
	else
	{
		// 수직/경사 벽면
		const float Yaw = atan2f(Forward.Y, Forward.X) * (180.0f / PI);
		const float Pitch = -asinf(FMath::Clamp(Forward.Z, -1.0f, 1.0f)) * (180.0f / PI);
		OutRotation = FRotator(Pitch, Yaw, 0.0f);
	}

	return true;
}

bool APortalActor::AdjustLocationForEdges(FVector& InOutLocation, const FRotator& Rotation, const FVector& SurfaceNormal) const
{
	if (!GetWorld())
	{
		return true;
	}

	const FQuat Quat = Rotation.Quaternion();
	const FVector Right = Quat.GetRightVector();
	const FVector Up = Quat.GetUpVector();

	const float HalfWidth = PortalWidth * 0.5f;
	const float HalfHeight = PortalHeight * 0.5f;
	const float EdgeMargin = 0.01f; // 1cm 마진

	FVector CurrentLocation = InOutLocation;
	const int32 MaxIterations = 3;

	for (int32 Iter = 0; Iter < MaxIterations; ++Iter)
	{
		FVector PushOffset = FVector::ZeroVector;
		bool bHitBottom = false;
		bool bHitTop = false;
		bool bHitLeft = false;
		bool bHitRight = false;

		// 1. 인접 면(바닥, 천장, 측면 벽) 관통/묻힘 검사 (Boundary Push)
		struct FEdgeCheck
		{
			FVector Dir;
			float RequiredDist;
			bool* OutHitFlag;
		};

		const FEdgeCheck Edges[4] = {
			{ -Up, HalfHeight, &bHitBottom },    // 0: 하단 (바닥면)
			{ Up, HalfHeight, &bHitTop },        // 1: 상단 (천장면)
			{ -Right, HalfWidth, &bHitLeft },    // 2: 좌측 벽
			{ Right, HalfWidth, &bHitRight }     // 3: 우측 벽
		};

		for (int32 i = 0; i < 4; ++i)
		{
			// 표면에서 5cm 띄운 위치에서 에지 방향으로 두께 없는 선분(Radius=0) 레이 발사
			const FVector Start = CurrentLocation + SurfaceNormal * 0.05f;
			const FRay EdgeRay{ Start, Edges[i].Dir };
			FHitResult Hit;

			if (GetWorld()->SweepSingle(EdgeRay, Edges[i].RequiredDist, 0.0f, Hit, const_cast<APortalActor*>(this)))
			{
				AActor* HitActor = Hit.HitComponent ? Hit.HitComponent->GetOwner() : nullptr;
				if (HitActor && HitActor != this && HitActor != LinkedPortal && !HitActor->IsA<APawn>())
				{
					// 자신이 붙어 있는 뒷벽 자체에 긁힌 경우 무시 (인접면은 법선이 서로 직교해야 함)
					if (FVector::Dot(Hit.ImpactNormal, SurfaceNormal) > 0.5f)
					{
						continue;
					}

					const float ProjectedDist = FVector::Dot(Hit.ImpactPoint - Start, Edges[i].Dir);
					if (ProjectedDist > 0.0f && ProjectedDist < Edges[i].RequiredDist)
					{
						const float Penetration = (Edges[i].RequiredDist - ProjectedDist) + EdgeMargin;
						PushOffset += (-Edges[i].Dir) * Penetration;
						*Edges[i].OutHitFlag = true;
					}
				}
			}
		}

		if (bHitBottom && bHitTop)
		{
			HTR_LOG(Warning, "AdjustLocationForEdges: Height clearance too narrow");
			return false;
		}

		// 2. 모서리가 벽을 벗어나 허공에 뜨는지 검사 (Overhang Check)
		const FVector CornerOffsets[4] = {
			Right * (HalfWidth * 0.95f) + Up * (HalfHeight * 0.95f),   // 0: Top-Right
			-Right * (HalfWidth * 0.95f) + Up * (HalfHeight * 0.95f),  // 1: Top-Left
			-Right * (HalfWidth * 0.95f) - Up * (HalfHeight * 0.95f),  // 2: Bottom-Left
			Right * (HalfWidth * 0.95f) - Up * (HalfHeight * 0.95f)    // 3: Bottom-Right
		};

		int32 MissingCornerCount = 0;
		for (int32 i = 0; i < 4; ++i)
		{
			if (bHitBottom && (i == 2 || i == 3))
			{
				continue;
			}
			if (bHitTop && (i == 0 || i == 1))
			{
				continue;
			}
			if (bHitLeft && (i == 1 || i == 2))
			{
				continue;
			}
			if (bHitRight && (i == 0 || i == 3))
			{
				continue;
			}

			const FVector CornerPos = CurrentLocation + CornerOffsets[i];
			const FRay BackRay{ CornerPos + SurfaceNormal * 0.1f, -SurfaceNormal };
			FHitResult Hit;

			if (!GetWorld()->SweepSingle(BackRay, 0.3f, 0.0f, Hit, const_cast<APortalActor*>(this)))
			{
				MissingCornerCount++;
				PushOffset -= CornerOffsets[i].GetSafeNormal() * 0.05f;
			}
		}

		if (MissingCornerCount >= 4)
		{
			HTR_LOG(Warning, "AdjustLocationForEdges: All corners in empty air");
			return false;
		}

		if (PushOffset.SizeSquared() < 1e-4f)
		{
			InOutLocation = CurrentLocation;
			return true;
		}

		CurrentLocation += PushOffset;

		if (FVector::Distance(CurrentLocation, InOutLocation) > HalfHeight * 1.2f)
		{
			HTR_LOG(Warning, "AdjustLocationForEdges: Pushed too far");
			return false;
		}
	}

	InOutLocation = CurrentLocation;
	return true;
}

bool APortalActor::IsOverlappingLinkedPortal(const FVector& DesiredLocation, const FVector& SurfaceNormal) const
{
	if (!LinkedPortal || !LinkedPortal->bPlaced)
	{
		return false;
	}

	const FVector LinkedNormal = LinkedPortal->GetPortalNormal();
	if (FVector::Dot(SurfaceNormal, LinkedNormal) < 0.95f)
	{
		return false;
	}

	const FVector LinkedLoc = LinkedPortal->GetActorLocation();
	const float PlaneDist = std::abs(FVector::Dot(DesiredLocation - LinkedLoc, SurfaceNormal));
	if (PlaneDist > 0.1f)
	{
		return false;
	}

	const FVector Delta = DesiredLocation - LinkedLoc;
	const FVector LinkedRight = LinkedPortal->GetActorQuat().GetRightVector();
	const FVector LinkedUp = LinkedPortal->GetActorQuat().GetUpVector();

	const float DistRight = std::abs(FVector::Dot(Delta, LinkedRight));
	const float DistUp = std::abs(FVector::Dot(Delta, LinkedUp));

	if (DistRight < PortalWidth && DistUp < PortalHeight)
	{
		return true;
	}

	return false;
}

bool APortalActor::HasEntryClearance(const FVector& Location, const FRotator& Rotation, const FVector& SurfaceNormal) const
{
	if (!GetWorld())
	{
		return true;
	}

	const FQuat Quat = Rotation.Quaternion();
	const FVector Forward = Quat.GetForwardVector();
	const FVector Up = Quat.GetUpVector();

	const float CheckDistance = 0.5f;
	const FVector StartPoints[3] = {
		Location + SurfaceNormal * 0.05f,
		Location + SurfaceNormal * 0.05f + Up * (PortalHeight * 0.35f),
		Location + SurfaceNormal * 0.05f - Up * (PortalHeight * 0.35f)
	};

	for (int32 i = 0; i < 3; ++i)
	{
		const FRay Ray{ StartPoints[i], Forward };
		FHitResult Hit;
		if (GetWorld()->SweepSingle(Ray, CheckDistance, 0.0f, Hit, const_cast<APortalActor*>(this)))
		{
			AActor* HitActor = Hit.HitComponent ? Hit.HitComponent->GetOwner() : nullptr;
			if (HitActor && HitActor != this && HitActor != LinkedPortal && !HitActor->IsA<APawn>())
			{
				if (FVector::Dot(Hit.ImpactNormal, SurfaceNormal) > 0.7f)
				{
					continue;
				}
				return false;
			}
		}
	}

	return true;
}
