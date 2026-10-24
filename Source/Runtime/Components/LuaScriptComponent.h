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

	// env 교체와 로드 상태 변경은 LoadScript만 한다 ("성공했을 때만 교체" 보장). 외부에는 읽기만 연다
	sol::environment& GetScriptEnv() { return ScriptEnv; }
	bool IsLoaded() const { return bIsLoaded; }

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

	// 로그용 스크립트 파일 이름 (예: Scene_Actor.lua)
	FString GetScriptFileName() const;

	FDelegateHandle ScriptWatchHandle;

	// 마지막 LoadScript 실패 사유. Lua 에러면 "경로:줄번호: 메시지" 형태 그대로 담긴다
	// LoadScript는 실패를 직접 로그로 찍지 않고 여기에 남긴다 -> 호출한 쪽(BeginPlay / 재로드)이 상황에 맞게 출력
	FString LastLoadError;
	// ==== HotReload ====

	// 반환값: Lua 에러가 났을 때만 false.
	// 스크립트에 그 함수가 없으면 할 일이 없는 것이므로 true (예: BeginPlay를 정의하지 않은 스크립트)
	template<typename... Args>
	bool CallLuaFunction(const char* FunctionName, Args&&... InArgs)
	{
		if (!bIsLoaded) return false;

		sol::protected_function Function = ScriptEnv[FunctionName];
		if (!Function.valid()) return true;

		auto Result = Function(std::forward<Args>(InArgs)...);
		if (!Result.valid())
		{
			sol::error Error = Result;
			HTR_LOG(Error, "[LuaScriptComponent] Error occurred while calling {}: {}", FunctionName, Error.what());
			return false;
		}
		return true;
	}

	void CallLuaBeginPlay();

	FString ScriptPath = "";

	sol::environment ScriptEnv;
	bool bIsLoaded = false;
	// Lua BeginPlay가 에러 없이 끝났는지. false면 다음 재로드 성공 시 OnHotReload 대신 BeginPlay를 부른다
	bool bLuaBegunPlay = false;
};
