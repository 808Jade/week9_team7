#pragma once
#include "ActorComponent.h"
#include <sol/sol.hpp>
#include <filesystem>

class ULuaScriptComponent : public UActorComponent
{
	DECLARE_CLASS(ULuaScriptComponent, UActorComponent)

	REFLECT_START(ClassName)
		PROPERTY(ScriptPath)
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
	void OnHit(AActor* OtherActor);

	// 협업용 델리게이트 래퍼
	void TriggerLuaOverlap(AActor* OtherActor) { OnOverlap(OtherActor); }
	void TriggerLuaHit(AActor* OtherActor) { OnHit(OtherActor); }

	FString GetScriptPath() const { return ScriptPath; }
	void SetScriptPath(const FString& InScriptPath) { ScriptPath = InScriptPath; }
	sol::environment& GetScriptEnv() { return ScriptEnv; }
	void SetScriptEnv(const sol::environment& InScriptEnv) { ScriptEnv = InScriptEnv; }
	std::filesystem::file_time_type GetLastWriteTime() const { return LastWriteTime; }
	void SetLastWriteTime(const std::filesystem::file_time_type& InLastWriteTime) { LastWriteTime = InLastWriteTime; }
	bool IsLoaded() const { return bIsLoaded; }
	void SetLoaded(bool bInIsLoaded) { bIsLoaded = bInIsLoaded; }

	using Super::Serialize;
	virtual void Serialize(FStructuredArchive::FRecord Record) override;

	FString ScriptPath;

private:
	template<typename... Args>
	void CallLuaFunction(const char* FunctionName, Args&&... InArgs)
	{
		if (!bIsLoaded) return;

		sol::protected_function Function = ScriptEnv[FunctionName];
		if (!Function.valid()) return;

		auto Result = Function(std::forward<Args>(InArgs)...);
		if (!Result.valid())
		{
			sol::error Error = Result;
			HTR_LOG(Error, "[LuaScriptComponent] Error occurred while calling {}: {}", FunctionName, Error.what());
		}
	}

	sol::environment ScriptEnv;
	std::filesystem::file_time_type LastWriteTime;
	bool bIsLoaded = false;
};
