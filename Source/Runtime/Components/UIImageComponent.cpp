#include "EnginePCH.h"
#include "UIImageComponent.h"
#include "Asset/AssetManager.h"

UUIImageComponent::UUIImageComponent()
{
	Mesh = UAssetManager::GetAssetByPath<UStaticMesh>("ParticleQuad");
	Material = UMaterial::CreateInstance(UAssetManager::GetAssetByPath<UMaterial>("UIMaterial"));
	Texture = UAssetManager::GetAssetByPath<UTexture2D>("WhiteTexture");
}

void UUIImageComponent::SetTexture(UTexture2D* InTexture)
{
	Texture = InTexture;
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
	if (!Mesh || !Material)
		return;

	const float ViewportWidth = static_cast<float>(ViewContext.Width);
	const float ViewportHeight = static_cast<float>(ViewContext.Height);

	UIConstants.Rect = FVector4(PosX, PosY, Width, Height);
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