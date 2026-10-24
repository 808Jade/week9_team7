#pragma once
#include "ActorComponent.h"
#include "Scripting/ScriptWatcher.h"
#include <sol/sol.hpp>
#include <filesystem>

class ULuaScriptComponent : public UActorComponent
{
	DECLARE_CLASS(ULuaScriptComponent, UActorComponent)
	REFLECT_START(ULuaScriptComponent)
		PROPERTY_FLAGS(ScriptPath, CPF_VisibleAnywhere)
	REFLECT_END()

public:
	ULuaScriptComponent();
	virtual ~ULuaScriptComponent() override;

	// 스크립트 파일을 새 environment에서 실행한다.
	// 실패하면 기존 environment(이전 버전 스크립트)를 그대로 유지한다.
	bool LoadScript(const std::filesystem::path& InScriptPath);

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime) override;
	void EndPlay();
	void OnOverlap(AActor* OtherActor);
	void OnHit(AActor* OtherActor);

	// 협업용 델리게이트 래퍼
	void TriggerLuaOverlap(AActor* OtherActor) { OnOverlap(OtherActor); }
	void TriggerLuaHit(AActor* OtherActor) { OnHit(OtherActor); }

	const FString& GetScriptPath() const { return ScriptPath; }
	// 감시 중에 경로가 바뀌면 해제 후 새 경로로 다시 등록한다
	void SetScriptPath(const FString& InPath);
	bool HasScriptPath() const { return !ScriptPath.empty(); }

	sol::environment& GetScriptEnv() { return ScriptEnv; }
	void SetScriptEnv(const sol::environment& InScriptEnv) { ScriptEnv = InScriptEnv; }
	std::filesystem::file_time_type GetLastWriteTime() const { return LastWriteTime; }
	void SetLastWriteTime(const std::filesystem::file_time_type& InLastWriteTime) { LastWriteTime = InLastWriteTime; }
	bool IsLoaded() const { return bIsLoaded; }
	void SetLoaded(bool bInIsLoaded) { bIsLoaded = bInIsLoaded; }

	using Super::Serialize;
	virtual void Serialize(FStructuredArchive::FRecord Record) override;

	// ==== HotReload ====
	// [계약] 성공/실패를 반환한다. 실패하면 기존 스크립트 상태를 그대로 유지한다.
	// [계약] 처음 로드와 재로드 모두 LoadScript를 거친다.
	bool ReloadScript();
	bool IsWatchingScript() const { return ScriptWatchHandle.IsValid(); }
	// ==== HotReload ====

protected:
	virtual void OnUnregister() override;

private:
	// ==== HotReload ====
	// 등록: BeginPlay (PIE에서만 호출됨)
	// 해제: OnUnregister (Destroy / DestroyComponent / PIE 종료 시 모두 거침) + 소멸자(안전망)
	void StartWatchingScript();
	void StopWatchingScript();
	void OnScriptFileChanged(const std::filesystem::path& InChangedPath);

	FDelegateHandle ScriptWatchHandle;
	// ==== HotReload ====

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

	FString ScriptPath = "";

	sol::environment ScriptEnv;
	std::filesystem::file_time_type LastWriteTime;
	bool bIsLoaded = false;
};
