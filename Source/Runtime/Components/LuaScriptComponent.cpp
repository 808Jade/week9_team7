#include "EnginePCH.h"
#include "LuaScriptComponent.h"

bool ULuaScriptComponent::ReloadScript()
{
	HTR_LOG(Info, "Reload requested: {}", ScriptPath.c_str());
	return true;
}