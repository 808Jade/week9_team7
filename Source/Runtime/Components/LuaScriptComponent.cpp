#include "EnginePCH.h"
#include "LuaScriptComponent.h"
#include "Lua/LuaManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include <fstream>
#include <sstream>

namespace
{
	// FString(UTF-8) -> path. std::string을 path에 그대로 넣으면 ANSI로 해석돼 한글이 깨진다.
	std::filesystem::path Utf8ToPath(const FString& InString)
	{
		return std::filesystem::path(std::u8string(InString.begin(), InString.end()));
	}

	// path -> FString(UTF-8). path::string()은 ANSI 변환이라 한글 경로에서 예외가 날 수 있다.
	FString PathToUtf8(const std::filesystem::path& InPath)
	{
		const std::u8string U8 = InPath.u8string();
		return FString(U8.begin(), U8.end());
	}

	// luaL_loadfile(fopen)은 ANSI 경로만 받으므로 파일은 직접 읽어서 load_buffer로 넘긴다
	bool ReadFileToString(const std::filesystem::path& InPath, std::string& OutCode)
	{
		std::ifstream File(InPath, std::ios::in | std::ios::binary);
		if (!File.is_open())
		{
			return false;
		}

		std::ostringstream Stream;
		Stream << File.rdbuf();
		OutCode = Stream.str();
		return true;
	}
}

ULuaScriptComponent::ULuaScriptComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
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

bool ULuaScriptComponent::LoadScript(const std::filesystem::path& InScriptPath)
{
	const FString PathString = PathToUtf8(InScriptPath);

	// 스크립트 파일이 존재하는지 확인
	std::error_code ErrorCode;
	if (!std::filesystem::exists(InScriptPath, ErrorCode))
	{
		HTR_LOG(Error, "Script file not found: {}", PathString);
		return false;
	}

	std::string Code;
	if (!ReadFileToString(InScriptPath, Code))
	{
		HTR_LOG(Error, "Failed to read script file: {}", PathString);
		return false;
	}

	// 스크립트 파일이 존재하면 Lua 상태를 가져옴
	// GameInstance가 소유한 LuaManager 우선 조회
	FLuaManager* LuaMgr = nullptr;
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			LuaMgr = GI->GetLuaManager();
		}
	}
	if (!LuaMgr)
	{
		LuaMgr = &FLuaManager::Get();
	}

	sol::state& LuaState = LuaMgr->GetState();

	// 1) 문법 검사: 컴파일만 하고 아직 실행하지 않는다
	sol::load_result Chunk = LuaState.load_buffer(Code.data(), Code.size(), "@" + PathString);
	if (!Chunk.valid())
	{
		sol::error Error = Chunk;
		HTR_LOG(Error, "Syntax error in script: {}", Error.what());
		return false;
	}

	// 2) 새 environment를 만들고, 현재 오브젝트를 Lua 환경에 바인딩
	sol::environment NewEnv(LuaState, sol::create, LuaState.globals());
	NewEnv["Object"] = GetOwner();
	NewEnv["obj"] = GetOwner();
	NewEnv["actor"] = GetOwner();
	NewEnv["this"] = GetOwner();

	// 3) 새 environment에서 실행. 실패하면 기존 ScriptEnv는 건드리지 않는다
	sol::protected_function ChunkFunc = Chunk;
	sol::set_environment(NewEnv, ChunkFunc);
	sol::protected_function_result Result = ChunkFunc();
	if (!Result.valid())
	{
		sol::error Error = Result;
		HTR_LOG(Error, "Error occurred while executing script: {}", Error.what());
		return false;
	}

	// 4) 성공 시에만 교체
	ScriptEnv = NewEnv;
	bIsLoaded = true;
	LastWriteTime = std::filesystem::last_write_time(InScriptPath, ErrorCode);

	// 외부에서 다른 경로로 로드한 경우 경로를 갱신 (감시 대상도 같이 바뀜)
	if (PathString != ScriptPath)
	{
		SetScriptPath(PathString);
	}

	return true;
}

void ULuaScriptComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!bIsLoaded && HasScriptPath())
	{
		LoadScript(Utf8ToPath(ScriptPath));
	}
	CallLuaFunction("BeginPlay");

	StartWatchingScript();
}

void ULuaScriptComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);
	if (!bIsLoaded) return;

	// 파일 변경 감지는 FScriptWatcher가 담당한다 (OnScriptFileChanged -> ReloadScript)
	CallLuaFunction("Tick", DeltaTime);
}

void ULuaScriptComponent::EndPlay()
{
	CallLuaFunction("EndPlay");
}

void ULuaScriptComponent::OnOverlap(AActor* OtherActor)
{
	CallLuaFunction("OnOverlap", OtherActor);
}

void ULuaScriptComponent::OnHit(AActor* OtherActor)
{
	CallLuaFunction("OnHit", OtherActor);
}

void ULuaScriptComponent::Serialize(FStructuredArchive::FRecord Record)
{
	Super::Serialize(Record);
	Record << SA_VALUE("ScriptPath", ScriptPath);
}

void ULuaScriptComponent::OnUnregister()
{
	StopWatchingScript();

	// Destroy / DestroyComponent / PIE 종료 모두 여기를 거친다.
	// 소멸자에서 Lua를 호출하면 Owner나 Lua VM이 이미 정리됐을 수 있으므로 여기서 EndPlay를 부른다.
	if (bIsLoaded && HasBegunPlay())
	{
		EndPlay();
	}
	ScriptEnv = sol::environment();
	bIsLoaded = false;

	Super::OnUnregister();
}

// ==== HotReload ====

void ULuaScriptComponent::StartWatchingScript()
{
	if (IsWatchingScript() || !HasScriptPath())
	{
		return;
	}

	// 멤버 함수 바인딩(AddDynamic). 델리게이트가 this를 들고 있으므로
	// 이 컴포넌트가 사라지기 전에 반드시 StopWatchingScript가 불려야 한다
	ScriptWatchHandle = FScriptWatcher::Get().Register(
		Utf8ToPath(ScriptPath),
		this,
		&ULuaScriptComponent::OnScriptFileChanged);
}

void ULuaScriptComponent::StopWatchingScript()
{
	if (!IsWatchingScript())
	{
		return;
	}

	FScriptWatcher::Get().Unregister(ScriptWatchHandle);
	ScriptWatchHandle.Reset();
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
	if (!HasScriptPath())
	{
		return false;
	}

	// 문법 검사 -> 새 environment에서 실행 -> 성공 시 교체 (LoadScript 내부)
	if (!LoadScript(Utf8ToPath(ScriptPath)))
	{
		return false;
	}

	// (선택) 스크립트가 재로드 직후 처리할 일이 있으면 정의해서 사용
	CallLuaFunction("OnHotReload");

	// TODO(코루틴): 이 컴포넌트가 소유한 코루틴 정리 (FCoroutineScheduler 연동 시)
	return true;
}

// ==== HotReload ====
