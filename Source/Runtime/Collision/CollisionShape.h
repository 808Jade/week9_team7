#pragma once
#include "Math/Vector.h"

namespace ECollisionShape
{
	enum Type
	{
		Line,
		Box,
		Sphere,
		Capsule,
	};
};

struct FCollisionShape
{
	ECollisionShape::Type ShapeType;
	
	static constexpr float MinBoxExtent() { return KINDA_SMALL_NUMBER; }
	static constexpr float MinSphereRadius() { return KINDA_SMALL_NUMBER; }
	static constexpr float MinCapsuleRadius() { return KINDA_SMALL_NUMBER; }
	
	union
	{
		struct
		{
			float HalfExtentX;
			float HalfExtentY;
			float HalfExtentZ;
		} Box;

		struct
		{
			float Raidus;
		} Sphere;

		struct
		{
			float Radius;
			float HalfHeight;
		} Capsule;
	};

	FCollisionShape()
	{
		ShapeType = ECollisionShape::Line;
	}

	bool IsLine() const
	{
		return ShapeType == ECollisionShape::Line;
	}

	bool IsBox()const
	{
		return ShapeType == ECollisionShape::Box;
	}

	bool IsSphere() const
	{
		return ShapeType == ECollisionShape::Sphere;
	}

	bool IsCapsule() const
	{
		return ShapeType == ECollisionShape::Capsule;
	}

	void SetBox(const FVector& HalfExtent)
	{
		ShapeType = ECollisionShape::Box;
		Box.HalfExtentX = HalfExtent.X;
		Box.HalfExtentY = HalfExtent.Y;
		Box.HalfExtentZ = HalfExtent.Z;
	}

	void SetSphere(const float Radius)
	{
		ShapeType = ECollisionShape::Sphere;
		Sphere.Raidus = Radius;
	}

	void SetCapsule(const float Radius, const float HalfHeight)
	{
		ShapeType = ECollisionShape::Capsule;
		Capsule.Radius = Radius;
		Capsule.HalfHeight = HalfHeight;
	}

	void SetCapsule(const FVector& Extent)
	{
		ShapeType = ECollisionShape::Capsule;
		Capsule.Radius = FMath::Max(Extent.X, Extent.Y);
		Capsule.HalfHeight = Extent.Z;
	}

	void SetShape(const ECollisionShape::Type InShapeType, const FVector& Extent)
	{
		switch (InShapeType)
		{
		case ECollisionShape::Box:
		{
			SetBox(Extent);
		}
		break;
		case ECollisionShape::Sphere:
		{
			SetSphere(Extent.X);		
		}
		break;
		case ECollisionShape::Capsule:
		{
			SetCapsule(std::max(Extent.X,Extent.Y), Extent.Z);
		}
		break;
		case ECollisionShape::Line:
		default:
			ShapeType = InShapeType;
		}
	}

	bool IsNearlyZero() const
	{
		switch (ShapeType)
		{					
		case ECollisionShape::Box:
		{
			return(Box.HalfExtentX <= FCollisionShape::MinBoxExtent() && Box.HalfExtentY <= FCollisionShape::MinBoxExtent() && Box.HalfExtentZ <= FCollisionShape::MinBoxExtent());
		}
		case ECollisionShape::Sphere:
		{
			return (Sphere.Raidus <= FCollisionShape::MinSphereRadius());
		}
		case ECollisionShape::Capsule:
		{
			return (Capsule.Radius <= FCollisionShape::MinCapsuleRadius());
		}		
		}

		return true;
	}

	FVector GetExtent() const
	{
		switch (ShapeType)
		{		
		case ECollisionShape::Box:
		{
			return FVector(Box.HalfExtentX, Box.HalfExtentY, Box.HalfExtentZ);
		}
		case ECollisionShape::Sphere:
		{
			return FVector(Sphere.Raidus, Sphere.Raidus, Sphere.Raidus);
		}
		case ECollisionShape::Capsule:
		{
			return FVector(Capsule.Radius, Capsule.Radius, Capsule.HalfHeight);
		}
		}

		return FVector::ZeroVector;
	}

	FVector GetBox() const
	{
		return FVector(Box.HalfExtentX, Box.HalfExtentY, Box.HalfExtentZ);
	}

	const float GetSphereRadius() const
	{
		return Sphere.Raidus;
	}

	const float GetCapsuleRadius() const
	{
		return Capsule.Radius;
	}

	const float GetCapsuleHalfHeight() const
	{
		return Capsule.HalfHeight;
	}

	FCollisionShape Inflate(const float Inflation) const
	{
		FCollisionShape InflatedShape;

		if(Inflation == 0.0f)
		{
			InflatedShape = *this;
		}
		else
		{
			switch(ShapeType)
			{
			case ECollisionShape::Box:
			{
				const FVector InflatedExtent = FVector(Box.HalfExtentX + Inflation, Box.HalfExtentY + Inflation, Box.HalfExtentZ + Inflation);
				InflatedShape.SetBox(InflatedExtent);
			}
				break;
			case ECollisionShape::Sphere:
			{
				InflatedShape.SetSphere(FMath::Max(Sphere.Raidus + Inflation, 0.0f));
			}
				break;
			case ECollisionShape::Capsule:
			{
				InflatedShape.SetCapsule(FMath::Max(Capsule.Radius + Inflation, 0.0f), FMath::Max(Capsule.HalfHeight + Inflation, 0.0f));
			}
				break;
			case ECollisionShape::Line:
			default:
			{
				InflatedShape = *this;
			}
			}
		}

		return InflatedShape;
	}

	static struct FCollisionShape LineShape;

	static FCollisionShape MakeBox(const FVector& BoxHalfExtent)
	{
		FCollisionShape BoxShpe;
		BoxShpe.SetBox(BoxHalfExtent);
		return BoxShpe;
	}

	static FCollisionShape MakeSphere(const float SphereRaidus)
	{
		FCollisionShape SphereShape;
		SphereShape.SetSphere(SphereRaidus);
		return SphereShape;
	}
	
	static FCollisionShape MakeCapsule(const float CapsulRadius, const float CapsulHalfHeight)
	{
		FCollisionShape CapsuleShape;
		CapsuleShape.SetCapsule(CapsulRadius, CapsulHalfHeight);
		return CapsuleShape;
	}

	static FCollisionShape MakeCapsule(const FVector& Extent)
	{
		FCollisionShape CapsuleShape;
		CapsuleShape.SetCapsule(Extent);
		return CapsuleShape;
	}
};