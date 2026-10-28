#include "EnginePCH.h"
#include "Math/Box.h"
#include "Math/Vector.h"

bool FMathUtil::LineExtentBoxIntersection(const FBox& InBox, const FVector& Start, const FVector& End, const FVector& Extent, FVector& HitLocation, FVector& HitNormal, float& HitTime)
{
    FBox Box = InBox;
    Box.Max.X += Extent.X;
    Box.Max.Y += Extent.Y;
    Box.Max.Z += Extent.Z;

    Box.Min.X -= Extent.X;
    Box.Min.Y -= Extent.Y;
    Box.Min.Z -= Extent.Z;

    const FVector Dir = End - Start;

    FVector Time;
    bool Inside = false;
    float FaceDir[3] = { 1,1,1 };

	/////////////// X
	if (Start.X < Box.Min.X)
	{
		if (Dir.X <= 0.0f)
			return false;
		else
		{
			Inside = 0;
			FaceDir[0] = -1;
			Time.X = (Box.Min.X - Start.X) / Dir.X;
		}
	}
	else if (Start.X > Box.Max.X)
	{
		if (Dir.X >= 0.0f)
			return false;
		else
		{
			Inside = 0;
			Time.X = (Box.Max.X - Start.X) / Dir.X;
		}
	}
	else
		Time.X = 0.0f;

	/////////////// Y
	if (Start.Y < Box.Min.Y)
	{
		if (Dir.Y <= 0.0f)
			return false;
		else
		{
			Inside = 0;
			FaceDir[1] = -1;
			Time.Y = (Box.Min.Y - Start.Y) / Dir.Y;
		}
	}
	else if (Start.Y > Box.Max.Y)
	{
		if (Dir.Y >= 0.0f)
			return false;
		else
		{
			Inside = 0;
			Time.Y = (Box.Max.Y - Start.Y) / Dir.Y;
		}
	}
	else
		Time.Y = 0.0f;

	/////////////// Z
	if (Start.Z < Box.Min.Z)
	{
		if (Dir.Z <= 0.0f)
			return false;
		else
		{
			Inside = 0;
			FaceDir[2] = -1;
			Time.Z = (Box.Min.Z - Start.Z) / Dir.Z;
		}
	}
	else if (Start.Z > Box.Max.Z)
	{
		if (Dir.Z >= 0.0f)
			return false;
		else
		{
			Inside = 0;
			Time.Z = (Box.Max.Z - Start.Z) / Dir.Z;
		}
	}
	else
		Time.Z = 0.0f;

	// If the line started inside the box
	if (Inside)
	{
		HitLocation = Start;
		HitNormal = FVector(0, 0, 1);
		HitTime = 0;
		return true;
	}
	// Otherwise, calculate when hit occured
	else
	{
		if (Time.Y > Time.Z)
		{
			HitTime = static_cast<std::remove_reference_t<decltype(HitTime)>>(Time.Y);	// LWC_TODO: Remove decltype
			HitNormal = FVector(0, FaceDir[1], 0);
		}
		else
		{
			HitTime = static_cast<std::remove_reference_t<decltype(HitTime)>>(Time.Z);
			HitNormal = FVector(0, 0, FaceDir[2]);
		}

		if (Time.X > HitTime)
		{
			HitTime = static_cast<std::remove_reference_t<decltype(HitTime)>>(Time.X);
			HitNormal = FVector(FaceDir[0], 0, 0);
		}

		if (HitTime >= 0.0f && HitTime <= 1.0f)
		{
			HitLocation = Start + Dir * HitTime;
			const float BOX_SIDE_THRESHOLD = 0.1f;
			if (HitLocation.X > Box.Min.X - BOX_SIDE_THRESHOLD && HitLocation.X < Box.Max.X + BOX_SIDE_THRESHOLD &&
				HitLocation.Y > Box.Min.Y - BOX_SIDE_THRESHOLD && HitLocation.Y < Box.Max.Y + BOX_SIDE_THRESHOLD &&
				HitLocation.Z > Box.Min.Z - BOX_SIDE_THRESHOLD && HitLocation.Z < Box.Max.Z + BOX_SIDE_THRESHOLD)
			{
				return true;
			}
		}

		return false;
	}

    return false;
}


FVector FMathUtil::ClosestPointOnSegment(const FVector& Point, const FVector& StartPoint, const FVector& EndPoint)
{
	const FVector Segment = EndPoint - StartPoint;
	const FVector Vect = Point - StartPoint;
	const float LengthSq = Segment.SizeSquared();

	if (LengthSq <= SMALL_NUMBER)
	{
		return StartPoint;
	}

	const float T = FMath::Clamp(FVector::Dot(Vect, Segment) / LengthSq, 0.0f, 1.0f);
	return StartPoint + Segment * T;
}

void FMathUtil::SegmentDistToSegment(FVector A1, FVector B1, FVector A2, FVector B2, FVector& OutP1, FVector& OutP2)
{
	const FVector D1 = B1 - A1;
	const FVector D2 = B2 - A2;
	const FVector R = A1 - A2;

	const float A = D1.SizeSquared();
	const float E = D2.SizeSquared();
	const float F = FVector::Dot(D2, R);

	float S = 0.0f;
	float T = 0.0f;

	if (A <= SMALL_NUMBER && E <= SMALL_NUMBER)
	{
		OutP1 = A1;
		OutP2 = A2;
		return;
	}

	if (A <= SMALL_NUMBER)
	{
		S = 0.0f;
		T = FMath::Clamp(F / E, 0.0f, 1.0f);
	}
	else
	{
		const float C = FVector::Dot(D1, R);
		if (E <= SMALL_NUMBER)
		{
			T = 0.0f;
			S = FMath::Clamp(-C / A, 0.0f, 1.0f);
		}
		else
		{
			const float B = FVector::Dot(D1, D2);
			const float Denom = A * E - B * B;

			if (Denom != 0.0f)
			{
				S = FMath::Clamp((B * F - C * E) / Denom, 0.0f, 1.0f);
			}
			else
			{
				S = 0.0f;
			}

			T = (B * S + F) / E;

			if (T < 0.0f)
			{
				T = 0.0f;
				S = FMath::Clamp(-C / A, 0.0f, 1.0f);
			}
			else if (T > 1.0f)
			{
				T = 1.0f;
				S = FMath::Clamp((B - C) / A, 0.0f, 1.0f);
			}
		}
	}

	OutP1 = A1 + D1 * S;
	OutP2 = A2 + D2 * T;
}
