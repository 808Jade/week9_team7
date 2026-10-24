#include "EnginePCH.h"
#include "LuaBindings.h"

#include "Core/EngineTimer.h"

namespace LuaBindings
{
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
	}
}
