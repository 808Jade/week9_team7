#include "EnginePCH.h"
#include "LuaManager.h"
#include "Bindings/LuaBindings.h"
#include "Core/EngineLog.h"

bool FLuaManager::Init()
{
	if (bInitialized) return true;

	// 루아 VM 초기화 및 표준 라이브러리 로드
	Lua.open_libraries(
		sol::lib::base,
		sol::lib::math,
		sol::lib::string,
		sol::lib::table,
		sol::lib::coroutine);

	Lua.set_exception_handler([](lua_State* L, sol::optional<const std::exception&> MaybeException, sol::string_view Description) {
		HTR_LOG(Error, "[Lua Exception] {}", Description);
		return sol::stack::push(L, Description);
	});
	lua_atpanic(Lua.lua_state(), [](lua_State* L) -> int {
		const char* Msg = lua_tostring(L, -1);
		HTR_LOG(Error, "[Lua Panic] Fatal error: {}", Msg ? Msg : "Unknown");
		return 0; // 에디터 강제 종료(abort) 방지
	});

	// 엔진 API 바인딩
	BindEngine();
	bInitialized = true;
	HTR_LOG(Info, "[LuaManager] Successfully initialized LuaJIT & Sol2!");

	return true;
}

void FLuaManager::Shutdown()
{
	if (!bInitialized) return;
	// 루아 VM 종료
	Lua.collect_garbage();
	bInitialized = false;
	HTR_LOG(Info, "[LuaManager] Shutdown completed.");
}

void FLuaManager::BindEngine()
{
	// 기본 print 함수를 엔진 로그 바인딩 (소문자/대문자 모두 지원)
	auto PrintFunc = [](sol::variadic_args args) {
		FString msg;
		for (auto arg : args) {
			msg += arg.as<FString>() + " ";
		}
		HTR_LOG(Info, "[Lua] {}", msg);
	};
	Lua.set_function("print", PrintFunc);
	Lua.set_function("Print", PrintFunc);

	// 도메인별 바인딩 모듈 호출
	LuaBindings::BindMath(Lua);
	LuaBindings::BindActor(Lua);
	LuaBindings::BindComponent(Lua);
	LuaBindings::BindInput(Lua);
	LuaBindings::BindTime(Lua);
}
