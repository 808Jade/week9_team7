#include "EnginePCH.h"
#include "LuaScriptComponent.h"
#include "GameFramework/Actor.h"

namespace
{
	// FString(UTF-8) -> path. std::string을 path에 그대로 넣으면 ANSI(CP949)로 해석돼 한글이 깨진다.
	// (Editor의 FLuaScriptUtils::FStringToFile과 같은 역할. Runtime은 Editor를 include할 수 없어서 따로 둔다)
	std::filesystem::path Utf8ToPath(const FString& InString)
	{
		return std::filesystem::path(std::u8string(InString.begin(), InString.end()));
	}
}

ULuaScriptComponent::~ULuaScriptComponent()
{
	// 정상 흐름이면 OnUnregister에서 이미 해제됨. 콜백이 죽은 this를 잡고 남지 않게 한 번 더 확인
	StopWatchingScript();
}

void ULuaScriptComponent::SetScriptPath(const FString& InPath)
{
	if (ScriptPath == InPath)
	{
		return;
	}

	const bool bWasWatching = IsWatchingScript();
	if (bWasWatching)
	{
		StopWatchingScript();
	}

	ScriptPath = InPath;

	if (bWasWatching)
	{
		StartWatchingScript();
	}
}

void ULuaScriptComponent::BeginPlay()
{
	Super::BeginPlay();

	// TODO(팀원 1): environment 생성, obj 주입, script_file, Lua BeginPlay 호출

	StartWatchingScript();
}

void ULuaScriptComponent::OnUnregister()
{
	StopWatchingScript();
	Super::OnUnregister();
}

// ==== HotReload ====

void ULuaScriptComponent::StartWatchingScript()
{
	if (IsWatchingScript() || !HasScriptPath())
	{
		return;
	}

	// 콜백이 this를 캡처하므로, 이 컴포넌트가 사라지기 전에 반드시 StopWatchingScript가 불려야 한다
	ScriptWatchHandle = FScriptWatcher::Get().Register(
		Utf8ToPath(ScriptPath),
		[this](const std::filesystem::path& InChangedPath)
		{
			OnScriptFileChanged(InChangedPath);
		});
}

void ULuaScriptComponent::StopWatchingScript()
{
	if (!IsWatchingScript())
	{
		return;
	}

	FScriptWatcher::Get().Unregister(ScriptWatchHandle);
	ScriptWatchHandle = InvalidScriptWatchHandle;
}

void ULuaScriptComponent::OnScriptFileChanged(const std::filesystem::path& InChangedPath)
{
	(void)InChangedPath;

	const FString OwnerName = GetOwner() ? GetOwner()->GetName() : FString("None");
	if (ReloadScript())
	{
		HTR_LOG(Info, "[HotReload] Reloaded: {} ({})", ScriptPath, OwnerName);
	}
	else
	{
		HTR_LOG(Error, "[HotReload] Reload failed, keeping previous version: {} ({})", ScriptPath, OwnerName);
	}
}

bool ULuaScriptComponent::ReloadScript()
{
	// TODO(6단계): load_file로 문법 검사 -> 새 environment에서 실행 -> 성공 시 교체
	//              -> Lua 함수 핸들 재캐싱 -> (선택) OnHotReload 호출 -> 소유 코루틴 정리
	HTR_LOG(Info, "Reload requested: {}", ScriptPath);
	return true;
}

// ==== HotReload ====
