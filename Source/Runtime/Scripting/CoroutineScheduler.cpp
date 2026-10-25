#include "EnginePCH.h"
#include "Scripting/CoroutineScheduler.h"
#include "Lua/LuaManager.h"

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

	return InvalidCoroutineHandle;
}

void FCoroutineScheduler::Update(float DeltaTime)
{
	(void)DeltaTime;

	// 이전 프레임 이후 새로 시작된 코루틴을 이번 프레임 처리 대상에 포함
	MergePending();

	bIsUpdating = true;

	// TODO(7단계): ActiveList 순회
	//   - bFinished 건너뛰기
	//   - WaitType별 대기 갱신 (Seconds: dt 차감 / Frames: 1 감소 / Condition: protected 호출)
	//   - 조건 충족 시 resume -> 결과 해석(6단계)
	//   - 순회 중 추가는 PendingList로, 삭제는 bFinished 표시로만

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
