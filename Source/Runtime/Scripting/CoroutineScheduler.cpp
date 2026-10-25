#include "EnginePCH.h"
#include "Scripting/CoroutineScheduler.h"
#include "Lua/LuaManager.h"

#include <cmath>
#include <limits>

namespace
{
	constexpr const char* YieldTagSeconds = "seconds";
	constexpr const char* YieldTagFrames = "frames";
	constexpr const char* YieldTagUntil = "until";

	// Value를 초로 변환
	float ToWaitSeconds(const sol::object& Value)
	{
		if (Value.get_type() != sol::type::number)
		{
			return 0.f;
		}
		const double Seconds = Value.as<double>();
		if (std::isnan(Seconds) || Seconds < 0.0)
		{
			return 0.f;
		}
		return static_cast<float>(Seconds);
	}

	// Value를 프레임으로 변환
	int32 ToWaitFrames(const sol::object& Value)
	{
		if (Value.get_type() != sol::type::number)
		{
			return 1;
		}
		const double Frames = Value.as<double>();
		if (!(Frames >= 1.0))
		{
			return 1;
		}
		constexpr double MaxFrames = static_cast<double>(std::numeric_limits<int32>::max());
		if (Frames >= MaxFrames)
		{
			return (std::numeric_limits<int32>::max());
		}
		return static_cast<int32>(Frames);
	}
}

FCoroutineScheduler& FCoroutineScheduler::Get()
{
	static FCoroutineScheduler Instance;
	return Instance;
}

// 코루틴 시작.
FCoroutineHandle FCoroutineScheduler::Start(ULuaScriptComponent* Owner, const sol::function& Function, const FString& DebugName)
{
	// Function이 유효하지 않으면 InvalidCoroutineHandle을 반환한다.
	if (!Function.valid())
	{
		HTR_LOG(Warning, "[Coroutine] Start: invalid function ({})", DebugName);
		return InvalidCoroutineHandle;
	}

	// sol::thread 생성 -> thread state 위에서 sol::coroutine 생성 -> 엔트리를 PendingList에 추가
	sol::state& Lua = FLuaManager::Get().GetState();

	// 코루틴 전용 스택
	sol::thread Thread = sol::thread::create(Lua.lua_state());
	if (!Thread.valid())
	{
		HTR_LOG(Error, "[Coroutine] Start: failed to create thread ({})", DebugName);
		return InvalidCoroutineHandle;
	}

	sol::coroutine Coroutine(Thread.thread_state(), Function);
	if (!Coroutine.valid())
	{
		HTR_LOG(Error, "[Coroutine] Start: failed to create coroutine ({})", DebugName);
		return InvalidCoroutineHandle;
	}

	FCoroutineEntry Entry;
	Entry.Handle = NextHandle++;
	Entry.Thread = std::move(Thread);
	Entry.Coroutine = std::move(Coroutine);
	Entry.Owner = Owner;
	Entry.DebugName = DebugName;

	// 새 엔트리는 항상 PendingList로 들어가고, 다음 Update부터 처리된다.
	const FCoroutineHandle Handle = Entry.Handle;
	PendingList.Add(std::move(Entry));

	return Handle;
}

void FCoroutineScheduler::Update(float DeltaTime)
{
	if (bIsUpdating)
	{
		HTR_LOG(Warning, "[Coroutine] Update called recursively, ignored");
		return;
	}

	// 이전 프레임 이후 새로 시작된 코루틴을 이번 프레임 처리 대상에 포함
	MergePending();
	if (ActiveList.IsEmpty())
	{
		return;
	}

	bIsUpdating = true;

	// ActiveList 순회
	const int32 Count = ActiveList.Num();
	for (int32 i = 0; i < Count; ++i)
	{
		FCoroutineEntry& Entry = ActiveList[i];
		if (Entry.bFinished)
		{
			continue;
		}
		if (IsReadyToResume(Entry, DeltaTime))
		{
			ResumeEntry(Entry);
		}
	}

	bIsUpdating = false;

	// 순회가 끝난 뒤에 한 번에 제거
	RemoveFinished();
}

