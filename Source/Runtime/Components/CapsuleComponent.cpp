#include "EnginePCH.h"
#include "CapsuleComponent.h"

#include "Collision/CollisionShape.h"
#include "Render/LineBatcher.h"

namespace
{
	constexpr int32 CAPSULE_SEGMENT_COUNT = 32;

	void AddWireCircle(
		FLineBatcher* LineBatcher,
		const FVector& Center,
		const FVector& AxisA,
		const FVector& AxisB,
		float Radius,
		const FVector4& Color)
	{
		FVector Prev = Center + AxisA * Radius;

		for (int32 i = 1; i <= CAPSULE_SEGMENT_COUNT; ++i)
		{
			const float Angle = 2.0f * PI * i / CAPSULE_SEGMENT_COUNT;
			const FVector Point = Center + AxisA * (std::cosf(Angle) * Radius) + AxisB * (std::sinf(Angle) * Radius);
			LineBatcher->AddLine(Prev, Point, Color);
			Prev = Point;
		}
	}

	void AddWireHalfCircle(
		FLineBatcher* LineBatcher,
		const FVector& Center,
		const FVector& AxisA,
		const FVector& AxisB,
		float Radius,
		const FVector4& Color)
	{
		constexpr int32 HalfCount = CAPSULE_SEGMENT_COUNT / 2;
		FVector Prev = Center + AxisA * Radius;

		for (int32 i = 1; i <= HalfCount; ++i)
		{
			const float Angle = PI * i / HalfCount;
			const FVector Point = Center + AxisA * (std::cosf(Angle) * Radius) + AxisB * (std::sinf(Angle) * Radius);
			LineBatcher->AddLine(Prev, Point, Color);
			Prev = Point;
		}
	}
}

UCapsuleComponent::UCapsuleComponent()
{
}

FBox UCapsuleComponent::CalcLocalBounds() const
{
	return FBox(FVector(-Radius, -Radius, -HalfHeight), FVector(Radius, Radius, HalfHeight));
}

float UCapsuleComponent::GetScaledCapsuleRadius() const
{
	const FVector Scale = GetWorldScale3D().GetAbs();
	const float MaxHorizontalScale = std::max(Scale.X, Scale.Y);
	return std::max(Radius * MaxHorizontalScale, 0.0f);
}

float UCapsuleComponent::GetScaledCapsuleHalfHeight() const
{
	const FVector Scale = GetWorldScale3D().GetAbs();
	return std::max(HalfHeight * Scale.Z, 0.0f);
}

FCollisionShape UCapsuleComponent::GetCollisionShape(float Inflation) const
{
	return FCollisionShape::MakeCapsule(GetScaledCapsuleRadius(), GetScaledCapsuleHalfHeight()).Inflate(Inflation);
}

void UCapsuleComponent::DrawDebug(FLineBatcher* LineBatcher) const
{
	if (!LineBatcher) return;

	FTransform WorldTransform;
	WorldTransform.Location = GetWorldLocation();
	WorldTransform.Rotation = GetWorldRotation();

	const FVector Center = WorldTransform.Location;
	const FVector Forward = WorldTransform.GetForward();
	const FVector Right = WorldTransform.GetRight();
	const FVector Up = WorldTransform.GetUp();

	const float ScaledRadius = GetScaledCapsuleRadius();
	const float ScaledHalfHeight = GetScaledCapsuleHalfHeight();
	const float HalfLength = std::max(0.0f, ScaledHalfHeight - ScaledRadius);
	const FVector4 Color = ShapeColor.ToVector4();

	const FVector TopCenter = Center + Up * HalfLength;
	const FVector BottomCenter = Center - Up * HalfLength;

	AddWireCircle(LineBatcher, TopCenter, Forward, Right, ScaledRadius, Color);
	AddWireCircle(LineBatcher, BottomCenter, Forward, Right, ScaledRadius, Color);

	LineBatcher->AddLine(TopCenter + Forward * ScaledRadius, BottomCenter + Forward * ScaledRadius, Color);
	LineBatcher->AddLine(TopCenter - Forward * ScaledRadius, BottomCenter - Forward * ScaledRadius, Color);
	LineBatcher->AddLine(TopCenter + Right * ScaledRadius, BottomCenter + Right * ScaledRadius, Color);
	LineBatcher->AddLine(TopCenter - Right * ScaledRadius, BottomCenter - Right * ScaledRadius, Color);

	AddWireHalfCircle(LineBatcher, TopCenter, Forward, Up, ScaledRadius, Color);
	AddWireHalfCircle(LineBatcher, TopCenter, Right, Up, ScaledRadius, Color);
	AddWireHalfCircle(LineBatcher, BottomCenter, Forward, -Up, ScaledRadius, Color);
	AddWireHalfCircle(LineBatcher, BottomCenter, Right, -Up, ScaledRadius, Color);
}

void UCapsuleComponent::PostEditChangeProperty(const FProperty& Property)
{
	Super::PostEditChangeProperty(Property);
	if (Property.Name == "Radius" || Property.Name == "HalfHeight")
	{
		OnTransformDirty();
	}
}
