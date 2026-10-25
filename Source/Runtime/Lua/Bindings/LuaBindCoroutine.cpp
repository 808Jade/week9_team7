#include "EnginePCH.h"
#include "LuaBindings.h"
#include "Core/EngineLog.h"
#include "Scripting/CoroutineScheduler.h"

// ============================================================================
// 코루틴 Lua API
//
//   wait(sec)        -> yield("seconds", sec)   -> ECoroutineWaitType::Seconds
//   wait_frames(n)   -> yield("frames", n)      -> ECoroutineWaitType::Frames
//   wait_until(fn)   -> yield("until", fn)      -> ECoroutineWaitType::Condition
//   값 없는 yield    -> (없음)                   -> ECoroutineWaitType::None (다음 프레임)
// ============================================================================

namespace
{
	constexpr const char* CoroutinePreludeChunkName = "=[CoroutinePrelude]";

	constexpr const char* CoroutinePrelude = R"LUA(
-- 코루틴 밖(Tick, BeginPlay 등)에서 부르면 친절한 에러를 낸다.
-- 레벨 3: ensure_in_coroutine(1) -> wait 계열(2) -> 사용자 스크립트(3)
local function ensure_in_coroutine(func_name)
	local co, is_main = coroutine.running()
	-- LuaJIT(5.1): 메인 스레드면 nil. 5.2 호환 빌드면 (thread, true)
	if co == nil or is_main then
		error(func_name .. "() can only be used inside a coroutine started by StartCoroutine()", 3)
	end
end

-- wait(seconds): 게임 시간 기준으로 seconds초 대기. nil / 0 / 음수면 다음 프레임에 재개
function wait(seconds)
	ensure_in_coroutine("wait")

	if seconds == nil then
		seconds = 0
	end
	if type(seconds) ~= "number" then
		error("wait() expects a number (seconds), got " .. type(seconds), 2)
	end
	-- NaN(seconds ~= seconds)이나 음수는 0으로 취급
	if seconds ~= seconds or seconds < 0 then
		seconds = 0
	end

	return coroutine.yield("seconds", seconds)
end

-- wait_frames(n): n프레임 대기. nil이나 1보다 작으면 1(다음 프레임)
function wait_frames(frames)
	ensure_in_coroutine("wait_frames")

	if frames == nil then
		frames = 1
	end
	if type(frames) ~= "number" then
		error("wait_frames() expects a number (frames), got " .. type(frames), 2)
	end
	frames = math.floor(frames)
	if frames ~= frames or frames < 1 then
		frames = 1
	end

	return coroutine.yield("frames", frames)
end

-- wait_until(predicate): predicate()가 true를 반환할 때까지 매 프레임 검사하며 대기
function wait_until(predicate)
	ensure_in_coroutine("wait_until")

	if type(predicate) ~= "function" then
		error("wait_until() expects a function, got " .. type(predicate), 2)
	end

	return coroutine.yield("until", predicate)
end
)LUA";
}

namespace LuaBindings
{
	void BindCoroutine(sol::state& Lua)
	{
		// 대기 함수(wait / wait_frames / wait_until)를 전역에 정의한다.
		sol::protected_function_result Result = Lua.safe_script(CoroutinePrelude, sol::script_pass_on_error, CoroutinePreludeChunkName);
		if (!Result.valid())
		{
			sol::error Error = Result;
			HTR_LOG(Error, "[Coroutine] Failed to load coroutine prelude: {}", Error.what());
			// return하지 않는다: 대기 함수가 없어도 StopCoroutine은 바인딩해 둔다
		}

		// StopCoroutine(handle): 전역 바인딩.
		Lua.set_function("StopCoroutine", [](sol::object HandleObject) -> bool
		{
			if (HandleObject.get_type() != sol::type::number)
			{
				// nil은 조용히 무시한다
				if (HandleObject.get_type() != sol::type::lua_nil)
				{
					HTR_LOG(Warning, "[Coroutine] StopCoroutine expects a handle (number), got {}", sol::type_name(HandleObject.lua_state(), HandleObject.get_type()));
				}
				return false;
			}

			// Lua number는 double. 1 이상 2^53 이하의 정수만 유효한 핸들이다
			constexpr double MaxExactHandle = 9007199254740992.0; // 2^53
			const double Value = HandleObject.as<double>();
			if (!(Value >= 1.0 && Value <= MaxExactHandle) || Value != static_cast<double>(static_cast<FCoroutineHandle>(Value)))
			{
				return false;
			}

			return FCoroutineScheduler::Get().Stop(static_cast<FCoroutineHandle>(Value));
		});
	}
}
