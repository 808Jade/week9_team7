#pragma once
#include "ActorComponent.h"
#include <sol/sol.hpp>
#include <filesystem>

class ULuaScriptComponent : public UActorComponent
{
	DECLARE_CLASS(ULuaScriptComponent, UActorComponent)

	REFLECT_START(ClassName)
		REFLECT_END()

public:
	ULuaScriptComponent();
	~ULuaScriptComponent() override;

	bool LoadScript(const std::filesystem::path& InScriptPath);
	void ReloadScript();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime) override;
	void EndPlay();
	void OnOverlap(AActor* OtherActor);

	std::filesystem::path GetScriptPath() const { return ScriptPath; }
	void SetScriptPath(const std::filesystem::path& InScriptPath) { ScriptPath = InScriptPath; }
	sol::environment& GetScriptEnv() { return ScriptEnv; }
	void SetScriptEnv(const sol::environment& InScriptEnv) { ScriptEnv = InScriptEnv; }
	std::filesystem::file_time_type GetLastWriteTime() const { return LastWriteTime; }
	void SetLastWriteTime(const std::filesystem::file_time_type& InLastWriteTime) { LastWriteTime = InLastWriteTime; }
	bool IsLoaded() const { return bIsLoaded; }
	void SetLoaded(bool bInIsLoaded) { bIsLoaded = bInIsLoaded; }

private:
	std::filesystem::path ScriptPath;
	sol::environment ScriptEnv;
	std::filesystem::file_time_type LastWriteTime;
	bool bIsLoaded = false;
};
