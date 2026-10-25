#pragma once

#include "Collision/Ray.h"
#include "Math/Transform.h"
#include "Components/SceneComponent.h"
#include "Components/LocalLightComponent.h"
#include "Containers/Array.h"
#include <functional>

class UCameraComponent;

enum class EGizmoMode
{
	Location,
	Rotation,
	Scale,
};

enum class EGizmoSpace
{
	Local,
	World,
};

class FGizmo   // 상태 + 로직
{
public:
	// Gizmo 대상 설정
	void SetTarget(USceneComponent* InTarget, bool InbUseActorBounds = false)
	{ 
		Target = InTarget; 
		bUseActorBounds = InbUseActorBounds; 
	}
	USceneComponent* GetTarget() const { return Target; }

	void SetAdditionalTargets(const TArray<USceneComponent*>& InTargets) { AdditionalTargets = InTargets; }
	const TArray<USceneComponent*>& GetAdditionalTargets() const { return AdditionalTargets; }

	// Gizmo 모드(이동, 회전, 크기)
	void SetMode(EGizmoMode InMode) { Mode = InMode; }
	EGizmoMode GetMode() const { return Mode; }

	void SetSpace(EGizmoSpace InSpace) { Space = InSpace; }
	EGizmoSpace GetSpace() const { return Space; }
	// Grid Snapping
	void SetGridSnapEnabled(bool bEnable) { bGridSnapEnabled = bEnable; }
	bool IsGridSnapEnabled() const { return bGridSnapEnabled; }

	void SetGridSnapSize(float InSize) { GridSnapSize = InSize; }
	float GetGridSnapSize() const { return GridSnapSize; }

	void SetAbsoluteGridSnap(bool bAbsolute) { bAbsoluteGridSnap = bAbsolute; }
	bool IsAbsoluteGridSnap() const { return bAbsoluteGridSnap; }

	// Rotation Snapping
	void SetRotationSnapEnabled(bool bEnable) { bRotationSnapEnabled = bEnable; }
	bool IsRotationSnapEnabled() const { return bRotationSnapEnabled; }

	void SetRotationSnapAngle(float InDegrees) { RotationSnapAngle = InDegrees; }
	float GetRotationSnapAngle() const { return RotationSnapAngle; }

	// Scale Snapping
	void SetScaleSnapEnabled(bool bEnable) { bScaleSnapEnabled = bEnable; }
	bool IsScaleSnapEnabled() const { return bScaleSnapEnabled; }

	void SetScaleSnapStep(float InStep) { ScaleSnapStep = InStep; }
	float GetScaleSnapStep() const { return ScaleSnapStep; }

	using FOnTransformCommitted = std::function<void(const TArray<USceneComponent*>& Targets, const TArray<FTransform>& PreTransforms, const TArray<FTransform>& PostTransforms)>;
	void SetOnTransformCommitted(FOnTransformCommitted InCallback) { OnTransformCommitted = InCallback; }

	// Active View의 Ray와 행렬만 사용해 Hover·Drag 상태를 갱신한다.
	void Update(const FRay& MouseRay, const FVector2& MousePos,
		const FMatrix& ViewProj, int ScreenW, int ScreenH,
		bool bMouseDown, const FVector& CameraLocation, bool bCameraOrthographic);

	bool IsUsing() const { return DraggingAxis >= 0; }
	int GetHoveredAxis() const { return HoveredAxis; }

	int PickAxis(const FVector2& MousePos, const FMatrix& ViewProj, int ScreenW, int ScreenH);

	int PickLinearAxis(const FVector2& MousePos, const FMatrix& ViewProj, int ScreenW, int ScreenH);
	int PickRotationAxis(const FVector2& MousePos, const FMatrix& ViewProj, int ScreenW, int ScreenH);


	void BeginDrag(int Axis, const FRay& MouseRay, const FVector2& MousePos);
	void UpdateDrag(const FRay& MouseRay, const FVector2& MousePos);
	void EndDrag();

	float ComputeAngleOnPlane(const FVector& Point, int Axis) const;

	inline FTransform GetTransform() const { return Target ? Target->GetTransform() : FTransform(); }
	inline FVector GetLocation() const { return Target ? Target->GetRelativeLocation() : FVector(0, 0, 0); }
	inline FRotator GetRotation() const { return Target ? Target->GetRelativeRotation() : FRotator(0, 0, 0); }
	inline FVector GetScale() const { return Target ? Target->GetRelativeScale3D() : FVector(0, 0, 0); }

	FVector GetRenderLocation() const;
	// 원근 View에서만 카메라 거리에 맞춰 크기 보정 위치를 구하고 직교 View는 월드 위치를 유지한다.
	FVector GetRenderLocationForView(const FVector& CameraLocation, bool bCameraOrthographic) const;
	FVector GetCameraLocation() const;
	FVector GetAxisDirection(int Axis) const;
private:
	EGizmoMode Mode = EGizmoMode::Location;
	EGizmoSpace Space = EGizmoSpace::Local;
	FTransform Transform = FTransform();
	USceneComponent* Target = nullptr;
	TArray<USceneComponent*> AdditionalTargets;
	TArray<FVector> AdditionalStartLocations;
	bool bUseActorBounds = false;

	int HoveredAxis = -1;
	int DraggingAxis = -1;

	FVector2 DragStartMousePos;
	FVector DragStartPoint;
	FVector DragStartLocation;
	FRotator DragStartRotation;
	FVector DragStartScale;
	FVector DragAxisDirection;
	FVector DragStartRenderLocation;

	FVector DragPlaneNormal;

	FVector ViewCameraLocation{};
	bool bViewCameraOrthographic = false;

	float DragStartAngle;
	float RingRadius = 1.0f;

	// Light Component인 경우 Radius를 증감하기 위한 멤버변수와 함수
	float DragStartRadius = 0.0f;
	bool ApplyScaleToLight(float Factor);
	ULocalLightComponent* GetTargetLight() const;

	bool bGridSnapEnabled = true;
	float GridSnapSize = 1.0f;
	bool bAbsoluteGridSnap = true;

	bool bRotationSnapEnabled = true;
	float RotationSnapAngle = 15.0f;

	bool bScaleSnapEnabled = false;
	float ScaleSnapStep = 0.25f;

	TArray<FTransform> DragStartTransforms;
	FOnTransformCommitted OnTransformCommitted;
};
