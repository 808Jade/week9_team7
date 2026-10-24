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
	ScriptEnv["obj"] = GetOwner();
	ScriptEnv["actor"] = GetOwner();
	ScriptEnv["this"] = GetOwner();
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
	if (!bIsLoaded && !ScriptPath.empty())
	{
		LoadScript(ScriptPath);
	}
	CallLuaFunction("BeginPlay");
}

void ULuaScriptComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);
	if (!bIsLoaded) return;

	if (!std::filesystem::exists(ScriptPath))
	{
		HTR_LOG(Error, "Script file not found: {}", ScriptPath.string());
		bIsLoaded = false;
		return;
	}

	std::filesystem::file_time_type CurrentWriteTime = std::filesystem::last_write_time(ScriptPath);
	if (CurrentWriteTime > LastWriteTime)
	{
		ReloadScript();
	}

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
	FString PathStr = ScriptPath.string();
	Record << SA_VALUE("ScriptPath", PathStr);
	if (Record.GetUnderlyingArchive().IsLoading())
	{
		ScriptPath = PathStr;
	}
}