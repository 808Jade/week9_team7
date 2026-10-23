#pragma once
#include "ActorComponent.h"
#include "Scripting/ScriptWatcher.h"

class ULuaScriptComponent : public UActorComponent
{
	DECLARE_CLASS(ULuaScriptComponent, UActorComponent)
	REFLECT_START(ULuaScriptComponent)
		PROPERTY_FLAGS(ScriptPath, CPF_VisibleAnywhere)
	REFLECT_END()

public:
	virtual ~ULuaScriptComponent() override;

	const FString& GetScriptPath() const { return ScriptPath; }
	// 감시 중에 경로가 바뀌면 해제 후 새 경로로 다시 등록한다
	void SetScriptPath(const FString& InPath);
	bool HasScriptPath() const { return !ScriptPath.empty(); }

	virtual void BeginPlay() override;

	// ==== HotReload ====
	// [계약] 성공/실패를 반환한다. 실패하면 기존 스크립트 상태를 그대로 유지해야 한다.
	// [계약] Lua 함수 핸들(Tick, BeginPlay 등)을 멤버로 캐싱한다면,
	//        처음 로드와 재로드 모두 같은 '핸들 갱신 함수'를 거치게 한다. (6단계 ③)
	bool ReloadScript();
	bool IsWatchingScript() const { return ScriptWatchHandle != InvalidScriptWatchHandle; }
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

	FScriptWatchHandle ScriptWatchHandle = InvalidScriptWatchHandle;
	// ==== HotReload ====

	FString ScriptPath = "";
};
