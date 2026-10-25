#include "EnginePCH.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Lua/LuaManager.h"
#include "Core/EngineLog.h"

UGameInstance::UGameInstance() = default;

UGameInstance::~UGameInstance()
{
	Shutdown();
}

void UGameInstance::Init()
{
	// 루아 가상 머신(FLuaManager) 생성 및 초기화
	LuaManager = MakeUnique<FLuaManager>();
	LuaManager->Init();
	LuaManager->SetActiveWorld(GetWorld());
	FLuaManager::SetActiveManager(LuaManager.get());
	HTR_LOG(Info, "[GameInstance] Initialized with LuaManager.");
}

void UGameInstance::Shutdown()
{
	if (LuaManager)
	{
		if (FLuaManager::GetActive() == LuaManager.get())
		{
			FLuaManager::SetActiveManager(nullptr);
		}
		LuaManager->Shutdown();
		LuaManager.reset();
		HTR_LOG(Info, "[GameInstance] Shutdown: LuaManager destroyed.");
	}

	if (!WorldContext) return;
	if (WorldContext->GameViewport)
	{
		WorldContext->GameViewport->Reset();
		WorldContext->GameViewport.reset();
	}
	WorldContext->OwningGameInstance = nullptr;
	WorldContext = nullptr;
}

UWorld* UGameInstance::GetWorld() const
{
	return WorldContext ? WorldContext->World() : nullptr;
}

void UGameInstance::SetWorldContext(FWorldContext* Context)
{
	WorldContext = Context;
	if (Context) Context->OwningGameInstance = this;
}

bool UGameInstance::InitializeForPlayInEditor(int32 PIEInstance)
{
	UEngine* Engine = GetEngine();
	if (!Engine) return false;
	FWorldContext* Context = Engine->GetWorldContextFromPIEInstance(PIEInstance);
	if (!Context || !Engine->GetEditorWorld()) return false;
	SetWorldContext(Context);
	if (!Engine->CreatePIEWorldByDuplication(*Context, Engine->GetEditorWorld())) return false;
	if (UWorld* World = GetWorld())
	{
		World->SetGameInstance(this);
	}
	Init();
	return true;
}

bool UGameInstance::StartPlayInEditorGameInstance()
{
	UWorld* World = GetWorld();
	if (!World || !WorldContext) return false;
	WorldContext->GameViewport = MakeShared<FGameViewportClient>();
	WorldContext->GameViewport->Init(*WorldContext, GetEngine());
	World->BeginPlay();
	return true;
}

UEngine* UGameInstance::GetEngine() const
{
	return GEngine;
}
