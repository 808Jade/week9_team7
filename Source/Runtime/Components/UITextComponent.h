#pragma once
#include "Components/PrimitiveComponent.h"
#include "Text/Font.h"

class UUITextComponent : public UPrimitiveComponent
{
	DECLARE_CLASS(UUITextComponent, UPrimitiveComponent)
	REFLECT_START(ClassName)
		PROPERTY(Text)
		PROPERTY(FontSize)
		PROPERTY(PosX)
		PROPERTY(PosY)
		PROPERTY(Layer)
		PROPERTY(Color)
		PROPERTY(Opacity)
		PROPERTY(Font)
		REFLECT_END()

public:
	UUITextComponent();
	virtual ~UUITextComponent() = default;

	virtual void SubmitToRenderQueue(FRenderQueue& RenderQueue, const FViewContext& ViewContext) override;
	virtual FBox CalcLocalBounds() const override { return FBox(FVector(-100000.0f), FVector(100000.0f)); }
	virtual const FStaticMeshData* GetMeshData() const override { return Mesh ? &Mesh->GetMeshData() : nullptr; }
	virtual UStaticMesh* GetRenderMesh() const override { return Mesh; }

	// Getters & Setters
	const FString& GetText() const { return Text; }
	void SetText(const FString& InText);

	float GetFontSize() const { return FontSize; }
	void SetFontSize(float InSize);

	FVector2 GetPosition() const { return FVector2(PosX, PosY); }
	void SetPosition(const FVector2& InPos) { PosX = InPos.X; PosY = InPos.Y; }
	void SetPosition(float InX, float InY) { PosX = InX; PosY = InY; }

	FVector2 GetSize() const { return Size; }

	int32 GetLayer() const { return Layer; }
	void SetLayer(int32 InLayer) { Layer = InLayer; }

	void SetColor(const FColor& InColor) { Color = InColor; }
	void SetOpacity(float InOpacity) { Opacity = InOpacity; }

	UFont* GetFont() const { return Font; }
	void SetFont(UFont* InFont);

private:
	void RebuildTextMesh();

	FString Text = "Text";
	float FontSize = 32.0f;
	float PosX = 100.0f;
	float PosY = 100.0f;
	int32 Layer = 0;
	FColor Color = FColor(255, 255, 255, 255);
	float Opacity = 1.0f;

	FVector2 Size = FVector2(100.0f, 32.0f); // 텍스트 전체 가로, 세로 크기 (자동 계산)

	UFont* Font = nullptr;
	UStaticMesh* Mesh = nullptr;
	UMaterial* Material = nullptr;

	FString CachedText;
	float CachedFontSize = -1.0f;
	UFont* CachedFont = nullptr;
};