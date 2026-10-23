#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>

#include "Containers/Containers.h"

// ============================================================================
// FScriptWatcher : .lua 파일 변경 감시자 (Hot Reload)
//
// - 파일이 바뀌었는지 판단해서 알려주는 일만 한다.
// - Lua / sol2 / 액터 / 컴포넌트를 모른다. (이 헤더에 sol2나 컴포넌트 헤더를 include하지 말 것)
// - 의존 방향: ULuaScriptComponent -> FScriptWatcher (반대 방향 없음)
// - 단일 스레드 폴링. 엔진 메인 루프(FEngineLoop::Tick)에서 월드 Tick보다 먼저 호출된다.
// ============================================================================

// 등록 핸들. 0은 "등록 안 됨"으로 예약한다.
using FScriptWatchHandle = uint64_t;
inline constexpr FScriptWatchHandle InvalidScriptWatchHandle = 0;

// 파일 변경 알림 콜백. 인자는 정규화된 절대 경로.
// TODO: 팀원 4의 TDelegate가 나오면 이 별칭만 교체한다.
using FOnScriptFileChanged = std::function<void(const std::filesystem::path&)>;

class FScriptWatcher
{
public:
	static FScriptWatcher& Get();

	// 파일 감시 등록. 실패 시 InvalidScriptWatchHandle을 반환한다.
	// InPath는 상대/절대 모두 가능 (상대 경로는 작업 디렉터리 = 저장소 루트 기준)
	FScriptWatchHandle Register(const std::filesystem::path& InPath, FOnScriptFileChanged InCallback);

	// 등록 해제. 무효 핸들이나 이미 해제된 핸들이 와도 안전해야 한다.
	void Unregister(FScriptWatchHandle InHandle);

	// 매 프레임 호출. 실제 경과 시간(게임 TimeScale/일시정지와 무관)을 받는다.
	void Tick(float InRealDeltaTime);

	// 감시자가 쓰는 경로 정규화: 절대 경로 + lexically_normal
	// (FLuaScriptUtils::MakeAbsolutePath와 같은 규칙)
	static std::filesystem::path NormalizePath(const std::filesystem::path& InPath);

	// 검사 주기(초)
	static constexpr float CheckInterval = 0.5f;

private:
	FScriptWatcher() = default;
	FScriptWatcher(const FScriptWatcher&) = delete;
	FScriptWatcher& operator=(const FScriptWatcher&) = delete;

	FScriptWatchHandle NextHandle = 1;

	struct FWatchSubscriber
	{
		FScriptWatchHandle Handle = InvalidScriptWatchHandle;
		FOnScriptFileChanged Callback;
	};

	// 감시하고 있는 파일의 정보
	struct FWatchedFile
	{
		std::filesystem::path Path;
		std::filesystem::file_time_type AppliedWriteTime;
		bool bIsTimeValid = false;
		bool bIsWaiting = false;
		std::filesystem::file_time_type PendingWriteTime;
		TArray<FWatchSubscriber> Subscribers;
	};

	//   - 키: 정규화된 경로 (소문자)
	//   - 값: 마지막 수정 시간, 구독자 목록(핸들 + 콜백), 디바운스 대기 플래그/시간
	TMap<std::wstring, FWatchedFile> FileMap;
	//   - 핸들 -> 경로 역참조 (Unregister에서 사용)
	TMap<FScriptWatchHandle, std::wstring> HandleMap;

	// path를 받아 소문자화 된 wstring을 반환
	static std::wstring MakeKey(const std::filesystem::path& InPath);

	// TODO(3단계): 검사 주기 누적 시간
	float AccumulatedTime = 0.f;
};
