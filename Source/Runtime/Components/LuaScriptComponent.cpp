#include "EnginePCH.h"
#include "LuaScriptComponent.h"

bool ULuaScriptComponent::ReloadScript()
{
	HTR_LOG(Info, "Reload requested: {}", ScriptPath);
	return true;
}