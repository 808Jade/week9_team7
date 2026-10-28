#pragma once

#include "ShapeComponent.h"

class UCapsuleComponent : public UShapeComponent
{
	DECLARE_CLASS(UCapsuleComponent, UShapeComponent)

	REFLECT_START(ClassName)
		PROPERTY(Radius)
		PROPERTY(HalfHeight)
	REFLECT_END()

public:
	UCapsuleComponent();

	virtual FBox CalcLocalBounds() const override;
	virtual FCollisionShape GetCollisionShape(float Inflation = 0.0f) const override;
	virtual void DrawDebug(FLineBatcher* LineBatcher) const override;
	virtual void PostEditChangeProperty(const FProperty& Property) override;

	float GetCapsuleRadius() const { return Radius; }
	float GetCapsuleHalfHeight() const { return HalfHeight; }
	float GetScaledCapsuleRadius() const;
	float GetScaledCapsuleHalfHeight() const;

	void SetCapsuleRadius(float InRadius)
	{
		Radius = InRadius;
		OnTransformDirty();
	}

	void SetCapsuleHalfHeight(float InHalfHeight)
	{
		HalfHeight = InHalfHeight;
		OnTransformDirty();
	}

	void SetCapsuleSize(float InRadius, float InHalfHeight)
	{
		Radius = InRadius;
		HalfHeight = InHalfHeight;
		OnTransformDirty();
	}

private:
	float Radius = 0.5f;
	float HalfHeight = 1.0f;
};
