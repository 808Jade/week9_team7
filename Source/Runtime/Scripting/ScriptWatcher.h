#pragma once

#include <cstdint>
#include <filesystem>

#include "Containers/Containers.h"
#include "Core/Delegate.h"

// ============================================================================
// FScriptWatcher : .lua 파일 변경 감시자 (Hot Reload)
//
// - 파일이 바뀌었는지 판단해서 알려주는 일만 한다.
// - Lua / sol2 / 액터 / 컴포넌트를 모른다. 
// - 의존 방향: ULuaScriptComponent -> FScriptWatcher 
// - 단일 스레드 폴링. 엔진 메인 루프(FEngineLoop::Tick)에서 월드 Tick보다 먼저 호출된다.
// ============================================================================

DECLARE_DELEGATE(FOnScriptFileChanged, const std::filesystem::path&)

class FScriptWatcher
{
public:
	static FScriptWatcher& Get();

	// Register: 파일 감시 등록. 실패 시 무효 핸들(IsValid() == false)을 반환한다.

	// (1) 람다 / 일반 함수용 -> TDelegate::Add
	FDelegateHandle Register(const std::filesystem::path& InPath, FOnScriptFileChanged::FFuncType InCallback);

	// (2) 멤버 함수용 -> TDelegate::AddDynamic
	//     해제 책임은 호출한 쪽에 있다. Instance가 사라지기 전에 반드시 Unregister(핸들)을 부를 것
	template<typename UserClass>
	FDelegateHandle Register(const std::filesystem::path& InPath, UserClass* Instance, void(UserClass::* Func)(const std::filesystem::path&))
	{
		std::wstring Key;
		bool bCreatedFile = false;
		FWatchedFile* File = FindOrAddFile(InPath, Key, bCreatedFile);
		if (!File)
		{
			return FDelegateHandle{};
		}

		// Instance나 Func가 null이면 AddDynamic이 무효 핸들을 돌려준다 -> CommitRegister가 정리
		const FDelegateHandle Handle = File->Subscribers.AddDynamic(Instance, Func);
		return CommitRegister(Key, Handle, bCreatedFile);
	}

	// 등록 해제. 무효 핸들이나 이미 해제된 핸들이 와도 안전해야 한다.
	void Unregister(FDelegateHandle InHandle);

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

	// 감시하고 있는 파일의 정보
	struct FWatchedFile
	{
		std::filesystem::path Path;
		std::filesystem::file_time_type AppliedWriteTime;
		bool bIsTimeValid = false;
		bool bIsWaiting = false;
		std::filesystem::file_time_type PendingWriteTime;
		bool bIsReadFailing = false;
		FOnScriptFileChanged Subscribers;
	};

	//   - 키: 정규화된 경로 (소문자)
	//   - 값: 마지막 수정 시간, 구독자 목록(핸들 + 콜백), 디바운스 대기 플래그/시간
	TMap<std::wstring, FWatchedFile> FileMap;
	//   - 핸들 -> 경로 역참조 (Unregister에서 사용)
	TMap<uint64, std::wstring> HandleMap;

	// path를 받아 소문자화 된 wstring을 반환
	static std::wstring MakeKey(const std::filesystem::path& InPath);

	// 경로 정규화 -> 키 생성 -> 파일 항목 검색, 없으면 생성
	FWatchedFile* FindOrAddFile(const std::filesystem::path& InPath, std::wstring& OutKey, bool& bOutCreated);

	// 핸들 검증 -> 역참조 기록 -> 로그
	FDelegateHandle CommitRegister(const std::wstring& InKey, FDelegateHandle InHandle, bool bCreatedFile);

	// 검사 주기 누적 시간
	float AccumulatedTime = 0.f;

	// 파일의 last_write_time 변경 판정
	void CheckFiles();

	// 업데이트 된 파일 알림을 보냄
	void BroadcastUpdates(const TArray<std::wstring>& InConfrimedList);
};
