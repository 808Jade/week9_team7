#pragma once

#include <cstdint>
#include <sol/sol.hpp>

#include "Core/Types.h"
#include "Containers/Containers.h"

class ULuaScriptComponent;

// ============================================================================
// FCoroutineScheduler : Lua 코루틴 스케줄러
//
// - 코루틴 목록을 보관하고, 매 프레임 대기 조건(시간/프레임/조건 함수)을 갱신해 조건이 충족되면 resume한다.
// - resume 결과(yield / 정상 종료 / 에러)를 해석해 다음 대기 상태를 정하거나 목록에서 뺀다.
// - 스크립트 파일 경로, Hot Reload, 액터 이동/게임 규칙은 모른다.
// - 의존 방향: ULuaScriptComponent -> FCoroutineScheduler
// - 단일 스레드. 월드 Tick 안에서 "게임 dt"로 Update를 부른다.
// ============================================================================

// 코루틴 식별 핸들. 0은 무효
using FCoroutineHandle = uint64;
constexpr FCoroutineHandle InvalidCoroutineHandle = 0;

// 코루틴이 다음 resume까지 무엇을 기다리는지
enum class ECoroutineWaitType : uint8
{
	None,      // 다음 Update에서 바로 resume
	Seconds,   // RemainingTime이 0 이하가 되면 resume (게임 시간)
	Frames,    // RemainingFrames가 0이 되면 resume
	Condition, // Condition()이 true를 반환하면 resume
};

// 코루틴 하나 = 엔트리 하나
struct FCoroutineEntry
{
	FCoroutineHandle Handle = InvalidCoroutineHandle;

	// 코루틴 전용 Lua 스택. C++이 들고 있어야 GC가 회수하지 않는다
	sol::thread Thread;
	// 실제 resume 대상. 반드시 Thread의 state 위에서 만든다 (5단계)
	sol::coroutine Coroutine;

	// 이 코루틴을 시작한 컴포넌트. 정리(StopAllForOwner) 식별용으로만 사용한다.
	// nullptr 허용(전역 코루틴) -> Clear()로만 정리된다
	// Owner는 사라지기 전에 반드시 StopAllForOwner(this)를 불러야 한다
	ULuaScriptComponent* Owner = nullptr;

	// ---- 대기 상태 ----
	// 대기 상태를 바꿀 때는 ResetWait()로 이전 값을 먼저 지운다
	ECoroutineWaitType WaitType = ECoroutineWaitType::None;
	float RemainingTime = 0.f;              // Seconds 전용
	int32 RemainingFrames = 0;              // Frames 전용
	sol::protected_function Condition;      // Condition 전용 (에러가 나도 엔진이 죽지 않게 protected)

	// 삭제 예정 표시. Update 순회 중에는 즉시 지우지 않고 이 값만 세운다
	bool bFinished = false;

	// 에러 로그용 (예: "Scene_Enemy.lua (Enemy)")
	FString DebugName;

	void ResetWait()
	{
		WaitType = ECoroutineWaitType::None;
		RemainingTime = 0.f;
		RemainingFrames = 0;
		// Lua 함수 참조를 놓아야 클로저가 붙잡은 값들이 GC될 수 있다
		Condition = sol::protected_function();
	}
};

class FCoroutineScheduler
{
public:
	static FCoroutineScheduler& Get();

	// 코루틴 시작.
	// Function이 유효하지 않으면 InvalidCoroutineHandle을 반환한다.
	// 새 엔트리는 항상 PendingList로 들어가고, 다음 Update부터 처리된다.
	// 인자가 필요하면 Lua 쪽에서 클로저로 감싸서 넘긴다: StartCoroutine(function() f(a, b) end)
	FCoroutineHandle Start(ULuaScriptComponent* Owner, const sol::function& Function, const FString& DebugName = "");

	// 매 프레임 한 번 호출. 게임 dt를 받는다 (PIE가 아닐 때는 부르지 않는다)
	void Update(float DeltaTime);

	// 핸들 하나 정지.
	// 무효 핸들이나 이미 끝난 핸들이 와도 안전하다.
	// Update 순회 중이면 표시만 하고, 실제 제거는 순회가 끝난 뒤에 한다.
	// 반환값: 정지 표시된 코루틴이 있었으면 true
	bool Stop(FCoroutineHandle Handle);

	// Owner가 시작한 코루틴 전부 정지 (Active + Pending 양쪽)
	// Owner == nullptr이면 아무것도 하지 않는다 (실수로 전역 코루틴을 날리지 않기 위해)
	// 반환값: 정지 표시된 코루틴 개수
	int32 StopAllForOwner(const ULuaScriptComponent* Owner);

	// 전부 즉시 비운다. PIE 종료 / 엔진 종료 때만 사용.
	// 반드시 FLuaManager::Shutdown()보다 먼저 불러야 한다
	//        (Lua state가 사라진 뒤 sol::thread / sol::coroutine 소멸자가 돌면 크래시)
	void Clear();

	// 디버그용: 실행 중(+대기 중) 코루틴 개수. 끝남 표시된 것은 제외
	int32 GetActiveCount() const;

	bool IsUpdating() const { return bIsUpdating; }

private:
	FCoroutineScheduler() = default;
	~FCoroutineScheduler() = default;
	FCoroutineScheduler(const FCoroutineScheduler&) = delete;
	FCoroutineScheduler& operator=(const FCoroutineScheduler&) = delete;

	// PendingList -> ActiveList 이동
	void MergePending();

	// bFinished인 엔트리를 Active / Pending 양쪽에서 실제로 제거
	// Update 순회 중에는 부르지 않는다
	void RemoveFinished();

	// Update 순회 중이 아니면 바로 정리, 순회 중이면 Update 끝에서 정리
	void RemoveFinishedIfSafe();

	// 지금 돌고 있는 코루틴
	TArray<FCoroutineEntry> ActiveList;
	// Update 도중(또는 프레임 사이)에 새로 시작된 코루틴. 다음 Update 시작 시 ActiveList로 합친다
	// -> 순회 중인 ActiveList에 push하면 재할당으로 참조/반복자가 깨지기 때문
	TArray<FCoroutineEntry> PendingList;

	// 다음에 발급할 핸들 (0은 무효이므로 1부터)
	FCoroutineHandle NextHandle = 1;

	// Update 순회 중인지. Start/Stop/Clear가 이 값을 보고 동작을 바꾼다
	bool bIsUpdating = false;
};
