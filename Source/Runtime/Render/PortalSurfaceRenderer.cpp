#include "EnginePCH.h"
#include "Render/PortalSurfaceRenderer.h"
#include "Render/RenderCommand.h"
#include "Render/RenderResourceManager.h"
#include "Render/Texture2D.h"
#include "Asset/AssetManager.h"
#include <chrono>

namespace
{
	// 포탈 면 깊이를 카메라 쪽으로 당기는 NDC 깊이 바이어스.
	constexpr float PortalDepthBias = 1.0e-6f;
}

bool FPortalSurfaceRenderer::EnsureInit()
{
	if (bInitialized) return true;

	PipelineState.Shader = FRenderResourceManager::GetShaderProgram("Resources/Shader/PortalShader.hlsl");
	PipelineState.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	PipelineState.RasterizerState = ERasterizerState::SolidNone;    // 양면: 컬링 방향 신경 X
	PipelineState.BlendState = EBlendState::Opaque;
	PipelineState.DepthStencilState = EDepthStencilState::Default;    // 씬 깊이로 테스트 + 기록

	ConstantBuffer = RenderCommand::CreateConstantBuffer(sizeof(FPortalSurfaceConstants));
	bInitialized = PipelineState.Shader && ConstantBuffer;
	return bInitialized;
}

void FPortalSurfaceRenderer::Draw(UStaticMesh* Mesh, const FMatrix& World, const FMatrix& ViewProjection,
	FTexture2D* PortalTexture, const FVector4& PortalColor)
{
	if (!Mesh || !Mesh->IndexBuffer || !EnsureInit()) return;

	static const auto StartTime = std::chrono::steady_clock::now();
	const float Seconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - StartTime).count();

	FPortalSurfaceConstants Constants{};
	Constants.World = World;
	Constants.ViewProjection = ViewProjection;
	Constants.PortalColor = PortalColor;
	Constants.Params = FVector4(PortalTexture ? 1.0f : 0.0f, Seconds, PortalDepthBias, 0.0f); // y = 시간, z = 깊이 바이어스

	RenderCommand::BindPipelineState(PipelineState);
	RenderCommand::BindMesh(Mesh);
	RenderCommand::UpdateBufferData(ConstantBuffer.get(), &Constants, sizeof(Constants));
	RenderCommand::BindConstantBuffer(0, ConstantBuffer.get(), EShaderBindFlagBits::Vertex);
	RenderCommand::BindConstantBuffer(0, ConstantBuffer.get(), EShaderBindFlagBits::Pixel);
	RenderCommand::BindShaderResource(0, PortalTexture, EShaderBindFlagBits::Pixel);

	RenderCommand::DrawIndexed(Mesh->IndexBuffer->GetIndexCount());

	// 다음 프레임에 이 텍스처를 렌더 타깃으로 쓸 수 있도록 SRV를 푼다
	RenderCommand::BindShaderResource(0, static_cast<FTexture2D*>(nullptr), EShaderBindFlagBits::Pixel);
}