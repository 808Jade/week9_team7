#pragma once
#include "Core/Types.h"
#include "SequenceData.h"
#include <memory>

class UWorld;

class FSequencePlayer
{
public:
	FSequencePlayer();
	~FSequencePlayer() = default;

	void SetSequence(TSharedPtr<FLevelSequence> InSequence) { Sequence = InSequence; }
	TSharedPtr<FLevelSequence> GetSequence() const { return Sequence; }

	// 재생 제어
	void Play();
	void Pause();
	void Stop(UWorld* World = nullptr);
	// 타임 라인 드래그 용도
	void ScrubTo(float InTime, UWorld* World = nullptr);

	void Tick(float DeltaTime, UWorld* World);
	void Evaluate(UWorld* World);

	float GetCurrentTime() const { return CurrentTime; }
	void SetCurrentTime(float InTime) { CurrentTime = InTime; }

	bool IsPlaying() const { return bIsPlaying; }
	bool IsLooping() const { return bIsLooping; }
	void SetLooping(bool bLoop) { bIsLooping = bLoop; }

private:
	TSharedPtr<FLevelSequence> Sequence;
	float CurrentTime = 0.0f;
	float PlayRate = 1.0f;
	bool bIsPlaying = false;
	bool bIsLooping = true;

	AActor* OriginalViewTarget = nullptr; // 원래 플레이어 시점 백업용
	bool bHasCameraCutActive = false;     // 카메라 컷이 활성화되었는지 플래그
	// 시점 전환 및 복구 헬퍼 함수
	void ApplyCameraCut(UWorld* World);
	void RestoreCameraCut(UWorld* World);
};