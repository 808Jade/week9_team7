#pragma once 
#include "ActorComponent.h"

class ULuaScriptComponent : public UActorComponent
{
	DECLARE_CLASS(ULuaScriptComponent, UActorComponent)
	REFLECT_START(ULuaScriptComponent)
		PROPERTY_FLAGS(ScriptPath, CPF_VisibleAnywhere)
	REFLECT_END()

public:
	const FString& GetScriptPath() const { return ScriptPath; }
	void SetScriptPath(const FString& InPath) { ScriptPath = InPath; }
	bool HasScriptPath() const { return !ScriptPath.empty(); }
	bool ReloadScript();

private:
	FString ScriptPath = "";
};