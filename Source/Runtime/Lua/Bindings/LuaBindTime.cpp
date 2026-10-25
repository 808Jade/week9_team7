#include "EnginePCH.h"
#include "LuaBindings.h"

#include "Core/EngineTimer.h"
#include "Core/EngineLog.h"
#include "Containers/Array.h"
#include <algorithm>

namespace LuaBindings
{
	struct FLuaTimer
	{
		uint32 Id = 0;
		float RemainingTime = 0.0f;
		sol::function Callback;
		bool bCancelled = false;
	};

	static TArray<FLuaTimer> ActiveTimers;
	static uint32 NextTimerId = 1;

	void TickTimers(float DeltaTime)
	{
		if (ActiveTimers.IsEmpty())
			return;

		for (int32 i = 0; i < ActiveTimers.Num(); ++i)
		{
			if (ActiveTimers[i].bCancelled)
				continue;

			ActiveTimers[i].RemainingTime -= DeltaTime;
			if (ActiveTimers[i].RemainingTime <= 0.0f)
			{
				if (ActiveTimers[i].Callback.valid())
				{
					try
					{
						ActiveTimers[i].Callback();
					}
					catch (const sol::error& e)
					{
						HTR_LOG(Error, "[Lua Timer Error] {}", e.what());
					}
				}
				ActiveTimers[i].bCancelled = true;
			}
		}

		for (int32 i = ActiveTimers.Num() - 1; i >= 0; --i)
		{
			if (ActiveTimers[i].bCancelled)
			{
				ActiveTimers.RemoveAt(i);
			}
		}
	}

	void ClearTimers()
	{
		ActiveTimers.Empty();
	}

	void BindTime(sol::state& Lua)
	{
		auto TimeTable = Lua.create_named_table("Time");

		TimeTable["GetTotalTime"] = []() -> float {
			return EngineTimer::GetTotalTime();
		};

		// 자주 사용하는 단축 별칭
		TimeTable["GetTime"] = []() -> float {
			return EngineTimer::GetTotalTime();
		};

		TimeTable["GetDeltaTime"] = []() -> float {
			return EngineTimer::GetDeltaTime();
		};

		// 실시간 FPS 조회
		TimeTable["GetFPS"] = []() -> float {
			float dt = EngineTimer::GetDeltaTime();
			return (dt > 1e-4f) ? (1.0f / dt) : 60.0f;
		};

		// ==========================================
		// [Timer 모듈]
		// ==========================================
		auto TimerTable = Lua.create_named_table("Timer");

		// Timer.After(delaySeconds, callback) -> handle
		TimerTable["After"] = [](float DelaySeconds, sol::function Callback) -> uint32 {
			if (!Callback.valid())
				return 0;

			uint32 Id = NextTimerId++;
			ActiveTimers.Add({ Id, std::max(0.0f, DelaySeconds), Callback, false });
			return Id;
		};

		// Timer.Clear(handle)
		TimerTable["Clear"] = [](uint32 Handle) {
			for (auto& Timer : ActiveTimers)
			{
				if (Timer.Id == Handle)
				{
					Timer.bCancelled = true;
					break;
				}
			}
		};

		// Timer.ClearAll()
		TimerTable["ClearAll"] = []() {
			ClearTimers();
		};
	}
}
