#include "EnginePCH.h"
#include "SphereComponent.h"

#include "Collision/CollisionShape.h"
#include "Render/LineBatcher.h"

namespace
{
    constexpr int32 SPHERE_SEGMENT_COUNT = 32;

    void AddWireSphereRing(
        FLineBatcher* LineBatcher,
        const FVector& Center,
        const FVector& AxisA,
        const FVector& AxisB,
        float Radius,
        const FVector4& Color)
    {
        FVector Prev = Center + AxisA * Radius;

        for (int32 i = 1; i <= SPHERE_SEGMENT_COUNT; ++i)
        {
            const float Angle = 2.0f * PI * i / SPHERE_SEGMENT_COUNT;
            const FVector Point = Center + AxisA * (std::cosf(Angle) * Radius) + AxisB * (std::sinf(Angle) * Radius);
            LineBatcher->AddLine(Prev, Point, Color);
            Prev = Point;
        }
    }
}

USphereComponent::USphereComponent()
{

}

FBox USphereComponent::CalcLocalBounds() const
{
    return FBox(FVector(-this->Radius), FVector(this->Radius));
}

float USphereComponent::GetScaledSphereRadius() const
{
    const FVector Scale = GetWorldScale3D().GetAbs();
    const float MinScale = std::min({ Scale.X, Scale.Y, Scale.Z });
    return std::max(this->Radius * MinScale, 0.0f);
}

FCollisionShape USphereComponent::GetCollisionShape(float Inflation) const
{
    return FCollisionShape::MakeSphere(GetScaledSphereRadius()).Inflate(Inflation);
}

void USphereComponent::DrawDebug(FLineBatcher* LineBatcher) const
{
    if (!LineBatcher) return;

    FTransform WorldTransform;
    WorldTransform.Location = GetWorldLocation();
    WorldTransform.Rotation = GetWorldRotation();

    const FVector Center = WorldTransform.Location;
    const FVector Forward = WorldTransform.GetForward();
    const FVector Right = WorldTransform.GetRight();
    const FVector Up = WorldTransform.GetUp();
    const float ScaledRadius = GetScaledSphereRadius();
    const FVector4 Color = ShapeColor.ToVector4();

    AddWireSphereRing(LineBatcher, Center, Forward, Right, ScaledRadius, Color);
    AddWireSphereRing(LineBatcher, Center, Forward, Up, ScaledRadius, Color);
    AddWireSphereRing(LineBatcher, Center, Right, Up, ScaledRadius, Color);
}

void USphereComponent::PostEditChangeProperty(const FProperty& Property)
{
    Super::PostEditChangeProperty(Property);
    if (Property.Name == "Radius")
    {
        OnTransformDirty();
    }
}
