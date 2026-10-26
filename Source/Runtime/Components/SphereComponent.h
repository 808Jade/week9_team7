#pragma once

#include "ShapeComponent.h"

class USphereComponent : public UShapeComponent
{
	DECLARE_CLASS(USphereComponent, UShapeComponent)

	REFLECT_START(ClassName)
		PROPERTY(Radius)
		REFLECT_END()

public:
	USphereComponent();

	virtual FBox CalcLocalBounds() const override;
	virtual FCollisionShape GetCollisionShape(float Inflation = 0.0f) const override;
	virtual void DrawDebug(FLineBatcher* LineBatcher) const override;
	virtual void PostEditChangeProperty(const FProperty& Property) override;

	const float GetSphereRadius() const { return Radius; }
	float GetScaledSphereRadius() const;
	void SetSphereRadius(float InRadius)
	{
		Radius = InRadius;
		OnTransformDirty();
	}

private:
	float Radius = 1.0f;
};
