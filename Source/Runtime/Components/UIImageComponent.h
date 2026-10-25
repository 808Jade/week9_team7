#pragma once
#include "Components/PrimitiveComponent.h"

struct alignas(16) FUIConstants
{
	FVector4 Rect;         // (X, Y, Width, Height) - 픽셀 좌표
	FVector4 ViewportSize; // (ViewportWidth, ViewportHeight, 0, 0)
	FLinearColor Color;    // (R, G, B, A)
};

class UUIImageComponent : public UPrimitiveComponent
{
	DECLARE_CLASS(UUIImageComponent, UPrimitiveComponent)
	REFLECT_START(ClassName)
		PROPERTY(PosX)
		PROPERTY(PosY)
		PROPERTY(Width)
		PROPERTY(Height)
		PROPERTY(Layer)
		PROPERTY(Material)
		PROPERTY(Color)
		PROPERTY(Opacity)
		REFLECT_END()

public:
	UUIImageComponent();
	virtual ~UUIImageComponent() = default;

	virtual void SubmitToRenderQueue(FRenderQueue& RenderQueue, const FViewContext& ViewContext) override;
	virtual FBox CalcLocalBounds() const override { return FBox(FVector(-100000.0f), FVector(100000.0f)); }
	virtual const FStaticMeshData* GetMeshData() const override { return Mesh ? &Mesh->GetMeshData() : nullptr; }
	virtual UStaticMesh* GetRenderMesh() const override { return Mesh; }

	// Getters & Setters (기존 FVector2 코드 및 호출부 호환 유지)
	FVector2 GetPosition() const { return FVector2(PosX, PosY); }
	void SetPosition(const FVector2& InPos) { PosX = InPos.X; PosY = InPos.Y; }
	void SetPosition(float InX, float InY) { PosX = InX; PosY = InY; }

	FVector2 GetSize() const { return FVector2(Width, Height); }
	void SetSize(const FVector2& InSize) { Width = InSize.X; Height = InSize.Y; }
	void SetSize(float InW, float InH) { Width = InW; Height = InH; }

	void SetTexture(UTexture2D* InTexture);
	void SetColor(const FColor& InColor) { Color = InColor; }
	void SetOpacity(float InOpacity) { Opacity = InOpacity; }

	int32 GetLayer() const { return Layer; }
	void SetLayer(int32 InLayer) { Layer = InLayer; }

private:
	float PosX = 100.0f;   // 화면 좌상단 X (픽셀)
	float PosY = 100.0f;   // 화면 좌상단 Y (픽셀)
	float Width = 200.0f;  // UI 가로 크기 (픽셀)
	float Height = 100.0f; // UI 세로 크기 (픽셀)
	int32 Layer = 0;      // UI 레이어
	FColor Color = FColor(255, 255, 255, 255);
	float Opacity = 1.0f;

	UStaticMesh* Mesh = nullptr;
	UMaterial* Material = nullptr;
	UTexture2D* Texture = nullptr;
	FUIConstants UIConstants;
};