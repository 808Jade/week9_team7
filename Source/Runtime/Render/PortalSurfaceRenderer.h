#pragma once

#include "Math/Matrix.h"
#include "Math/Vector4.h"
#include "Render/Buffer.h"
#include "Render/PipelineState.h"

class UStaticMesh;
class FTexture2D;

struct alignas(16) FPortalSurfaceConstants
{
	FMatrix  World;
	FMatrix  ViewProjection;
	FVector4 PortalColor;
	FVector4 Params;
};
static_assert(sizeof(FPortalSurfaceConstants) == 160, "Portal constant buffer layout mismatch");

// 톤매핑이 끝난 SceneColor 위에 포탈 면을 그린다.
class FPortalSurfaceRenderer
{
public:
	static FPortalSurfaceRenderer& Get() { static FPortalSurfaceRenderer Instance; return Instance; }

	// PortalTexture가 nullptr이면 PortalColor 단색으로 그린다.
	void Draw(UStaticMesh* Mesh, const FMatrix& World, const FMatrix& ViewProjection,
		FTexture2D* PortalTexture, const FVector4& PortalColor, float ResolutionScale = 0.5f);

private:
	bool EnsureInit();

	bool bInitialized = false;
	FPipelineState PipelineState;
	TUniquePtr<FConstantBuffer> ConstantBuffer;
};