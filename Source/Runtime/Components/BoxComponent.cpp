#include "EnginePCH.h"
#include "BoxComponent.h"

#include "Collision/CollisionShape.h"
#include "Render/LineBatcher.h"

FBox UBoxComponent::CalcLocalBounds() const
{
    return FBox(-this->Extent, this->Extent);
}

FCollisionShape UBoxComponent::GetCollisionShape(float Inflation) const
{
    return FCollisionShape::MakeBox(GetScaledBoxExtent()).Inflate(Inflation);
}

void UBoxComponent::DrawDebug(FLineBatcher* LineBatcher) const
{
    if (!LineBatcher) return;

    LineBatcher->AddOrientedBox(CalcLocalBounds(), GetWorldMatrix(), ShapeColor.ToVector4());
}

void UBoxComponent::PostEditChangeProperty(const FProperty& Property)
{
    Super::PostEditChangeProperty(Property);
    if (Property.Name == "Extent")
    {
        OnTransformDirty();
    }
}
