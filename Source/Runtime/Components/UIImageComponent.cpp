#include "EnginePCH.h"
#include "UIImageComponent.h"
#include "Asset/AssetManager.h"

UUIImageComponent::UUIImageComponent()
{
	EnsureMaterial();
}

void UUIImageComponent::EnsureMaterial()
{
	if (!Mesh)
	{
		Mesh = UAssetManager::GetAssetByPath<UStaticMesh>("ParticleQuad");
	}
	if (!Material)
	{
		Material = UMaterial::CreateInstance(UAssetManager::GetAssetByPath<UMaterial>("UIMaterial"));
		if (Material)
		{
			Material->Textures.Empty();
			Material->Textures.Add(Texture ? Texture : UAssetManager::GetAssetByPath<UTexture2D>("WhiteTexture"));
		}
	}
}

void UUIImageComponent::SetTexture(UTexture2D* InTexture)
{
	Texture = InTexture;
	EnsureMaterial();
	if (Material)
	{
		if (Material->Textures.IsEmpty())
			Material->Textures.Add(Texture ? Texture : UAssetManager::GetAssetByPath<UTexture2D>("WhiteTexture"));
		else
			Material->Textures[0] = Texture ? Texture : UAssetManager::GetAssetByPath<UTexture2D>("WhiteTexture");
	}
}

void UUIImageComponent::SubmitToRenderQueue(FRenderQueue& RenderQueue, const FViewContext& ViewContext)
{
	EnsureMaterial();
	if (!Mesh || !Material)
		return;

	const float ViewportWidth = static_cast<float>(ViewContext.Width);
	const float ViewportHeight = static_cast<float>(ViewContext.Height);

	float ActualX = PosX;
	float ActualY = PosY;
	float ActualW = Width;
	float ActualH = Height;

	if (bMatchViewportSize)
	{
		// 뷰포트 전체 화면에 맞춤 (좌상단 0,0 ~ 가로/세로 전체)
		ActualX = 0.0f;
		ActualY = 0.0f;
		ActualW = ViewportWidth;
		ActualH = ViewportHeight;
	}
	else if (bCenterOnViewport)
	{
		// 뷰포트 중앙에 정확히 배치 (정중앙 + Offset)
		ActualX = (ViewportWidth - Width) * 0.5f + OffsetX;
		ActualY = (ViewportHeight - Height) * 0.5f + OffsetY;
	}

	UIConstants.Rect = FVector4(ActualX, ActualY, ActualW, ActualH);
	UIConstants.ViewportSize = FVector4(ViewportWidth, ViewportHeight, 0.0f, 0.0f);
	UIConstants.Color = FLinearColor(
		Color.R / 255.0f,
		Color.G / 255.0f,
		Color.B / 255.0f,
		(Color.A / 255.0f) * Opacity
	);

	FRenderPacket Packet;
	Packet.Mesh = Mesh;
	Packet.Material = Material;
	static const FMatrix IdentityMatrix = FMatrix::Identity;
	Packet.Model = &IdentityMatrix;
	Packet.MaterialParamData = &UIConstants;
	Packet.MaterialParamDataSize = sizeof(FUIConstants);

	RenderQueue.Add(Packet);
}
