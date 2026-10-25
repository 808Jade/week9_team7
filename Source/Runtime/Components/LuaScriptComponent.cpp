#include "EnginePCH.h"
#include "LuaScriptComponent.h"
#include "Lua/LuaManager.h"
#include "GameFramework/Actor.h"
#include "Scripting/CoroutineScheduler.h"
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

	// C++가 LoadScript에서 매번 새로 주입하는 키. 옛 값을 옮기지 않는다
	constexpr const char* InjectedKeys[] = { "Object", "obj", "actor", "this", "StartCoroutine", "StopAllCoroutines"};

	// 인자로 들어오는 key가 LoadScript에서 매번 새로 주입하는 키인지 확인한다. 
	bool IsInjectedKey(const sol::object& Key)
	{
		if (Key.get_type() != sol::type::string)
		{
			return false;
		}

		const std::string Name = Key.as<std::string>();
		for (const char* Injected : InjectedKeys)
		{
			if (Name == Injected)
			{
				return true;
			}
		}
		return false;
	}

	int32 MigrateScriptState(const sol::environment& OldEnv, sol::environment& NewEnv, int32& OutSkippedFunctions)
	{
		int32 Migrated = 0;
		OutSkippedFunctions = 0;

		for (const auto& [Key, Value] : OldEnv)
		{
			const sol::type ValueType = Value.get_type();
			// 함수는 새 코드 것을 써야함
			if (ValueType == sol::type::function)
			{
				++OutSkippedFunctions;
				continue;
			}
			// 코루틴은 옛 코드로 돌고 있기 때문에 패스
			if (ValueType == sol::type::thread)
			{
				continue;
			}
			// 주입되는 키도 패스
			if (IsInjectedKey(Key))
			{
				continue;
			}

			NewEnv.raw_set(Key, Value);
			++Migrated;
		}
		return Migrated;
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
	FCoroutineScheduler::Get().StopAllForOwner(this);
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
	LastLoadError.clear();

	// 스크립트 파일이 존재하는지 확인
	std::error_code ErrorCode;
	if (!std::filesystem::exists(InScriptPath, ErrorCode))
	{
		LastLoadError = "Script file not found: " + PathString;
		return false;
	}

	std::string Code;
	if (!ReadFileToString(InScriptPath, Code))
	{
		LastLoadError = "Failed to read script file: " + PathString;
		return false;
	}

	// 스크립트 파일이 존재하면 Lua 상태를 가져옴
	sol::state& LuaState = FLuaManager::Get().GetState();

	// 1) 문법 검사: 컴파일만 하고 아직 실행하지 않는다
	sol::load_result Chunk = LuaState.load_buffer(Code.data(), Code.size(), "@" + PathString);
	if (!Chunk.valid())
	{
		// 청크 이름이 "@경로"라서 메시지가 "경로:줄번호: 내용" 형태로 온다
		sol::error Error = Chunk;
		LastLoadError = FString("Syntax error: ") + Error.what();
		return false;
	}

	// 2) 새 environment를 만들고, 현재 오브젝트를 Lua 환경에 바인딩
	sol::environment NewEnv(LuaState, sol::create, LuaState.globals());

	int32 MigratedCount = 0;
	int32 SkippedFunctionCount = 0;
	const bool bIsReload = bIsLoaded && ScriptEnv.valid();
	if (bIsReload)
	{
		MigratedCount = MigrateScriptState(ScriptEnv, NewEnv, SkippedFunctionCount);
	}

	NewEnv["Object"] = GetOwner();
	NewEnv["obj"] = GetOwner();
	NewEnv["actor"] = GetOwner();
	NewEnv["this"] = GetOwner();
	InjectCoroutineAPI(NewEnv);

	// 3) 새 environment에서 실행. 실패하면 기존 ScriptEnv는 건드리지 않는다
	sol::protected_function ChunkFunc = Chunk;
	sol::set_environment(NewEnv, ChunkFunc);
	sol::protected_function_result Result = ChunkFunc();
	if (!Result.valid())
	{
		sol::error Error = Result;
		LastLoadError = FString("Runtime error: ") + Error.what();
		return false;
	}

	// 4) 성공 시에만 교체
	ScriptEnv = NewEnv;
	bIsLoaded = true;
	if (bIsReload)
	{
		HTR_LOG(Info, "[HotReload] State migrated: {} values, {} functions replaced", MigratedCount, SkippedFunctionCount);
	}

	// 외부에서 다른 경로로 로드한 경우 경로를 갱신
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
		if (!LoadScript(Utf8ToPath(ScriptPath)))
		{
			HTR_LOG(Error, "[Lua] {} load failed: {}", GetScriptFileName(), LastLoadError);
			HTR_LOG(Warning, "[Lua] {} is not running. Fix and save the file to load it (Hot Reload)", GetScriptFileName());
		}
	}
	CallLuaBeginPlay();
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
	if (bIsLoaded && bLuaBegunPlay)
	{
		EndPlay();
	}

	FCoroutineScheduler::Get().StopAllForOwner(this);

	ScriptEnv = sol::environment();
	bIsLoaded = false;
	bLuaBegunPlay = false;
	CoroutineAliveToken.reset();

	Super::OnUnregister();
}

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

	// 성공/실패 로그는 ReloadScript가 남긴다
	ReloadScript();
}

