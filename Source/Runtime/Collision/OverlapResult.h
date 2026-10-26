#pragma once

#include "Core/Types.h"
#include "Math/Vector.h"

class UPrimitiveComponent;
class AActor;

struct FOverlapResult
{
	bool bBlockingHit = false;

	AActor* OverlappedActor = nullptr;
	UPrimitiveComponent* Component = nullptr;

	FOverlapResult()
	{
		bBlockingHit = false;
		OverlappedActor = nullptr;
		Component = nullptr;
	}
};