bool FCoroutineScheduler::Stop(FCoroutineHandle Handle)
{
	if (Handle == InvalidCoroutineHandle)
	{
		return false;
	}

	bool bFound = false;
	auto MarkInList = [Handle, &bFound](TArray<FCoroutineEntry>& List)
	{
		for (FCoroutineEntry& Entry : List)
		{
			if (Entry.Handle == Handle && !Entry.bFinished)
			{
				Entry.bFinished = true;
				bFound = true;
				return;
			}
		}
	};

	// 같은 프레임에 시작하자마자 멈추는 경우도 있으므로 Pending까지 찾는다
	MarkInList(ActiveList);
	if (!bFound)
	{
		MarkInList(PendingList);
	}

	if (bFound)
	{
		RemoveFinishedIfSafe();
	}
	return bFound;
}

int32 FCoroutineScheduler::StopAllForOwner(const ULuaScriptComponent* Owner, FCoroutineHandle BeforeHandle)
{
	// Owner == nullptr이면 아무것도 하지 않는다
	if (!Owner)
	{
		return 0;
	}

	int32 Count = 0;
	auto MarkInList = [Owner, BeforeHandle, &Count](TArray<FCoroutineEntry>& List)
	{
		for (FCoroutineEntry& Entry : List)
		{
			if (BeforeHandle != InvalidCoroutineHandle && Entry.Handle >= BeforeHandle)
			{
				continue;
			}
			if (Entry.Owner == Owner && !Entry.bFinished)
			{
				Entry.bFinished = true;
				++Count;
			}
		}
	};

	MarkInList(ActiveList);
	MarkInList(PendingList);

	if (Count > 0)
	{
		RemoveFinishedIfSafe();
	}
	return Count;
}

void FCoroutineScheduler::Clear()
{
	if (bIsUpdating)
	{
		// 순회 중에 비우면 지금 resume 중인 엔트리 참조가 깨진다 -> 표시만 하고 Update 끝에서 제거
		HTR_LOG(Warning, "[Coroutine] Clear called during Update. Marking all as finished instead");
		for (FCoroutineEntry& Entry : ActiveList)  { Entry.bFinished = true; }
		for (FCoroutineEntry& Entry : PendingList) { Entry.bFinished = true; }
		return;
	}

	const int32 Count = GetActiveCount();

	// sol 참조들이 여기서 해제된다 (Lua state가 살아 있을 때 불려야 함)
	ActiveList.Empty();
	PendingList.Empty();

	if (Count > 0)
	{
		HTR_LOG(Info, "[Coroutine] Cleared {} coroutine(s)", Count);
	}
}

int32 FCoroutineScheduler::GetActiveCount() const
{
	int32 Count = 0;
	for (const FCoroutineEntry& Entry : ActiveList)
	{
		if (!Entry.bFinished) ++Count;
	}
	for (const FCoroutineEntry& Entry : PendingList)
	{
		if (!Entry.bFinished) ++Count;
	}
	return Count;
}

void FCoroutineScheduler::MergePending()
{
	if (PendingList.IsEmpty())
	{
		return;
	}

	for (FCoroutineEntry& Entry : PendingList)
	{
		if (!Entry.bFinished)
		{
			ActiveList.Add(std::move(Entry));
		}
	}
	PendingList.Empty();
}

void FCoroutineScheduler::RemoveFinished()
{
	// 앞에서부터 살아 있는 엔트리를 당겨 채우고(순서 유지), 남은 꼬리를 잘라낸다
	auto Compact = [](TArray<FCoroutineEntry>& List)
	{
		int32 Write = 0;
		for (int32 Read = 0; Read < List.Num(); ++Read)
		{
			if (List[Read].bFinished)
			{
				continue;
			}
			if (Write != Read)
			{
				List[Write] = std::move(List[Read]);
			}
			++Write;
		}
		while (List.Num() > Write)
		{
			List.RemoveLast();
		}
	};

	Compact(ActiveList);
	Compact(PendingList);
}

