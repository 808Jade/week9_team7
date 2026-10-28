#pragma once

#include "Core/Types.h"
#include "Math/Vector.h"
#include "Math/Rotator.h"
#include "Math/Matrix.h"
#include "Math/Frustum.h"
#include "Math/Rect.h"
#include "Asset/LOD/StaticMeshLODSelector.h"
#include <algorithm>

// Immutable description of one camera view for the duration of a render.
struct FSceneView
{
	int32 ViewIndex = 0;
	FVector ViewLocation{};
	FRotator ViewRotation{};
	FVector ViewForward{};
	FMatrix ViewMatrix = FMatrix::Identity;
	FMatrix ProjectionMatrix = FMatrix::Identity;
	FMatrix ViewProjectionMatrix = FMatrix::Identity;
	FFrustumPlanes Frustum{};
	FFrustumCorners FrustumCorners;
	FRect ViewRect{};
	float FieldOfView = 60.0f;
	float AspectRatio = 16.0f / 9.0f;
	float NearClip = 0.1f;
	float FarClip = 10000.0f;
	float OrthoWidth = 10.0f;
	bool bIsPerspective = true;
	bool bIsWireframe = false;
	bool bGameView = false;
	float LetterBoxAmount = 0.0f;   // 0 = 없음, 1 = 띠가 완전히 내려온 상태
	float LetterBoxAspect = 2.39f;  // 띠를 제외하고 남는 영역의 목표 화면비
	const TArray<const UPrimitiveComponent*>* HiddenComponents = nullptr;

	FViewContext ToViewContext() const
	{
		FViewContext Context{};
		Context.ViewIndex = ViewIndex;
		Context.Width = static_cast<uint32>(ViewRect.Width);
		Context.Height = static_cast<uint32>(ViewRect.Height);
		Context.CameraPosition = ViewLocation;
		Context.CameraForward = ViewForward;
		Context.ViewProjection = ViewProjectionMatrix;
		Context.NearZ = NearClip;
		Context.bOrthographic = !bIsPerspective;
		Context.bGameView = bGameView;
		Context.HiddenComponents = HiddenComponents;
		const float ScaleX = ProjectionMatrix.M[1][0];
		const float ScaleY = ProjectionMatrix.M[2][1];
		Context.ProjectionScaleSquared = std::max(ScaleX * ScaleX, ScaleY * ScaleY);
		Context.Prepare();
		return Context;
	}

	FViewContext ToLODViewContext() const
	{
		return ToViewContext();
	}
};
