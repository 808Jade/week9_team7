#include "EnginePCH.h"
#include "CollisionDetector.h"
#include "HitResult.h"
#include "Math/Sphere.h"
#include "Math/Box.h"
#include "Math/Capsule.h"
#include "Math/MathUtility.h"

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
    // Box-Box
    if(Shape1.IsBox() && Shape2.IsBox())
    {
        const FVector& InMax1 = Shape1.GetBox();
        const FVector& InMin1 = -Shape1.GetBox();

        const FVector& InMax2 = Shape2.GetBox();
        const FVector& InMin2 = -Shape2.GetBox();

        return BoxBoxContactPoint(FBox(InMin1, InMax1), Shape1Transform, FBox(InMin2, InMax2), Shape2Transform, CullDistance);
    }
    if(Shape1.IsSphere() && Shape2.IsCapsule())
    {
        return SphereCapsuleContactPoint(FSphere(FVector::ZeroVector, Shape1.GetSphereRadius()), Shape1Transform, FCapsule(FVector::ZeroVector, Shape2.GetCapsuleRadius(), Shape2.GetCapsuleHalfHeight()), Shape2Transform, CullDistance);
    }
    if(Shape1.IsCapsule() && Shape2.IsSphere())
    {
        FContactPoint Result = SphereCapsuleContactPoint(FSphere(FVector::ZeroVector, Shape2.GetSphereRadius()), Shape2Transform, FCapsule(FVector::ZeroVector, Shape1.GetCapsuleRadius(), Shape1.GetCapsuleHalfHeight()), Shape1Transform, CullDistance);
        std::swap(Result.ContactPoints[0], Result.ContactPoints[1]);
        Result.ContactNormal = -Result.ContactNormal;
        return Result;
    }
    if(Shape1.IsCapsule() && Shape2.IsBox())
    {
        const FVector& InMax = Shape2.GetBox();
        const FVector& InMin = -Shape2.GetBox();

        return CapsuleBoxContactPoint(FCapsule(FVector::ZeroVector, Shape1.GetCapsuleRadius(), Shape1.GetCapsuleHalfHeight()), Shape1Transform, FBox(InMin, InMax), Shape2Transform, CullDistance);
    }
    if(Shape1.IsBox() && Shape2.IsCapsule())
    {
        const FVector& InMax = Shape1.GetBox();
        const FVector& InMin = -Shape1.GetBox();

        FContactPoint Result = CapsuleBoxContactPoint(FCapsule(FVector::ZeroVector, Shape2.GetCapsuleRadius(), Shape2.GetCapsuleHalfHeight()), Shape2Transform, FBox(InMin, InMax), Shape1Transform, CullDistance);
        std::swap(Result.ContactPoints[0], Result.ContactPoints[1]);
        Result.ContactNormal = -Result.ContactNormal;
        return Result;
    }
    if(Shape1.IsCapsule() && Shape2.IsCapsule())
    {
        return CapsuleCapsuleContactPoint(FCapsule(FVector::ZeroVector, Shape1.GetCapsuleRadius(), Shape1.GetCapsuleHalfHeight()), Shape1Transform, FCapsule(FVector::ZeroVector, Shape2.GetCapsuleRadius(), Shape2.GetCapsuleHalfHeight()), Shape2Transform, CullDistance);
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

FContactPoint FCollisionDetector::BoxBoxContactPoint(const FBox& Box1, const FTransform& Box1Transform, const FBox& Box2, const FTransform& Box2Transform, const float CullDistance)
{
    FContactPoint Result;

    const FVector Center1 = Box1Transform.Location;
    const FVector Center2 = Box2Transform.Location;
    const FVector Extent1 = Box1.GetExtent();
    const FVector Extent2 = Box2.GetExtent();

    const FVector Axis1[3] = { Box1Transform.GetForward(), Box1Transform.GetRight(), Box1Transform.GetUp() };
    const FVector Axis2[3] = { Box2Transform.GetForward(), Box2Transform.GetRight(), Box2Transform.GetUp() };
    FVector CandidateAxes[15] = {};

    const FVector D = Center1 - Center2;

    // Make candidate axes
    for(int i = 0; i < 3; i++)
    {
        CandidateAxes[i] = Axis1[i];
        CandidateAxes[i + 3] = Axis2[i];

        for(int j = 0;j < 3;j++)
        {
            CandidateAxes[6 + i * 3 + j] = FVector::Cross(Axis1[i], Axis2[j]).Normalized();
        }
    }

    // Loop candidate axes
    float MaxPhi = -FLT_MAX;
    FVector BestL;
    float BestR1;
    for(int i = 0; i < 15;i++)
    {
        if(CandidateAxes[i].SizeSquared() <= KINDA_SMALL_NUMBER)
        {
            continue;
        }

        const FVector L = CandidateAxes[i].Normalized();
        
        float Dist = std::abs((FVector::Dot(D, L)));
        float R1 = 0.0f;
        float R2 = 0.0f;
        for(int j = 0; j < 3; j++)
        {
            R1 += Extent1.V[j] * std::abs(FVector::Dot(Axis1[j], L));
            R2 += Extent2.V[j] * std::abs(FVector::Dot(Axis2[j], L));
        }
        float R = R1 + R2;
        
        float PhiL = Dist - R;
        if(PhiL > CullDistance)
        {
            return FContactPoint();
        }

        if(PhiL > MaxPhi)
        {
            MaxPhi = PhiL;
            BestL = L;
            BestR1 = R1;
        }
    }

    if(FVector::Dot(D, BestL) < 0.0f)
    {
        BestL = -BestL;
    }

    const FVector LocationWorld = Center1 - BestL * BestR1;
    Result.ContactPoints[0] = Box1Transform.InverseTransformPosition(LocationWorld);
    Result.ContactPoints[1] = Box2Transform.InverseTransformPosition(LocationWorld - BestL * MaxPhi);
    Result.ContactNormal = BestL;
    Result.ContactDepth = MaxPhi;

    return Result;
}

FContactPoint FCollisionDetector::SphereCapsuleContactPoint(const FSphere& Sphere, const FTransform& SphereTransform, const FCapsule& Capsule, const FTransform& CapsuleTransform, const float CullDistance)
{
    FContactPoint Result;

    const FVector SphereWorld = SphereTransform.TransformPosition(Sphere.GetCenter());
    FVector Seg0, Seg1;
    Capsule.GetSegment(CapsuleTransform, Seg0, Seg1);

    const FVector ClosestPointOnCapsuleSeg = FMathUtil::ClosestPointOnSegment(SphereWorld, Seg0, Seg1);
    const FVector Direction = SphereWorld - ClosestPointOnCapsuleSeg;
    const float SizeSq = Direction.SizeSquared();
    const float R1 = Sphere.GetRadius();
    const float R2 = Capsule.GetRadius();
    const float CullDistanceSq = FMath::Square(CullDistance + R1 + R2);

    if (SizeSq < CullDistanceSq)
    {
        const float Size = std::sqrt(SizeSq);
        const FVector Normal = Size > KINDA_SMALL_NUMBER ? Direction / Size : CapsuleTransform.GetUp();
        const float NewPhi = Size - (R1 + R2);

        const FVector LocationSphere = SphereWorld - Normal * R1;
        const FVector LocationCapsule = ClosestPointOnCapsuleSeg + Normal * R2;

        Result.ContactPoints[0] = SphereTransform.InverseTransformPosition(LocationSphere);
        Result.ContactPoints[1] = CapsuleTransform.InverseTransformPosition(LocationCapsule);
        Result.ContactNormal = Normal;
        Result.ContactDepth = NewPhi;
    }

    return Result;
}

FContactPoint FCollisionDetector::CapsuleCapsuleContactPoint(const FCapsule& Capsule1, const FTransform& Capsule1Transform, const FCapsule& Capsule2, const FTransform& Capsule2Transform, const float CullDistance)
{
    FContactPoint Result;

    FVector Seg1_0, Seg1_1;
    Capsule1.GetSegment(Capsule1Transform, Seg1_0, Seg1_1);

    FVector Seg2_0, Seg2_1;
    Capsule2.GetSegment(Capsule2Transform, Seg2_0, Seg2_1);

    FVector Closest1, Closest2;
    FMathUtil::SegmentDistToSegment(Seg1_0, Seg1_1, Seg2_0, Seg2_1, Closest1, Closest2);

    const FVector Direction = Closest1 - Closest2;
    const float SizeSq = Direction.SizeSquared();
    const float R1 = Capsule1.GetRadius();
    const float R2 = Capsule2.GetRadius();
    const float CullDistanceSq = FMath::Square(CullDistance + R1 + R2);

    if (SizeSq < CullDistanceSq)
    {
        const float Size = std::sqrt(SizeSq);
        const FVector Normal = Size > KINDA_SMALL_NUMBER ? Direction / Size : Capsule2Transform.GetUp();
        const float NewPhi = Size - (R1 + R2);

        const FVector Location1 = Closest1 - Normal * R1;
        const FVector Location2 = Closest2 + Normal * R2;

        Result.ContactPoints[0] = Capsule1Transform.InverseTransformPosition(Location1);
        Result.ContactPoints[1] = Capsule2Transform.InverseTransformPosition(Location2);
        Result.ContactNormal = Normal;
        Result.ContactDepth = NewPhi;
    }

    return Result;
}

FContactPoint FCollisionDetector::CapsuleBoxContactPoint(const FCapsule& Capsule, const FTransform& CapsuleTransform, const FBox& Box, const FTransform& BoxTransform, const float CullDistance)
{
    FContactPoint Result;

    FVector Seg0, Seg1;
    Capsule.GetSegment(CapsuleTransform, Seg0, Seg1);

    const FVector Seg0Box = BoxTransform.InverseTransformPosition(Seg0);
    const FVector Seg1Box = BoxTransform.InverseTransformPosition(Seg1);

    float BestT = 0.0f;
    float MinPhiBox = FLT_MAX;
    FVector BestNormalBox;

    constexpr int32 NumSamples = 16;
    for (int32 i = 0; i <= NumSamples; ++i)
    {
        const float T = static_cast<float>(i) / static_cast<float>(NumSamples);
        const FVector SamplePoint = Seg0Box + (Seg1Box - Seg0Box) * T;
        FVector NormalBox;
        const float Phi = Box.PhiWithNormal(SamplePoint, NormalBox);
        if (Phi < MinPhiBox)
        {
            MinPhiBox = Phi;
            BestT = T;
            BestNormalBox = NormalBox;
        }
    }

    float LowT = FMath::Max(0.0f, BestT - (1.0f / static_cast<float>(NumSamples)));
    float HighT = FMath::Min(1.0f, BestT + (1.0f / static_cast<float>(NumSamples)));
    for (int32 Iter = 0; Iter < 12; ++Iter)
    {
        const float M1 = LowT + (HighT - LowT) / 3.0f;
        const float M2 = HighT - (HighT - LowT) / 3.0f;

        FVector N1, N2;
        const float Phi1 = Box.PhiWithNormal(Seg0Box + (Seg1Box - Seg0Box) * M1, N1);
        const float Phi2 = Box.PhiWithNormal(Seg0Box + (Seg1Box - Seg0Box) * M2, N2);

        if (Phi1 < Phi2)
        {
            HighT = M2;
            if (Phi1 < MinPhiBox)
            {
                MinPhiBox = Phi1;
                BestT = M1;
                BestNormalBox = N1;
            }
        }
        else
        {
            LowT = M1;
            if (Phi2 < MinPhiBox)
            {
                MinPhiBox = Phi2;
                BestT = M2;
                BestNormalBox = N2;
            }
        }
    }

    const float Phi = MinPhiBox - Capsule.GetRadius();
    if (Phi > CullDistance)
    {
        return Result;
    }

    const FVector BestPointLocalBox = Seg0Box + (Seg1Box - Seg0Box) * BestT;
    const FVector BestPointWorld = BoxTransform.TransformPosition(BestPointLocalBox);
    const FVector NormalWorld = BoxTransform.TransformVectorNoScale(BestNormalBox);
    const FVector LocationWorld = BestPointWorld - NormalWorld * Capsule.GetRadius();

    Result.ContactPoints[0] = CapsuleTransform.InverseTransformPosition(LocationWorld);
    Result.ContactPoints[1] = BoxTransform.InverseTransformPosition(LocationWorld - NormalWorld * Phi);
    Result.ContactNormal = NormalWorld;
    Result.ContactDepth = Phi;

    return Result;
}

bool FCollisionDetector::Sweep(const FCollisionShape& Shape1, const FTransform& Shape1Transform, const FCollisionShape& Shape2, const FTransform& Shape2Transform, const FVector& SweepDir, const float SweepLength, FHitResult& OutHit)
{
    // Sphere-Sphere
    if(Shape1.IsSphere() && Shape2.IsSphere())
    {
        return SphereSphereSweep(FSphere(FVector::ZeroVector, Shape1.GetSphereRadius()), Shape1Transform, FSphere(FVector::ZeroVector, Shape2.GetSphereRadius()), Shape2Transform, SweepDir,SweepLength, OutHit);
    }
    // Sphere-Box
    if(Shape1.IsSphere() && Shape2.IsBox())
    {
        const FVector& InMax = Shape2.GetBox();
        const FVector& InMin = -Shape2.GetBox();

        return SphereBoxSweep(FSphere(FVector::ZeroVector, Shape1.GetSphereRadius()), Shape1Transform, FBox(InMin, InMax), Shape2Transform, SweepDir,SweepLength, OutHit);
    }
    if(Shape1.IsBox() && Shape2.IsSphere())
    {
        const FVector& InMax = Shape1.GetBox();
        const FVector& InMin = -Shape1.GetBox();

        bool Result = SphereBoxSweep(FSphere(FVector::ZeroVector, Shape2.GetSphereRadius()), Shape2Transform, FBox(InMin, InMax), Shape1Transform, -SweepDir, SweepLength, OutHit);

        OutHit.Normal = -OutHit.Normal;
        OutHit.ImpactNormal = OutHit.Normal;
        OutHit.Location = Shape1Transform.Location + SweepDir * OutHit.Distance;
        OutHit.ImpactPoint = Shape2Transform.Location - OutHit.Normal * Shape2.GetSphereRadius();

        return Result;
    }
    // Box-Box
    if(Shape1.IsBox() && Shape2.IsBox())
    {
        const FVector& InMax1 = Shape1.GetBox();
        const FVector& InMin1 = -Shape1.GetBox();

        const FVector& InMax2 = Shape2.GetBox();
        const FVector& InMin2 = -Shape2.GetBox();

        return BoxBoxSweep(FBox(InMin1, InMax1), Shape1Transform, FBox(InMin2, InMax2), Shape2Transform, SweepDir, SweepLength, OutHit);
    }
    if(Shape1.IsSphere() && Shape2.IsCapsule())
    {
        return SphereCapsuleSweep(FSphere(FVector::ZeroVector, Shape1.GetSphereRadius()), Shape1Transform, FCapsule(FVector::ZeroVector, Shape2.GetCapsuleRadius(), Shape2.GetCapsuleHalfHeight()), Shape2Transform, SweepDir, SweepLength, OutHit);
    }
    if(Shape1.IsCapsule() && Shape2.IsSphere())
    {
        bool Result = SphereCapsuleSweep(FSphere(FVector::ZeroVector, Shape2.GetSphereRadius()), Shape2Transform, FCapsule(FVector::ZeroVector, Shape1.GetCapsuleRadius(), Shape1.GetCapsuleHalfHeight()), Shape1Transform, -SweepDir, SweepLength, OutHit);

        OutHit.Normal = -OutHit.Normal;
        OutHit.ImpactNormal = OutHit.Normal;
        OutHit.Location = Shape1Transform.Location + SweepDir * OutHit.Distance;
        OutHit.ImpactPoint = Shape2Transform.Location - OutHit.Normal * Shape2.GetSphereRadius();

        return Result;
    }
    if(Shape1.IsCapsule() && Shape2.IsBox())
    {
        const FVector& InMax = Shape2.GetBox();
        const FVector& InMin = -Shape2.GetBox();

        return CapsuleBoxSweep(FCapsule(FVector::ZeroVector, Shape1.GetCapsuleRadius(), Shape1.GetCapsuleHalfHeight()), Shape1Transform, FBox(InMin, InMax), Shape2Transform, SweepDir, SweepLength, OutHit);
    }
    if(Shape1.IsBox() && Shape2.IsCapsule())
    {
        const FVector& InMax = Shape1.GetBox();
        const FVector& InMin = -Shape1.GetBox();

        bool Result = CapsuleBoxSweep(FCapsule(FVector::ZeroVector, Shape2.GetCapsuleRadius(), Shape2.GetCapsuleHalfHeight()), Shape2Transform, FBox(InMin, InMax), Shape1Transform, -SweepDir, SweepLength, OutHit);

        OutHit.Normal = -OutHit.Normal;
        OutHit.ImpactNormal = OutHit.Normal;
        OutHit.Location = Shape1Transform.Location + SweepDir * OutHit.Distance;
        OutHit.ImpactPoint = OutHit.Location + OutHit.Normal * OutHit.PenetrationDepth;

        return Result;
    }
    if(Shape1.IsCapsule() && Shape2.IsCapsule())
    {
        return CapsuleCapsuleSweep(FCapsule(FVector::ZeroVector, Shape1.GetCapsuleRadius(), Shape1.GetCapsuleHalfHeight()), Shape1Transform, FCapsule(FVector::ZeroVector, Shape2.GetCapsuleRadius(), Shape2.GetCapsuleHalfHeight()), Shape2Transform, SweepDir, SweepLength, OutHit);
    }

    return false;
}

bool FCollisionDetector::SphereSphereSweep(const FSphere& Sphere1, const FTransform& Sphere1Transform, const FSphere& Sphere2, const FTransform& Sphere2Transform, const FVector& SweepDir, const float SweepLength, FHitResult& OutHit)
{
    const FVector Center1 = Sphere1Transform.TransformPosition(Sphere1.GetCenter());
    const FVector Center2 = Sphere2Transform.TransformPosition(Sphere2.GetCenter());
    const float RadiusSum = Sphere1.GetRadius() + Sphere2.GetRadius();
    const float RadiusSumSqr = RadiusSum * RadiusSum;

    const FVector Offset = Center2 - Center1;
    const float OffsetSizeSqr = Offset.SizeSquared();
    if(OffsetSizeSqr < RadiusSumSqr)
    {
        const FVector Dir = Center1 - Center2;
        const float Dist = std::sqrt(OffsetSizeSqr);

        OutHit.Normal = Dir.GetSafeNormal();
        OutHit.ImpactNormal = OutHit.Normal;
        OutHit.ImpactPoint = Center2 + OutHit.Normal * Sphere2.GetRadius();
        OutHit.PenetrationDepth = RadiusSum - Dist;
        OutHit.bStartPentrating = true;
        OutHit.Time = 0.0f;
        OutHit.Distance = 0.0f;
        OutHit.Location = Center1;
        return true;
    }

    const float HalfB = -FVector::Dot(Offset, SweepDir);
    const float C = OffsetSizeSqr - RadiusSumSqr;

    const float QuarterUnderRoot = HalfB * HalfB - C;
    if(QuarterUnderRoot < 0.0f)
    {
        return false;
    }

    float FirstTime = QuarterUnderRoot < KINDA_SMALL_NUMBER ? -HalfB : -HalfB - std::sqrt(QuarterUnderRoot);
    if(FirstTime >= 0 && FirstTime <= SweepLength)
    {
        const FVector FinalSpherePosition = Center1 + SweepDir * FirstTime;
        const FVector FinalNormal = (FinalSpherePosition - Center2) / RadiusSum;
        const FVector IntersectionPoint = FinalSpherePosition - FinalNormal * Sphere1.GetRadius();
        
        OutHit.Distance = FirstTime;
        OutHit.Time = FirstTime / SweepLength;
        OutHit.Location = FinalSpherePosition;
        OutHit.Normal = FinalNormal;
        OutHit.ImpactNormal = OutHit.Normal;
        OutHit.ImpactPoint = IntersectionPoint;

        return true;
    }


    return false;
}

bool FCollisionDetector::SphereBoxSweep(const FSphere& Sphere, const FTransform& SphereTransform, const FBox& Box, const FTransform& BoxTransform, const FVector& SweepDir, const float SweepLength, FHitResult& OutHit)
{
    const FVector SphereWorld = SphereTransform.TransformPosition(Sphere.GetCenter());
    const FVector LocalRayStart = BoxTransform.Rotation.Quaternion().Inverse().RotateVector(SphereWorld - BoxTransform.Location);
    const FVector LocalRayDir = BoxTransform.Rotation.Quaternion().Inverse().RotateVector(SweepDir);

    FVector NormalBox;
    float DistToLocalRayStart = Box.PhiWithNormal(LocalRayStart, NormalBox);
    if(DistToLocalRayStart < Sphere.GetRadius())
    {
        OutHit.Normal = BoxTransform.TransformVectorNoScale(NormalBox);
        OutHit.ImpactNormal = OutHit.Normal;
        OutHit.Distance = 0.0f;
        OutHit.Location = SphereWorld;
        OutHit.ImpactPoint = SphereWorld - OutHit.Normal * DistToLocalRayStart;
        OutHit.PenetrationDepth = Sphere.GetRadius() - DistToLocalRayStart;
        OutHit.bStartPentrating = true;
        OutHit.Time = 0.0f;
        
        return true;
    }

    const FVector MinInflated = Box.Min - FVector(Sphere.GetRadius());
    const FVector StartToMin = MinInflated - LocalRayStart;

    const FVector MaxInflated = Box.Max + FVector(Sphere.GetRadius());
    const FVector StartToMax = MaxInflated - LocalRayStart;

    float LastestStartTime = 0.0f;
    float EaliestEndtime = FLT_MAX;
    FVector Normal(0.0f);

    for(int Axis = 0; Axis < 3; Axis++)
    {
        const bool bParallel = FMath::IsNearlyZero(LocalRayDir.V[Axis]);
        float Time1, Time2;
        if(bParallel)
        {
            if(StartToMin[Axis] > 0 || StartToMax[Axis] < 0)
            {
                return false; // parallel and outside
            }
            else
            {
                Time1 = 0.0f;
                Time2 = FLT_MAX;
            }
        }
        else
        {
            const float InvRaydir = 1.0f / LocalRayDir.V[Axis];
            Time1 = StartToMin.V[Axis] * InvRaydir;
            Time2 = StartToMax.V[Axis] * InvRaydir;
        }

        FVector CurNormal = FVector::AxisVector(Axis);

        if(Time1 > Time2)
        {
            std::swap(Time1, Time2);            
        }
        else
        {
            // Hit negative plane
            CurNormal[Axis] = -1;
        }

        if(Time1 > LastestStartTime)
        {
            Normal = CurNormal;
        }
        LastestStartTime = FMath::Max(LastestStartTime, Time1);
        EaliestEndtime = FMath::Min(EaliestEndtime, Time2);

        if(LastestStartTime > EaliestEndtime)
        {
            return false; // Outside
        }

        if(LastestStartTime > SweepLength || EaliestEndtime < 0.0f)
        {
            return false; // Outside
        }
    }

    // Simple slab
    if(Normal.SizeSquared() < SMALL_NUMBER)
    {
        if(FVector::Dot(LocalRayDir, NormalBox) >= 0.0f)
        {
            return false;
        }
        else
        {
            Normal = NormalBox;
        }
    }

    OutHit.Distance = LastestStartTime;
    OutHit.Time = LastestStartTime / SweepLength;
    OutHit.Location = SphereWorld + SweepDir * LastestStartTime;
    OutHit.Normal = BoxTransform.TransformVectorNoScale(Normal).Normalized();
    OutHit.ImpactNormal = OutHit.Normal;
    OutHit.ImpactPoint = OutHit.Location - OutHit.Normal * Sphere.GetRadius();

    return true;
}

bool FCollisionDetector::BoxBoxSweep(const FBox& Box1, const FTransform& Box1Transform, const FBox& Box2, const FTransform& Box2Transform, const FVector& SweepDir, const float SweepLength, FHitResult& OutHit)
{
    const FVector Center1 = Box1Transform.Location;
    const FVector Center2 = Box2Transform.Location;
    const FVector Extent1 = Box1.GetExtent();
    const FVector Extent2 = Box2.GetExtent();

    const FVector Axis1[3] = { Box1Transform.GetForward(), Box1Transform.GetRight(), Box1Transform.GetUp() };
    const FVector Axis2[3] = { Box2Transform.GetForward(), Box2Transform.GetRight(), Box2Transform.GetUp() };
    FVector CandidateAxes[15] = {};

    const FVector D = Center1 - Center2;

    // Make candidate axes
    for(int i = 0; i < 3; i++)
    {
        CandidateAxes[i] = Axis1[i];
        CandidateAxes[i + 3] = Axis2[i];

        for(int j = 0;j < 3;j++)
        {
            CandidateAxes[6 + i * 3 + j] = FVector::Cross(Axis1[i], Axis2[j]).Normalized();
        }
    }

    float LastestStartTime = 0.0f;
    float EaliestEndtime = FLT_MAX;
    bool bInitiallyOverlapping = true;
    float MinPenetration = FLT_MAX;
    FVector InitialNormal(0.0f);
    FVector HitNormal(0.0f);  

    for(int i = 0; i < 15;i++)
    {
        if(CandidateAxes[i].SizeSquared() <= KINDA_SMALL_NUMBER)
        {
            continue;
        }
        float Time1, Time2;
        const FVector L = CandidateAxes[i].Normalized();

        float R1 = 0.0f;
        float R2 = 0.0f;
        for(int j = 0; j < 3; j++)
        {
            R1 += Extent1.V[j] * std::abs(FVector::Dot(Axis1[j], L));
            R2 += Extent2.V[j] * std::abs(FVector::Dot(Axis2[j], L));
        }
        float R = R1 + R2;

        const float S0 = FVector::Dot(D, L);
        const float AbsS0 = std::abs(S0);
        const float V = FVector::Dot(SweepDir, L);

        if(AbsS0 < R && R - AbsS0 < MinPenetration)
        {
            InitialNormal = S0 >= 0 ? L : -L;
            MinPenetration = R - AbsS0;
        }
        else if(AbsS0 >= R)
        {
            bInitiallyOverlapping = false;
        }

        const bool bParallel = FMath::IsNearlyZero(V);
        if(bParallel)
        {
            if(AbsS0 > R)
            {
                return false;
            }
            else
            {
                Time1 = 0.0f;
                Time2 = FLT_MAX;
            }
        }
        else
        {
            const float InvV = 1.0f / V;
            Time1 = (-R - S0) * InvV;
            Time2 = (R - S0) * InvV;
        }

        FVector CurNormal = V > 0 ? -L : L;

        if(Time1 > Time2)
        {
            std::swap(Time1, Time2);
        }

        if(Time1 > LastestStartTime || (AbsS0 >= R && Time1 >= LastestStartTime))
        {
            HitNormal = CurNormal;
        }
        LastestStartTime = FMath::Max(LastestStartTime, Time1);
        EaliestEndtime = FMath::Min(EaliestEndtime, Time2);

        if(LastestStartTime > EaliestEndtime)
        {
            return false; // Outside
        }

        if(LastestStartTime > SweepLength || EaliestEndtime < 0.0f)
        {
            return false; // Outside
        }
    }

    if(bInitiallyOverlapping)
    {
        OutHit.bStartPentrating = true;
        OutHit.Time = 0.0f;
        OutHit.Distance = 0.0f;
        OutHit.Location = Center1;
        OutHit.PenetrationDepth = MinPenetration;
        OutHit.Normal = InitialNormal;
        OutHit.ImpactNormal = OutHit.Normal;
        return true;
    }
    else
    {
        OutHit.Distance = LastestStartTime;
        OutHit.Time = LastestStartTime / SweepLength;
        OutHit.Location = Center1 + SweepDir * LastestStartTime;
        OutHit.Normal = HitNormal;
        OutHit.ImpactNormal = OutHit.Normal;
        return true;
    }
}

bool FCollisionDetector::SphereCapsuleSweep(const FSphere& Sphere, const FTransform& SphereTransform, const FCapsule& Capsule, const FTransform& CapsuleTransform, const FVector& SweepDir, const float SweepLength, FHitResult& OutHit)
{
    const FVector Center = SphereTransform.TransformPosition(Sphere.GetCenter());
    FVector Seg0, Seg1;
    Capsule.GetSegment(CapsuleTransform, Seg0, Seg1);

    const float RadiusSum = Sphere.GetRadius() + Capsule.GetRadius();
    const float RadiusSumSqr = RadiusSum * RadiusSum;

    const FVector ClosestOnSeg = FMathUtil::ClosestPointOnSegment(Center, Seg0, Seg1);
    const FVector InitialOffset = Center - ClosestOnSeg;
    const float InitialDistSqr = InitialOffset.SizeSquared();

    if (InitialDistSqr < RadiusSumSqr)
    {
        const float Dist = std::sqrt(InitialDistSqr);
        OutHit.bStartPentrating = true;
        OutHit.Time = 0.0f;
        OutHit.Distance = 0.0f;
        OutHit.Location = Center;
        OutHit.PenetrationDepth = RadiusSum - Dist;
        OutHit.Normal = Dist > KINDA_SMALL_NUMBER ? InitialOffset / Dist : CapsuleTransform.GetUp();
        OutHit.ImpactNormal = OutHit.Normal;
        OutHit.ImpactPoint = ClosestOnSeg + OutHit.Normal * Capsule.GetRadius();
        return true;
    }

    float EarliestHitTime = FLT_MAX;
    FVector BestNormal(0.0f, 0.0f, 1.0f);

    const FVector SegDelta = Seg1 - Seg0;
    const float SegLength = SegDelta.Size();

    if (SegLength > KINDA_SMALL_NUMBER)
    {
        const FVector SegAxis = SegDelta / SegLength;
        const FVector M = Center - Seg0;

        const FVector D_perp = SweepDir - SegAxis * FVector::Dot(SweepDir, SegAxis);
        const FVector M_perp = M - SegAxis * FVector::Dot(M, SegAxis);

        const float A = D_perp.SizeSquared();
        const float B = 2.0f * FVector::Dot(D_perp, M_perp);
        const float C = M_perp.SizeSquared() - RadiusSumSqr;

        if (A > SMALL_NUMBER)
        {
            const float Discr = B * B - 4.0f * A * C;
            if (Discr >= 0.0f)
            {
                const float SqrtDiscr = std::sqrt(Discr);
                const float T = (-B - SqrtDiscr) / (2.0f * A);
                if (T >= 0.0f && T <= SweepLength)
                {
                    const FVector HitPos = Center + SweepDir * T;
                    const float Z = FVector::Dot(HitPos - Seg0, SegAxis);
                    if (Z >= 0.0f && Z <= SegLength)
                    {
                        EarliestHitTime = T;
                        const FVector AxisPoint = Seg0 + SegAxis * Z;
                        BestNormal = (HitPos - AxisPoint).GetSafeNormal();
                    }
                }
            }
        }
    }

    const FVector CapSpheres[2] = { Seg0, Seg1 };
    for (int32 i = 0; i < 2; ++i)
    {
        const FVector Offset = CapSpheres[i] - Center;
        const float OffsetSizeSqr = Offset.SizeSquared();

        const float HalfB = -FVector::Dot(Offset, SweepDir);
        const float C = OffsetSizeSqr - RadiusSumSqr;

        const float QuarterUnderRoot = HalfB * HalfB - C;
        if (QuarterUnderRoot >= 0.0f)
        {
            const float FirstTime = QuarterUnderRoot < KINDA_SMALL_NUMBER ? -HalfB : -HalfB - std::sqrt(QuarterUnderRoot);
            if (FirstTime >= 0.0f && FirstTime <= SweepLength && FirstTime < EarliestHitTime)
            {
                EarliestHitTime = FirstTime;
                const FVector FinalSpherePos = Center + SweepDir * FirstTime;
                BestNormal = (FinalSpherePos - CapSpheres[i]) / RadiusSum;
            }
        }
    }

    if (EarliestHitTime <= SweepLength)
    {
        const FVector FinalPos = Center + SweepDir * EarliestHitTime;
        OutHit.Distance = EarliestHitTime;
        OutHit.Time = EarliestHitTime / SweepLength;
        OutHit.Location = FinalPos;
        OutHit.Normal = BestNormal.GetSafeNormal();
        OutHit.ImpactNormal = OutHit.Normal;
        OutHit.ImpactPoint = FinalPos - OutHit.Normal * Sphere.GetRadius();
        return true;
    }

    return false;
}

bool FCollisionDetector::CapsuleCapsuleSweep(const FCapsule& Capsule1, const FTransform& Capsule1Transform, const FCapsule& Capsule2, const FTransform& Capsule2Transform, const FVector& SweepDir, const float SweepLength, FHitResult& OutHit)
{
    const FVector Center1 = Capsule1Transform.Location;
    const FVector Center2 = Capsule2Transform.Location;

    const FVector Capsule1Axis = Capsule1Transform.GetUp();
    const FVector Capsule2Axis = Capsule2Transform.GetUp();

    const float HalfHeight1 = FMath::Max(0.0f, Capsule1.GetHalfHeight() - Capsule1.GetRadius());
    const float HalfHeight2 = FMath::Max(0.0f, Capsule2.GetHalfHeight() - Capsule2.GetRadius());

    const float Radius1 = Capsule1.GetRadius();
    const float Radius2 = Capsule2.GetRadius();

    const FVector Axis1[3] = { Capsule1Transform.GetForward(), Capsule1Transform.GetRight(), Capsule1Axis };
    const FVector Axis2[3] = { Capsule2Transform.GetForward(), Capsule2Transform.GetRight(), Capsule2Axis };

    FVector CandidateAxes[15] = {};
    const FVector D = Center1 - Center2;

    for (int32 i = 0; i < 3; ++i)
    {
        CandidateAxes[i] = Axis1[i];
        CandidateAxes[i + 3] = Axis2[i];

        for (int32 j = 0; j < 3; ++j)
        {
            CandidateAxes[6 + i * 3 + j] = FVector::Cross(Axis1[i], Axis2[j]).Normalized();
        }
    }

    float LastestStartTime = 0.0f;
    float EaliestEndtime = FLT_MAX;
    bool bInitiallyOverlapping = true;
    float MinPenetration = FLT_MAX;
    FVector InitialNormal(0.0f);
    FVector HitNormal(0.0f);

    for (int32 i = 0; i < 15; ++i)
    {
        if (CandidateAxes[i].SizeSquared() <= KINDA_SMALL_NUMBER)
        {
            continue;
        }

        const FVector L = CandidateAxes[i].Normalized();

        const float R1 = Radius1 + HalfHeight1 * std::abs(FVector::Dot(Capsule1Axis, L));
        const float R2 = Radius2 + HalfHeight2 * std::abs(FVector::Dot(Capsule2Axis, L));
        const float R = R1 + R2;

        const float S0 = FVector::Dot(D, L);
        const float AbsS0 = std::abs(S0);
        const float V = FVector::Dot(SweepDir, L);

        if (AbsS0 < R && R - AbsS0 < MinPenetration)
        {
            InitialNormal = S0 >= 0 ? L : -L;
            MinPenetration = R - AbsS0;
        }
        else if (AbsS0 >= R)
        {
            bInitiallyOverlapping = false;
        }

        const bool bParallel = FMath::IsNearlyZero(V);
        float Time1, Time2;
        if (bParallel)
        {
            if (AbsS0 > R)
            {
                return false;
            }
            else
            {
                Time1 = 0.0f;
                Time2 = FLT_MAX;
            }
        }
        else
        {
            const float InvV = 1.0f / V;
            Time1 = (-R - S0) * InvV;
            Time2 = (R - S0) * InvV;
        }

        const FVector CurNormal = V > 0 ? -L : L;

        if (Time1 > Time2)
        {
            std::swap(Time1, Time2);
        }

        if (Time1 > LastestStartTime || (AbsS0 >= R && Time1 >= LastestStartTime))
        {
            HitNormal = CurNormal;
        }
        LastestStartTime = FMath::Max(LastestStartTime, Time1);
        EaliestEndtime = FMath::Min(EaliestEndtime, Time2);

        if (LastestStartTime > EaliestEndtime)
        {
            return false;
        }

        if (LastestStartTime > SweepLength || EaliestEndtime < 0.0f)
        {
            return false;
        }
    }

    if (bInitiallyOverlapping)
    {
        OutHit.bStartPentrating = true;
        OutHit.Time = 0.0f;
        OutHit.Distance = 0.0f;
        OutHit.Location = Center1;
        OutHit.PenetrationDepth = MinPenetration;
        OutHit.Normal = InitialNormal;
        OutHit.ImpactNormal = OutHit.Normal;
        OutHit.ImpactPoint = Center1 - InitialNormal * Radius1;
        return true;
    }
    else
    {
        OutHit.Distance = LastestStartTime;
        OutHit.Time = LastestStartTime / SweepLength;
        OutHit.Location = Center1 + SweepDir * LastestStartTime;
        OutHit.Normal = HitNormal;
        OutHit.ImpactNormal = OutHit.Normal;
        OutHit.ImpactPoint = OutHit.Location - HitNormal * Radius1;
        return true;
    }
}

bool FCollisionDetector::CapsuleBoxSweep(const FCapsule& Capsule, const FTransform& CapsuleTransform, const FBox& Box, const FTransform& BoxTransform, const FVector& SweepDir, const float SweepLength, FHitResult& OutHit)
{
    const FVector Center1 = CapsuleTransform.Location;
    const FVector Center2 = BoxTransform.Location;
    const FVector Extent2 = Box.GetExtent();

    const FVector CapsuleAxis = CapsuleTransform.GetUp();
    const float HalfHeight = FMath::Max(0.0f, Capsule.GetHalfHeight() - Capsule.GetRadius());
    const float Radius = Capsule.GetRadius();

    const FVector Axis2[3] = { BoxTransform.GetForward(), BoxTransform.GetRight(), BoxTransform.GetUp() };
    FVector CandidateAxes[15] = {};

    const FVector D = Center1 - Center2;

    CandidateAxes[0] = CapsuleTransform.GetForward();
    CandidateAxes[1] = CapsuleTransform.GetRight();
    CandidateAxes[2] = CapsuleAxis;

    CandidateAxes[3] = Axis2[0];
    CandidateAxes[4] = Axis2[1];
    CandidateAxes[5] = Axis2[2];

    for (int32 i = 0; i < 3; ++i)
    {
        for (int32 j = 0; j < 3; ++j)
        {
            CandidateAxes[6 + i * 3 + j] = FVector::Cross(CandidateAxes[i], Axis2[j]).Normalized();
        }
    }

    float LastestStartTime = 0.0f;
    float EaliestEndtime = FLT_MAX;
    bool bInitiallyOverlapping = true;
    float MinPenetration = FLT_MAX;
    FVector InitialNormal(0.0f);
    FVector HitNormal(0.0f);

    for (int32 i = 0; i < 15; ++i)
    {
        if (CandidateAxes[i].SizeSquared() <= KINDA_SMALL_NUMBER)
        {
            continue;
        }

        const FVector L = CandidateAxes[i].Normalized();

        const float R1 = Radius + HalfHeight * std::abs(FVector::Dot(CapsuleAxis, L));
        float R2 = 0.0f;
        for (int32 j = 0; j < 3; ++j)
        {
            R2 += Extent2.V[j] * std::abs(FVector::Dot(Axis2[j], L));
        }
        const float R = R1 + R2;

        const float S0 = FVector::Dot(D, L);
        const float AbsS0 = std::abs(S0);
        const float V = FVector::Dot(SweepDir, L);

        if (AbsS0 < R && R - AbsS0 < MinPenetration)
        {
            InitialNormal = S0 >= 0 ? L : -L;
            MinPenetration = R - AbsS0;
        }
        else if (AbsS0 >= R)
        {
            bInitiallyOverlapping = false;
        }

        const bool bParallel = FMath::IsNearlyZero(V);
        float Time1, Time2;
        if (bParallel)
        {
            if (AbsS0 > R)
            {
                return false;
            }
            else
            {
                Time1 = 0.0f;
                Time2 = FLT_MAX;
            }
        }
        else
        {
            const float InvV = 1.0f / V;
            Time1 = (-R - S0) * InvV;
            Time2 = (R - S0) * InvV;
        }

        const FVector CurNormal = V > 0 ? -L : L;

        if (Time1 > Time2)
        {
            std::swap(Time1, Time2);
        }

        if (Time1 > LastestStartTime || (AbsS0 >= R && Time1 >= LastestStartTime))
        {
            HitNormal = CurNormal;
        }
        LastestStartTime = FMath::Max(LastestStartTime, Time1);
        EaliestEndtime = FMath::Min(EaliestEndtime, Time2);

        if (LastestStartTime > EaliestEndtime)
        {
            return false;
        }

        if (LastestStartTime > SweepLength || EaliestEndtime < 0.0f)
        {
            return false;
        }
    }

    if (bInitiallyOverlapping)
    {
        OutHit.bStartPentrating = true;
        OutHit.Time = 0.0f;
        OutHit.Distance = 0.0f;
        OutHit.Location = Center1;
        OutHit.PenetrationDepth = MinPenetration;
        OutHit.Normal = InitialNormal;
        OutHit.ImpactNormal = OutHit.Normal;
        OutHit.ImpactPoint = Center1 - InitialNormal * Radius;
        return true;
    }
    else
    {
        OutHit.Distance = LastestStartTime;
        OutHit.Time = LastestStartTime / SweepLength;
        OutHit.Location = Center1 + SweepDir * LastestStartTime;
        OutHit.Normal = HitNormal;
        OutHit.ImpactNormal = OutHit.Normal;
        OutHit.ImpactPoint = OutHit.Location - HitNormal * Radius;
        return true;
    }
}