void ULuaScriptComponent::CallLuaBeginPlay()
{
	if (!bIsLoaded)
	{
		return;
	}

	// 에러로 중간에 멈췄으면 '실행 안 함'으로 둔다 -> 고쳐서 저장하면 재로드 때 BeginPlay를 다시 부른다
	bLuaBegunPlay = CallLuaFunction("BeginPlay");
}

FString ULuaScriptComponent::GetScriptFileName() const
{
	if (!HasScriptPath())
	{
		return "(no script)";
	}
	return PathToUtf8(Utf8ToPath(ScriptPath).filename());
}

FString ULuaScriptComponent::GetCoroutineDebugName() const
{
	const FString OwnerName = GetOwner() ? GetOwner()->GetName() : FString("None");
	return GetScriptFileName() + " (" + OwnerName + ")";
}

void ULuaScriptComponent::InjectCoroutineAPI(sol::environment& Env)
{
	if (!CoroutineAliveToken)
	{
		CoroutineAliveToken = std::make_shared<bool>(true);
	}
	const std::weak_ptr<bool> WeakToken = CoroutineAliveToken;

	Env.set_function("StartCoroutine", [this, WeakToken](sol::object FunctionObject) -> FCoroutineHandle
	{
		if (WeakToken.expired())
		{
			HTR_LOG(Warning, "[Coroutine] StartCoroutine called after its script component was destroyed");
			return InvalidCoroutineHandle;
		}
		if (FunctionObject.get_type() != sol::type::function)
		{
			HTR_LOG(Warning, "[Coroutine] {}: StartCoroutine expects a function, got {}",
				GetCoroutineDebugName(), sol::type_name(FunctionObject.lua_state(), FunctionObject.get_type()));
			return InvalidCoroutineHandle;
		}
		return FCoroutineScheduler::Get().Start(this, FunctionObject.as<sol::function>(), GetCoroutineDebugName());
	});

	Env.set_function("StopAllCoroutines", [this, WeakToken]() -> int32
	{
		if (WeakToken.expired())
		{
			return 0;
		}
		return FCoroutineScheduler::Get().StopAllForOwner(this);
	});
}

bool ULuaScriptComponent::ReloadScript()
{
	if (!HasScriptPath())
	{
		LastLoadError = "Script path is empty";
		return false;
	}

	const FString FileName = GetScriptFileName();
	const FString OwnerName = GetOwner() ? GetOwner()->GetName() : FString("None");

	const FCoroutineHandle ReloadBoundary = FCoroutineScheduler::Get().GetNextHandle();

	// 문법 검사 -> 새 environment에서 실행 -> 성공 시 교체
	if (!LoadScript(Utf8ToPath(ScriptPath)))
	{
		// 실패: Lua 에러(줄 번호 포함)를 그대로 출력하고, 지금 무엇이 실행 중인지 알려준다
		HTR_LOG(Error, "[HotReload] {} reload failed ({}): {}", FileName, OwnerName, LastLoadError);
		if (bIsLoaded)
		{
			HTR_LOG(Warning, "[HotReload] {} ({}) is still running the previous version", FileName, OwnerName);
		}
		else
		{
			HTR_LOG(Warning, "[HotReload] {} ({}) is not running: no valid version has been loaded yet", FileName, OwnerName);
		}
		return false;
	}

	// 성공 로그를 Lua 훅(BeginPlay / OnHotReload)보다 먼저 찍어서, 스크립트가 찍는 로그가 그 아래에 오게 한다
	HTR_LOG(Info, "[HotReload] {} reloaded ({})", FileName, OwnerName);

	// 이 컴포넌트가 소유한 코루틴 정리
	const int32 StoppedCount = FCoroutineScheduler::Get().StopAllForOwner(this, ReloadBoundary);
	if (StoppedCount > 0)
	{
		HTR_LOG(Info, "[HotReload] {} ({}): stopped {} old coroutine(s)", FileName, OwnerName, StoppedCount);
	}

	if (!bLuaBegunPlay)
	{
		if (HasBegunPlay())
		{
			// 처음 로드가 실패했거나, 이전 BeginPlay가 에러로 중간에 멈춘 경우
			HTR_LOG(Info, "[HotReload] {} ({}): BeginPlay has not completed yet, calling BeginPlay", FileName, OwnerName);
			CallLuaBeginPlay();
		}
	}
	else
	{
		CallLuaFunction("OnHotReload");
	}

	return true;
}
