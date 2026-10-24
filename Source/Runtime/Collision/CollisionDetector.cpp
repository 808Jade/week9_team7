#include "EnginePCH.h"
#include "CollisionDetector.h"
#include "Math/Sphere.h"
#include "Math/Box.h"

FContactPoint FCollisionDetector::ComputeContactPoint(const FCollisionShape& Shape1, const FTransform& Shape1Transform, const FCollisionShape& Shape2, const FTransform& Shape2Transform, const float CullDistance)
{
    // Sphere-Sphere
    if(Shape1.IsSphere() && Shape2.IsSphere())
    {
        return SphereSphereContactPoint(FSphere(FVector::ZeroVector, Shape1.GetSphereRadius()), Shape1Transform, FSphere(FVector::ZeroVector, Shape2.GetSphereRadius()), Shape2Transform, CullDistance);
    }
    // Sphere-Box
    if(Shape1.IsSphere() && Shape2.IsBox())
    {
        const FVector& InMax = Shape2.GetBox();
        const FVector& InMin = -Shape2.GetBox();

        return SphereBoxContactPoint(FSphere(FVector::ZeroVector, Shape1.GetSphereRadius()), Shape1Transform, FBox(InMin, InMax), Shape2Transform, CullDistance);
    }
    if(Shape1.IsBox() && Shape2.IsSphere())
    {
        const FVector& InMax = Shape1.GetBox();
        const FVector& InMin = -Shape1.GetBox();

        FContactPoint Result = SphereBoxContactPoint(FSphere(FVector::ZeroVector, Shape2.GetSphereRadius()), Shape2Transform, FBox(InMin, InMax), Shape1Transform, CullDistance);
        
        std::swap(Result.ContactPoints[0], Result.ContactPoints[1]);
        Result.ContactNormal = -Result.ContactNormal;
        return Result;
    }

    return FContactPoint();
}

bool FCollisionDetector::IsOverlapping(const FCollisionShape& Shape1, const FTransform& Shape1Transform, const FCollisionShape& Shape2, const FTransform& Shape2Transform, const float CullDistance)
{
    FContactPoint ContactPoint = ComputeContactPoint(Shape1, Shape1Transform, Shape2, Shape2Transform, CullDistance);
    
    return ContactPoint.IsSet() && ContactPoint.ContactDepth <= CullDistance;
}

FContactPoint FCollisionDetector::SphereSphereContactPoint(const FSphere& Sphere1, const FTransform& Sphere1Transform, const FSphere& Sphere2, const FTransform& Sphere2Transform, const float CullDistance)
{
    FContactPoint Result;

    const float R1 = Sphere1.GetRadius();
    const float R2 = Sphere2.GetRadius();

    const FTransform Sphere1ToSphere2 = Sphere1Transform.GetRelativeTransformNoScale(Sphere2Transform);

    const FVector Center1 = Sphere1ToSphere2.TransformPosition(Sphere1.GetCenter());
    const FVector Center2 = Sphere2.GetCenter();
    const FVector Direction = Center1 - Center2;
    const float SizeSq = Direction.SizeSquared();
    const float CullDistanceSq = FMath::Square(CullDistance + R1 + R2);

    if(SizeSq < CullDistanceSq)
    {
        const float Size = std::sqrt(SizeSq);
        const FVector Normal = Size > KINDA_SMALL_NUMBER ? Direction / Size : FVector(0.0f, 0.0f, 1.0f);
        const float NewPhi = Size - (R1 + R2);

        Result.ContactPoints[0] = Sphere1.GetCenter() - Sphere1ToSphere2.InverseTransformVector(Normal * R1);
        Result.ContactPoints[1] = Sphere2.GetCenter() + Normal * R2;
        Result.ContactNormal = Normal;
        Result.ContactDepth = NewPhi;
     }

    return Result;
}

FContactPoint FCollisionDetector::SphereBoxContactPoint(const FSphere& Sphere, const FTransform& SphereTransform, const FBox& Box, const FTransform& BoxTransform, const float CullDistance)
{
    FContactPoint Result;

    const FVector SphereWorld = SphereTransform.TransformPosition(Sphere.GetCenter());
    const FVector SphereBox = BoxTransform.InverseTransformPosition(SphereWorld);

    FVector NormalBox;
    float PhiToSphereCenter = Box.PhiWithNormal(SphereBox, NormalBox);
    float Phi = PhiToSphereCenter - Sphere.GetRadius();

    FVector NormalWorld = BoxTransform.TransformVectorNoScale(NormalBox);
    FVector LocationWorld = SphereWorld - NormalWorld * (Sphere.GetRadius());

    Result.ContactPoints[0] = SphereTransform.InverseTransformPosition(LocationWorld);
    Result.ContactPoints[1] = BoxTransform.InverseTransformPosition(LocationWorld - NormalWorld * Phi);
    Result.ContactNormal = NormalBox;
    Result.ContactDepth = Phi;

    return Result;
}
