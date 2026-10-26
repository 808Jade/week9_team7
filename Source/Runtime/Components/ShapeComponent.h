#pragma once
#include "PrimitiveComponent.h"
#include "Math/Color.h"

class FLineBatcher;

class UShapeComponent : public UPrimitiveComponent
{
	DECLARE_CLASS(UShapeComponent, UPrimitiveComponent)

	REFLECT_START(ClassName)
		PROPERTY(ShapeColor)
		REFLECT_END()

public:
	UShapeComponent();

	virtual void DrawDebug(FLineBatcher* LineBatcher) const {}

	FColor GetShapeColor() const { return ShapeColor; }
	void SetShapeColor(const FColor& InColor) { ShapeColor = InColor; }

protected:
	FColor ShapeColor = FColor(223, 149, 157, 255);
};