void FCoroutineScheduler::RemoveFinishedIfSafe()
{
	if (!bIsUpdating)
	{
		RemoveFinished();
	}
}

void FCoroutineScheduler::ResumeEntry(FCoroutineEntry& Entry)
{
	if (Entry.bFinished)
	{
		return;
	}
	if (!Entry.Coroutine.valid() || !Entry.Coroutine.runnable())
	{
		Entry.bFinished = true;
		return;
	}

	Entry.ResetWait();

	sol::protected_function_result Result = Entry.Coroutine();

	if (!Result.valid())
	{
		sol::error Error = Result;
		HTR_LOG(Error, "[Coroutine] {} error: {}", Entry.DebugName, Error.what());
		Entry.bFinished = true;
		return;
	}

	if (Entry.bFinished)
	{
		return;
	}

	if (Result.status() == sol::call_status::ok)
	{
		Entry.bFinished = true;
		return;
	}

	ApplyYield(Entry, Result);
}

void FCoroutineScheduler::ApplyYield(FCoroutineEntry& Entry, const sol::protected_function_result& Result)
{
	if (Result.return_count() == 0)
	{
		return;
	}

	const sol::object Tag = Result.get<sol::object>(0);
	if (Tag.get_type() != sol::type::string)
	{
		HTR_LOG(Warning, "[Coroutine] {}: unknown yield value, resuming next frame", Entry.DebugName);
		return;
	}

	const std::string TagName = Tag.as<std::string>();
	const sol::object Value = Result.return_count() > 1 ? Result.get<sol::object>(1) : sol::object();

	if (TagName == YieldTagSeconds)
	{
		Entry.WaitType = ECoroutineWaitType::Seconds;
		Entry.RemainingTime = ToWaitSeconds(Value);
	}
	else if (TagName == YieldTagFrames)
	{
		Entry.WaitType = ECoroutineWaitType::Frames;
		Entry.RemainingFrames = ToWaitFrames(Value);
	}
	else if (TagName == YieldTagUntil)
	{
		if (Value.get_type() != sol::type::function)
		{
			HTR_LOG(Warning, "[Coroutine] {}: wait_until without a function, resuming next frame", Entry.DebugName);
			return;
		}
		lua_State* MainState = FLuaManager::Get().GetState().lua_state();
		Entry.WaitType = ECoroutineWaitType::Condition;
		Entry.Condition = sol::protected_function(MainState, Value);
	}
	else
	{
		HTR_LOG(Warning, "[Coroutine] {}: unknown yield tag '{}', resuming next frame", Entry.DebugName, TagName);
	}

}

bool FCoroutineScheduler::IsReadyToResume(FCoroutineEntry& Entry, float DelatTime)
{
	switch (Entry.WaitType)
	{
	case ECoroutineWaitType::None:
		return true;

	case ECoroutineWaitType::Seconds:
		Entry.RemainingTime -= DelatTime;
		return Entry.RemainingTime <= 0.f;

	case ECoroutineWaitType::Frames:
		--Entry.RemainingFrames;
		return Entry.RemainingFrames <= 0;

	case ECoroutineWaitType::Condition:
	{
		if (!Entry.Condition.valid())
		{
			return true;
		}

		sol::protected_function_result Result = Entry.Condition();
		if (!Result.valid())
		{
			sol::error Error = Result;
			HTR_LOG(Error, "[Coroutine] {} wait_until condition error: {}", Entry.DebugName, Error.what());
			Entry.bFinished = true;
			return false;
		}
		if (Entry.bFinished)
		{
			return false;
		}

		return Result.return_count() > 0 && Result.get<bool>(0);
	}
	}

	return true;
}
