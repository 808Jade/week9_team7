#pragma once

#include "Containers/Containers.h"
#include "Core/EngineString.h"
#include "Math/EngineMath.h"

// 단일 키 프레임
struct FTransformKey
{
    float Time = 0.0f;
    FVector Location = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;
    FVector Scale = FVector(1.0f, 1.0f, 1.0f);
    bool operator<(const FTransformKey& Other) const { return Time < Other.Time; }
};
// 단일 액터 트랙
struct FSequencerTrack
{
	FString ActorName;
    TArray<FTransformKey> Keys;

    void SortKeys();
	bool Evaluate(float Time, FVector& OutLocation, FRotator& OutRotation, FVector& OutScale) const;
	void  AddOrUpdateKey(float Time, const FVector& Location, const FRotator& Rotation, const FVector& Scale = FVector(1.0f, 1.0f, 1.0f));
	bool RemoveKey(int32 Index);
};
// 시퀀스 에셋 데이터
class FLevelSequence
{
public:
	FString Name = "NewSequence";
	float Duration = 10.0f;
	float FrameRate = 30.0f;
	TArray<FSequencerTrack> Tracks;
	
	FSequencerTrack* FindTrack(const FString& InActorName);
	FSequencerTrack* AddTrack(const FString& InActorName);
	bool RemoveTrack(const FString& InActorName);
};