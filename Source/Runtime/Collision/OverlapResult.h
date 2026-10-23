#pragma once

#include "Core/Types.h"
#include "Math/Vector.h"

class UPrimitiveComponent;
class AActor;

struct FOverlapResult
{
	bool bBlockingHit = false;

	AActor* OverlappedActor;
	UPrimitiveComponent* Component = nullptr;
};