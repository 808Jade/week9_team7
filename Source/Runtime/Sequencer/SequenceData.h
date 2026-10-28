#pragma once

#include "Containers/Containers.h"
#include "Core/EngineString.h"
#include "Math/EngineMath.h"

enum class ETrackType
{
    Transform,
    Fade,
    LetterBox,
    CameraShake,
	Count
};

struct FSequencerKey
{
    float Time = 0.0f;
    // Transform 트랙용
    FVector Location = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;
    FVector Scale = FVector(1.0f, 1.0f, 1.0f);
    // Fade / Letterbox / Shake강도 공용 수치
    float Value = 0.0f;
    // Shake 지속시간 등
    float Duration = 0.5f;
    bool operator<(const FSequencerKey& Other) const { return Time < Other.Time; }
};

// 단일 액터 트랙
struct FSequencerTrack
{
    FString ActorName;
    ETrackType Type = ETrackType::Transform;
    TArray<FSequencerKey> Keys;

    void SortKeys();
    bool EvaluateTransform(float Time, FVector& OutLocation, FRotator& OutRotation, FVector& OutScale) const;
    bool EvaluateFloat(float Time, float& OutValue) const;
    void AddOrUpdateFloatKey(float Time, float Value);
    void AddOrUpdateTransformKey(float Time, const FVector& Location, const FRotator& Rotation, const FVector& Scale = FVector(1.0f, 1.0f, 1.0f));
    bool RemoveKey(int32 Index);
};

// 시퀀스 에셋 데이터
class FLevelSequence
{
public:
    FString Name = "NewSequence";
    float Duration = 5.0f;
    float FrameRate = 30.0f;
    TArray<FSequencerTrack> Tracks;

    FSequencerTrack* FindTrack(const FString& InActorName);
    FSequencerTrack* AddTrack(const FString& InActorName, ETrackType InType = ETrackType::Transform);
    bool RemoveTrack(const FString& InActorName);

    // JSON 파일 저장 및 불러오기
    bool SaveToFile(const FString& FilePath) const;
    bool LoadFromFile(const FString& FilePath);

    FString CameraCutTarget = "";
    float CameraBlendTime = 0.0f;
};
