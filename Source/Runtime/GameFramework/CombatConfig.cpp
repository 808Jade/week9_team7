#include "EnginePCH.h"
#include "CombatConfig.h"
#include "Lua/LuaManager.h"
#include "Scripting/ScriptWatcher.h"
#include "Core/EngineLog.h"
#include <filesystem>

FCombatConfig& FCombatConfig::Get()
{
	static FCombatConfig Instance;
	static bool bInitialized = false;
	if (!bInitialized)
	{
		Instance.LoadFromLua();
		bInitialized = true;

		FScriptWatcher::Get().Register("Scripts/CombatConfig.lua", [](const std::filesystem::path&) {
			FCombatConfig::Get().LoadFromLua();
		});
	}
	return Instance;
}

void FCombatConfig::LoadFromLua()
{
	sol::state& Lua = FLuaManager::Get().GetState();
	sol::protected_function_result Result = Lua.safe_script_file("Scripts/CombatConfig.lua", &sol::script_pass_on_error);
	if (!Result.valid())
	{
		Result = Lua.safe_script_file("../../../Scripts/CombatConfig.lua", &sol::script_pass_on_error);
	}

	if (Result.valid())
	{
		sol::table Config = Result;
		if (Config.valid())
		{
			sol::object MeleeObj = Config["Melee"];
			if (MeleeObj.is<sol::table>())
			{
				sol::table Melee = MeleeObj.as<sol::table>();
				MeleeRange = Melee.get_or("Range", MeleeRange);
				MeleeCooldown = Melee.get_or("Cooldown", MeleeCooldown);
			}

			sol::object HitStopObj = Config["HitStop"];
			if (HitStopObj.is<sol::table>())
			{
				sol::table HitStop = HitStopObj.as<sol::table>();
				HitStopDuration = HitStop.get_or("Duration", HitStopDuration);
				HitStopTimeDilation = HitStop.get_or("TimeDilation", HitStopTimeDilation);
			}

			sol::object SlomoObj = Config["Slomo"];
			if (SlomoObj.is<sol::table>())
			{
				sol::table Slomo = SlomoObj.as<sol::table>();
				SlomoDuration = Slomo.get_or("Duration", SlomoDuration);
				SlomoTimeDilation = Slomo.get_or("TimeDilation", SlomoTimeDilation);
			}

			sol::object KnockbackObj = Config["Knockback"];
			if (KnockbackObj.is<sol::table>())
			{
				sol::table Knockback = KnockbackObj.as<sol::table>();
				KnockbackHorizontal = Knockback.get_or("HorizontalImpulse", KnockbackHorizontal);
				KnockbackVertical = Knockback.get_or("VerticalImpulse", KnockbackVertical);
				Gravity = Knockback.get_or("Gravity", Gravity);
				GroundFriction = Knockback.get_or("GroundFriction", GroundFriction);
			}

			sol::object SquashObj = Config["Squash"];
			if (SquashObj.is<sol::table>())
			{
				sol::table Squash = SquashObj.as<sol::table>();
				SquashMinScale = Squash.get_or("MinScale", SquashMinScale);
				SquashMaxScale = Squash.get_or("MaxScale", SquashMaxScale);
				SquashUpScale = Squash.get_or("UpScale", SquashUpScale);
				SquashRecoverySpeed = Squash.get_or("RecoverySpeed", SquashRecoverySpeed);
			}
			HTR_LOG(Info, "[CombatConfig] Loaded successfully from Scripts/CombatConfig.lua");
		}
	}
	else
	{
		sol::error Err = Result;
		HTR_LOG(Warning, "[CombatConfig] Failed to load Scripts/CombatConfig.lua: {}. Using default values.", Err.what());
	}
}
