#pragma once

#include "GameFramework/Actor.h"
#include "Render/SceneViewTargets.h"
#include "Render/SceneRenderData.h"

class UStaticMeshComponent;

// FSceneView를 채울 때 사용
struct FPortalCameraPose
{
	FVector  Location;
	FRotator Rotation;
	FVector  Forward;
	FMatrix  ViewMatrix;
};

class APortalActor : public AActor
{
	DECLARE_CLASS(APortalActor, AActor)
public:
	APortalActor();

	// 게임 시작 시 1번 호출: 2개 스폰 + 서로 연결 + 숨김 상태
	static void SpawnPair(UWorld* World, APortalActor*& OutBlue, APortalActor*& OutOrange);

	// 포탈건이 맞힌 지점/법선으로 이동
	void PlaceAt(const FVector& Location, const FVector& Normal);

	// 나와 상대가 모두 설치됐을 때만 건너편을 그린다
	bool IsActive() const { return bPlaced && LinkedPortal && LinkedPortal->bPlaced; }

	APortalActor* GetLinkedPortal() const { return LinkedPortal; }
	UStaticMeshComponent* GetPortalMesh() const { return PortalMesh; }
	FVector GetPortalNormal() const { return GetActorQuat().GetForwardVector(); }

	// 렌더링 직전에 매 프레임 호출. 크기가 같으면 아무 일도 안 한다.
	bool EnsureRenderTarget(uint32 Width, uint32 Height) { return ViewTargets.Resize(Width, Height); }

	FSceneViewTargets& GetViewTargets() { return ViewTargets; }
	FSceneRenderData& GetRenderData() { return RenderData; }

	// 포탈 면 셰이더가 읽을 텍스처
	FTexture2D* GetPortalTexture() const { return ViewTargets.SceneColor.get(); }

	// 플레이어 카메라 → 이 포탈을 통해 본 가상 카메라
	bool CalcVirtualCamera(const FVector& CamLocation, const FRotator& CamRotation, FPortalCameraPose& Out) const;

	// 스케일 없는 포탈 월드 행렬 (회전 + 위치)
	FMatrix GetPortalMatrix() const;

private:
	UStaticMeshComponent* PortalMesh = nullptr;
	APortalActor* LinkedPortal = nullptr;
	bool bPlaced = false;

	FSceneViewTargets ViewTargets;   // 포탈 카메라가 그린 결과 (Color + Depth)
	FSceneRenderData  RenderData;    // 포탈 뷰 전용 렌더 큐/그림자 캐시

	float PortalWidth = 1.0f;
	float PortalHeight = 2.0f;
};