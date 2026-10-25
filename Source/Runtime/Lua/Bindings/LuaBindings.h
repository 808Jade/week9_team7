#pragma once

#include <sol/sol.hpp>

namespace LuaBindings
{
	void BindMath(sol::state& Lua);
	void BindActor(sol::state& Lua);
	void BindComponent(sol::state& Lua);
	void BindInput(sol::state& Lua);
	void BindTime(sol::state& Lua);
	void BindCoroutine(sol::state& Lua);

	void TickTimers(float DeltaTime);
	void ClearTimers();
}
