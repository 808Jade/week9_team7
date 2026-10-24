#pragma once

#include "Core/EngineString.h"
#include <functional>

class UObject;
class UClass;

struct FFunction
{
	FString Name;
	UClass* OwnerClass = nullptr;
	std::function<void(UObject* Target, void* Params)> Invoker;
};