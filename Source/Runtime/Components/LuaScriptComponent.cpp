#include "EnginePCH.h"
#include "LuaScriptComponent.h"
#include "Lua/LuaManager.h"
#include "GameFramework/Actor.h"

ULuaScriptComponent::ULuaScriptComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

ULuaScriptComponent::~ULuaScriptComponent()
{
	EndPlay();
}

bool ULuaScriptComponent::LoadScript(const std::filesystem::path& InScriptPath)
{
	// 스크립트 파일이 존재하는지 확인
	if (!std::filesystem::exists(InScriptPath))
	{
		// 스크립트 파일이 존재하지 않으면 에러 로그를 출력하고 로드하지 않음
		HTR_LOG(Error, "Script file not found: {}", InScriptPath.string());
		bIsLoaded = false;
		return false;
	}

	// 스크립트 파일이 존재하면 Lua 상태를 가져옴
	sol::state& LuaState = FLuaManager::Get().GetState();

	// Lua 환경을 생성하고, 현재 오브젝트를 Lua 환경에 바인딩
	ScriptEnv = sol::environment(LuaState, sol::create, LuaState.globals());

	// 현재 오브젝트를 Lua 환경에 바인딩
	ScriptEnv["Object"] = GetOwner();
	ScriptPath = InScriptPath;
	
	// 스크립트 파일을 실행하고, 에러가 발생하면 로그를 출력하고 로드하지 않음
	try 
	{
		LuaState.script_file(ScriptPath.string(), ScriptEnv);
	}
	catch (const sol::error& e)
	{
		HTR_LOG(Error, "Error occurred while executing script: {}", e.what());
		return false;
	}

	// 스크립트 로드가 성공하면 스크립트 경로와 마지막 수정 시간을 저장하고, 로드 상태를 true로 설정
	bIsLoaded = true;
	LastWriteTime = std::filesystem::last_write_time(ScriptPath);

	return true;
}

void ULuaScriptComponent::ReloadScript()
{
	if(!ScriptPath.empty())
	{
		LoadScript(ScriptPath);
	}
}

void ULuaScriptComponent::BeginPlay()
{
	Super::BeginPlay();
	// 스크립트가 로드되지 않았고, 스크립트 경로가 비어있지 않으면 스크립트를 로드
	if(!bIsLoaded && !ScriptPath.empty())
	{
		LoadScript(ScriptPath);
	}
	// 루아 스크립트가 로드되지 않았거나 스크립트 경로가 비어있으면 BeginPlay를 호출하지 않음
	if (!bIsLoaded) 	return;

	// BeginPlay 함수 바인딩
	sol::protected_function Function = ScriptEnv["BeginPlay"];
	// BeginPlay 함수가 유효하지 않으면 로그를 출력하고 반환
	if(!Function.valid())
	{
		return;
	}
	// 함수 호출하고 결과를 확인
	auto Result = Function();
	// 호출 결과가 유효하지 않으면 에러 로그를 출력
	if (!Result.valid())
	{
		// 에러 메시지 출력
		sol::error Error = Result;
		HTR_LOG(Error, "Error occurred while calling BeginPlay: {}", Error.what());
	}
}

void ULuaScriptComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);
	
	// 스크립트가 로드되지 않았거나 스크립트 경로가 비어있으면 TickComponent를 호출하지 않음
	if (!bIsLoaded) return;

	// 스크립트 파일이 존재하지 않으면 에러 로그를 출력하고 로드 상태를 false로 설정
	if (!std::filesystem::exists(ScriptPath))
	{
		HTR_LOG(Error, "Script file not found: {}", ScriptPath.string());
		bIsLoaded = false;
		return;
	}

	// 스크립트 파일의 마지막 수정 시간을 확인하고, 이전 수정 시간보다 최신이면 스크립트를 다시 로드
	std::filesystem::file_time_type CurrentWriteTime = std::filesystem::last_write_time(ScriptPath);
	if (CurrentWriteTime > LastWriteTime)
	{
		ReloadScript();
	}

	// Tick 함수 바인딩
	sol::protected_function Function = ScriptEnv["Tick"];
	// Tick 함수가 유효하지 않으면 로그를 출력하고 반환
	if (!Function.valid())
	{
		return;
	}
	// Tick 함수 호출하고 결과를 확인
	auto Result = Function(DeltaTime);
	// 호출 결과가 유효하지 않으면 에러 로그를 출력
	if (!Result.valid())
	{
		sol::error Error = Result;
		HTR_LOG(Error, "Error occurred while calling Tick: {}", Error.what());
	}
}

void ULuaScriptComponent::EndPlay()
{
	if (!bIsLoaded) return;

	// EndPlay 함수 바인딩
	sol::protected_function Function = ScriptEnv["EndPlay"];
	// EndPlay 함수가 유효하지 않으면 로그를 출력하고 반환
	if (!Function.valid())
	{
		return;
	}
	// 함수 호출하고 결과를 확인
	auto Result = Function();
	// 호출 결과가 유효하지 않으면 에러 로그를 출력
	if (!Result.valid())
	{
		sol::error Error = Result;
		HTR_LOG(Error, "Error occurred while calling EndPlay: {}", Error.what());
	}
}

void ULuaScriptComponent::OnOverlap(AActor* OtherActor)
{
	if (!bIsLoaded) return;

	// OnOverlap 함수 바인딩
	sol::protected_function Function = ScriptEnv["OnOverlap"];
	if (!Function.valid())
	{
		return;
	}
	// 함수 호출하고 결과를 확인
	auto Result = Function(OtherActor);
	if (!Result.valid())
	{
		sol::error Error = Result;
		HTR_LOG(Error, "Error occurred while calling OnOverlap: {}", Error.what());
	}
}

void ULuaScriptComponent::Serialize(FStructuredArchive::FRecord Record)
{
	Super::Serialize(Record);
	FString PathStr = ScriptPath.string();
	Record << SA_VALUE("ScriptPath", PathStr);
	if (Record.GetUnderlyingArchive().IsLoading())
	{
		ScriptPath = PathStr;
	}
}