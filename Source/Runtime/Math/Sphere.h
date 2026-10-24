#pragma once

#include "Matrix.h"
#include "Vector.h"
#include <algorithm>
#include <cmath>
#include <limits>

// A common bound for all mesh LODs, independent of the selected LOD or view.
struct FLODSphere
{
    FVector Center;
    float RadiusSquared = 0.0f;

    FLODSphere Transform(const FMatrix& World) const
    {
        FLODSphere Result;
        Result.Center = World.TransformPosition(Center);
        // Gershgorin bound on A*A^T: exact for orthogonal TRS axes and
        // conservative for shear from composed, non-uniform transforms.
        const auto DotRows = [&World](int I, int J)
        {
            return static_cast<double>(World.M[I][0]) * World.M[J][0]
                + static_cast<double>(World.M[I][1]) * World.M[J][1]
                + static_cast<double>(World.M[I][2]) * World.M[J][2];
        };
        const double XY = std::abs(DotRows(0, 1));
        const double XZ = std::abs(DotRows(0, 2));
        const double YZ = std::abs(DotRows(1, 2));
        const double MaxStretchSquared = std::max({
            DotRows(0, 0) + XY + XZ,
            DotRows(1, 1) + XY + YZ,
            DotRows(2, 2) + XZ + YZ });
        Result.RadiusSquared = std::nextafter(
            static_cast<float>(static_cast<double>(RadiusSquared) * MaxStretchSquared),
            std::numeric_limits<float>::infinity());
        return Result;
    }
};

class FSphere
{
public:
    FSphere() : Center(0.0f, 0.0f, 0.0f), Radius(0.0f) {}
    FSphere(const FVector& InCenter, const float InRadius) : Center(InCenter), Radius(InRadius) {}
    FSphere(const FSphere& Other) : Center(Other.GetCenter()), Radius(Other.GetRadius()) {}

    const FVector& GetCenter() const
    {
        return Center;
    }
    void SetCenter(const FVector& InCenter)
    {
        Center = InCenter;
    }

    const float GetRadius() const
    {
        return Radius;
    }
    void SetRadius(const float InRadius)
    {
        Radius = InRadius;
    }

private:
    FVector Center;
    float Radius;
};