#pragma once

#include <fmod.hpp>
#include <fmod_errors.h>

#include "Core/EngineLog.h"

#include "Math/Vector.h"

inline FMOD_VECTOR ToFMODVector(const FVector& InVec)
{
	return FMOD_VECTOR{ InVec.Y, InVec.Z, InVec.X };
}

inline bool FMODCheck(const FMOD_RESULT Result)
{
	if(Result != FMOD_OK && Result != FMOD_ERR_INVALID_HANDLE && Result != FMOD_ERR_CHANNEL_STOLEN)
	{
		HTR_LOG(Error, "FMOD Error : {}", FMOD_ErrorString(Result));
		return false;
	}
	else
	{
		return true;
	}
}