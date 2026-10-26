#pragma once

#include "CollisionShape.h"
#include "ContactPoint.h"
#include "Math/Transform.h"

class FSphere;
class FBox;
class FCapsule;
struct FHitResult;

class FCollisionDetector
{
public:
	static FContactPoint ComputeContactPoint(const FCollisionShape& Shape1, const FTransform& Shape1Transform, const FCollisionShape& Shape2, const FTransform& Shape2Transform, const float CullDistance = 0.0f);

	static bool IsOverlapping(const FCollisionShape& Shape1, const FTransform& Shape1Transform, const FCollisionShape& Shape2, const FTransform& Shape2Transform, const float CullDistance = 0.0f);

	static FContactPoint SphereSphereContactPoint(const FSphere& Sphere1, const FTransform& Sphere1Transform, const FSphere& Sphere2, const FTransform& Sphere2Transform, const float CullDistance = 0.0f);
	static FContactPoint SphereBoxContactPoint(const FSphere& Sphere, const FTransform& SphereTransform, const FBox& Box, const FTransform& BoxTransform, const float CullDistance = 0.0f);
	static FContactPoint SphereCapsuleContactPoint(const FSphere& Sphere, const FTransform& SphereTransform, const FCapsule& Capsule, const FTransform& CapsuleTransform, const float CullDistance = 0.0f);

	static FContactPoint BoxBoxContactPoint(const FBox& Box1, const FTransform& Box1Transform, const FBox& Box2, const FTransform& Box2Transform, const float CullDistance = 0.0f);

	static FContactPoint CapsuleCapsuleContactPoint(const FCapsule& Capsule1, const FTransform& Capsule1Transform, const FCapsule& Capsule2, const FTransform& Capsule2Transform, const float CullDistance = 0.0f);
	static FContactPoint CapsuleBoxContactPoint(const FCapsule& Capsule, const FTransform& CapsuleTranform, const FBox& Box, const FTransform& BoxTransform, const float CullDistance = 0.0f);

	static bool Sweep(const FCollisionShape& Shape1, const FTransform& Shape1Transform, const FCollisionShape& Shape2, const FTransform& Shape2Transform, const FVector& SweepDir, const float SweepLength, FHitResult& OutHit);

	static bool SphereSphereSweep(const FSphere& Sphere1, const FTransform& Sphere1Transform, const FSphere& Sphere2, const FTransform& Sphere2Transform, const FVector& SweepDir, const float SweepLength, FHitResult& OutHit);
	static bool SphereBoxSweep(const FSphere& Sphere, const FTransform& SphereTransform, const FBox& Box, const FTransform& BoxTransform, const FVector& SweepDir, const float SweepLength, FHitResult& OutHit);
	static bool SphereCapsuleSweep(const FSphere& Sphere, const FTransform& SphereTransform, const FCapsule& Capsule, const FTransform& CapsuleTransform, const FVector& SweepDir, const float SweepLength, FHitResult& OutHit);

	static bool BoxBoxSweep(const FBox& Box1, const FTransform& Box1Transform, const FBox& Box2, const FTransform& Box2Transform, const FVector& SweepDir, const float SweepLength, FHitResult& OutHit);

	static bool CapsuleCapsuleSweep(const FCapsule& Capsule1, const FTransform& Capsule1Transform, const FCapsule& Capsule2, const FTransform& Capsule2Transform, const FVector& SweepDir, const float SweepLength, FHitResult& OutHit);
	static bool CapsuleBoxSweep(const FCapsule& Capsule, const FTransform& CapsuleTransform, const FBox& Box, const FTransform& BoxTransform, const FVector& SweepDir, const float SweepLength, FHitResult& OutHit);
};