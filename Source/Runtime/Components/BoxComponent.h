#pragma once

#include "ShapeComponent.h"

class UBoxComponent : public UShapeComponent
{
	DECLARE_CLASS(UBoxComponent, UShapeComponent)

	REFLECT_START(ClassName)
		PROPERTY(Extent)
		REFLECT_END()

public: 
	virtual FBox CalcLocalBounds() const override;
	virtual FCollisionShape GetCollisionShape(float Inflation = 0.0f) const override;
	virtual void DrawDebug(FLineBatcher* LineBatcher) const override;
	virtual void PostEditChangeProperty(const FProperty& Property) override;

	const FVector GetBoxExtent() const { return Extent; }
	FVector GetScaledBoxExtent() const { return Extent * GetWorldScale3D().GetAbs(); }
	void SetBoxExtent(const FVector InExtent)
	{
		Extent = InExtent;
		OnTransformDirty();
	}

private:
	FVector Extent = FVector(1.0f);
};
