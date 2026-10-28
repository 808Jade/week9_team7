#pragma once

#include "Core/Types.h"
#include "Core/EngineString.h"

struct FStaticMeshData;

// Cook이 끝난 FStaticMeshData를 .bin으로 저장하고 읽는다.
class FStaticMeshBake
{
public:
	static TUniquePtr<FStaticMeshData> ReadBaked(const FString& BinPath, uint64 SourceSignature);
	static void WriteBaked(const FString& BinPath, const FStaticMeshData& Data, uint64 SourceSignature);
};
