#include "EnginePCH.h"
#include "CollisionDetector.h"
#include "HitResult.h"
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
    // Box-Box
    if(Shape1.IsBox() && Shape2.IsBox())
    {
        const FVector& InMax1 = Shape1.GetBox();
        const FVector& InMin1 = -Shape1.GetBox();

        const FVector& InMax2 = Shape2.GetBox();
        const FVector& InMin2 = -Shape2.GetBox();

        return BoxBoxContactPoint(FBox(InMin1, InMax1), Shape1Transform, FBox(InMin2, InMax2), Shape2Transform, CullDistance);
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
