#pragma once
#include "ActorComponent.h"
#include "Scripting/ScriptWatcher.h"
#include "Lua/LuaManager.h"
#include <sol/sol.hpp>
#include <filesystem>
#include <memory>

class ULuaScriptComponent : public UActorComponent
{
	DECLARE_CLASS(ULuaScriptComponent, UActorComponent)
	REFLECT_START(ULuaScriptComponent)
		PROPERTY_FLAGS(ScriptPath, CPF_VisibleAnywhere)
	REFLECT_END()

public:
	ULuaScriptComponent();
	virtual ~ULuaScriptComponent() override;

	// 스크립트 파일을 새 environment에서 실행한다. 실패하면 기존 environment(이전 버전 스크립트)를 그대로 유지한다.
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

	// env 교체와 로드 상태 변경은 LoadScript만 한다. 외부에는 읽기만 연다
	sol::environment& GetScriptEnv() { return ScriptEnv; }
	bool IsLoaded() const { return bIsLoaded; }

	using Super::Serialize;
	virtual void Serialize(FStructuredArchive::FRecord Record) override;

	// 성공/실패를 반환한다. 실패하면 기존 스크립트 상태를 그대로 유지한다.
	bool ReloadScript();
	bool IsWatchingScript() const { return ScriptWatchHandle.IsValid(); }

protected:
	virtual void OnUnregister() override;

private:
	void StartWatchingScript();
	void StopWatchingScript();
	void OnScriptFileChanged(const std::filesystem::path& InChangedPath);

	// 로그용 스크립트 파일 이름 
	FString GetScriptFileName() const;

	// 코루틴 로그용 이름
	FString GetCoroutineDebugName() const;

	// env에 StartCoroutine / StopAllCoroutines 주입
	void InjectCoroutineAPI(sol::environment& Env);

	FDelegateHandle ScriptWatchHandle;

	// 마지막 LoadScript 실패 사유. Lua 에러면 "경로:줄번호: 메시지" 형태 그대로 담긴다
	FString LastLoadError;

	template<typename... Args>
	bool CallLuaFunction(const char* FunctionName, Args&&... InArgs)
	{
		if (!bIsLoaded) return false;
		if (!ScriptEnv.valid() || !ScriptEnv.lua_state()) return false;
		if (!FLuaManager::GetActive() || !FLuaManager::GetActive()->GetState().lua_state()) return false;

		sol::protected_function Function = ScriptEnv[FunctionName];
		if (!Function.valid()) return true;

		auto Result = Function(std::forward<Args>(InArgs)...);
		if (!Result.valid())
		{
			sol::error Error = Result;
			HTR_LOG(Error, "[Lua] {} Runtime error in {}: {}", GetScriptFileName(), FunctionName, Error.what());
			return false;
		}
		return true;
	}

	void CallLuaBeginPlay();

	FString ScriptPath = "";

	sol::environment ScriptEnv;
	bool bIsLoaded = false;

	// Lua BeginPlay가 에러 없이 끝났는지.
	bool bLuaBegunPlay = false;

	// 주입된 Lua 함수가 죽은 this를 쓰지 않게 하는 생존 토큰.
	std::shared_ptr<bool> CoroutineAliveToken;
};
