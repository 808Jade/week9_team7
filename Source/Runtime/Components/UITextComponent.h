#pragma once
#include "Components/PrimitiveComponent.h"
#include "Components/UIImageComponent.h" // FUIConstants
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
		PROPERTY(bCenterHorizontally)
		PROPERTY(bCenterVertically)
		PROPERTY(RelativeX)
		PROPERTY(RelativeY)
		PROPERTY(OffsetX)
		PROPERTY(OffsetY)
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

	// 가로 중앙 정렬
	void SetCenterHorizontally(bool bCenter, float InOffset = 0.0f)
	{
		bCenterHorizontally = bCenter;
		OffsetX = InOffset;
	}
	bool IsCenterHorizontally() const { return bCenterHorizontally; }

	// 세로 중앙 정렬
	void SetCenterVertically(bool bCenter, float InOffset = 0.0f)
	{
		bCenterVertically = bCenter;
		OffsetY = InOffset;
	}
	bool IsCenterVertically() const { return bCenterVertically; }

	// 뷰포트 상대 세로 위치 (0.0f ~ 1.0f, 음수면 무효)
	void SetRelativeY(float InRelY, float InOffset = 0.0f)
	{
		RelativeY = InRelY;
		OffsetY = InOffset;
	}
	float GetRelativeY() const { return RelativeY; }

	// 뷰포트 상대 가로 위치 (0.0f ~ 1.0f, 음수면 무효)
	void SetRelativeX(float InRelX, float InOffset = 0.0f)
	{
		RelativeX = InRelX;
		OffsetX = InOffset;
	}
	float GetRelativeX() const { return RelativeX; }

private:
	void RebuildTextMesh();

	FString Text = "Text";
	float FontSize = 32.0f;
	float PosX = 100.0f;
	float PosY = 100.0f;
	int32 Layer = 0;
	FColor Color = FColor(255, 255, 255, 255);
	float Opacity = 1.0f;

	bool bCenterHorizontally = false;
	bool bCenterVertically = false;
	float RelativeX = -1.0f;
	float RelativeY = -1.0f;
	float OffsetX = 0.0f;
	float OffsetY = 0.0f;

	FVector2 Size = FVector2(100.0f, 32.0f); // 텍스트 전체 가로, 세로 크기 (자동 계산)

	UFont* Font = nullptr;
	UStaticMesh* Mesh = nullptr;
	UMaterial* Material = nullptr;

	FString CachedText;
	float CachedFontSize = -1.0f;
	UFont* CachedFont = nullptr;

	FUIConstants UIConstants; // 각 인스턴스 전용 FUIConstants
};
