#include "EnginePCH.h"
#include "UITextComponent.h"
#include "Asset/AssetManager.h"
#include "Render/RenderCommand.h"
#include "Render/Vertex.h"
#include "UIImageComponent.h" // FUIConstants 공유

namespace
{
	TArray<uint32> DecodeUTF8(const FString& Str)
	{
		TArray<uint32> Out;
		const unsigned char* P = reinterpret_cast<const unsigned char*>(Str.c_str());
		const unsigned char* End = P + Str.size();

		while (P < End)
		{
			uint32 CP = 0;
			int Len = 0;
			if (*P < 0x80) { CP = *P;        Len = 1; }
			else if ((*P & 0xE0) == 0xC0) { CP = *P & 0x1F; Len = 2; }
			else if ((*P & 0xF0) == 0xE0) { CP = *P & 0x0F; Len = 3; }
			else if ((*P & 0xF8) == 0xF0) { CP = *P & 0x07; Len = 4; }
			else { ++P; continue; }

			if (P + Len > End) break;
			for (int i = 1; i < Len; ++i) CP = (CP << 6) | (P[i] & 0x3F);

			Out.Add(CP);
			P += Len;
		}
		return Out;
	}
}

UUITextComponent::UUITextComponent()
{
	Font = UAssetManager::GetAssetByPath<UFont>("Fonts/CookieRun.json");
	Material = UMaterial::CreateInstance(UAssetManager::GetAssetByPath<UMaterial>("UITextMaterial"));
	if (Font && Font->AtlasTexture && Material)
	{
		Material->Textures.Empty();
		Material->Textures.Add(Font->AtlasTexture);
	}
}

void UUITextComponent::SetText(const FString& InText)
{
	Text = InText;
	RebuildTextMesh();
}

void UUITextComponent::SetFontSize(float InSize)
{
	FontSize = InSize;
	RebuildTextMesh();
}

void UUITextComponent::SetFont(UFont* InFont)
{
	Font = InFont;
	if (Material && Font && Font->AtlasTexture)
	{
		Material->Textures.Empty();
		Material->Textures.Add(Font->AtlasTexture);
	}
	RebuildTextMesh();
}

void UUITextComponent::RebuildTextMesh()
{
	if (!Font || Text.empty())
		return;

	CachedText = Text;
	CachedFontSize = FontSize;
	CachedFont = Font;

	TArray<uint32> CodePoints = DecodeUTF8(Text);
	TArray<FParticleVertex> Vertices;
	TArray<uint32> Indices;

	float PenX = 0.0f;
	const float BaseLine = FontSize * 0.8f;

	for (uint32 CP : CodePoints)
	{
		const FGlyphInfo* Glyph = Font->GlyphMap.Find(CP);
		if (!Glyph) continue;

		if (Glyph->bHasBounds)
		{
			float Left = PenX + Glyph->PlaneLeft * FontSize;
			float Right = PenX + Glyph->PlaneRight * FontSize;
			float Top = BaseLine - Glyph->PlaneTop * FontSize;
			float Bottom = BaseLine - Glyph->PlaneBottom * FontSize;

			float ULeft = Glyph->AtlasLeft / Font->Width;
			float URight = Glyph->AtlasRight / Font->Width;
			float VTop = 1.0f - (Glyph->AtlasTop / Font->Height);
			float VBottom = 1.0f - (Glyph->AtlasBottom / Font->Height);

			uint32 BaseIndex = static_cast<uint32>(Vertices.Num());

			Vertices.Add({ FVector(Left, Top, 0.0f), FVector2(ULeft, VTop) });
			Vertices.Add({ FVector(Right, Top, 0.0f), FVector2(URight, VTop) });
			Vertices.Add({ FVector(Right, Bottom, 0.0f), FVector2(URight, VBottom) });
			Vertices.Add({ FVector(Left, Bottom, 0.0f), FVector2(ULeft, VBottom) });

			Indices.Add(BaseIndex + 0);
			Indices.Add(BaseIndex + 1);
			Indices.Add(BaseIndex + 2);
			Indices.Add(BaseIndex + 0);
			Indices.Add(BaseIndex + 2);
			Indices.Add(BaseIndex + 3);
		}

		PenX += Glyph->Advance * FontSize;
	}

	Size = FVector2(PenX, FontSize * Font->LineHeight);

	if (Vertices.IsEmpty())
		return;

	if (!Mesh)
		Mesh = NewObject<UStaticMesh>();

	Mesh->VertexBuffer = RenderCommand::CreateStaticVertexBuffer(
		Vertices.GetData(), static_cast<uint32>(sizeof(FParticleVertex) * Vertices.Num()), sizeof(FParticleVertex));
	Mesh->IndexBuffer = RenderCommand::CreateStaticIndexBuffer(
		Indices.GetData(), static_cast<uint32>(Indices.Num()));
}

void UUITextComponent::SubmitToRenderQueue(FRenderQueue& RenderQueue, const FViewContext& ViewContext)
{
	if (!Font || Text.empty())
		return;

	if (Text != CachedText || FontSize != CachedFontSize || Font != CachedFont || !Mesh)
	{
		RebuildTextMesh();
	}

	if (!Mesh || !Material)
		return;

	const float ViewportWidth = static_cast<float>(ViewContext.Width);
	const float ViewportHeight = static_cast<float>(ViewContext.Height);

	static FUIConstants UIConstants;
	UIConstants.Rect = FVector4(PosX, PosY, Size.X, Size.Y);
	UIConstants.ViewportSize = FVector4(ViewportWidth, ViewportHeight, Font->DistanceRange, 0.0f);
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