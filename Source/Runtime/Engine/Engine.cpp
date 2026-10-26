#include "EnginePCH.h"
#include "Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Level.h"
#include "Camera/CameraActor.h"

UEngine* GEngine = nullptr;

bool UEngine::Init()
{
	AssetManager = NewObject<UAssetManager>();
	/*World = FObjectFactory::ConstructObject<UWorld>();

	if (!World || !World->Init()) return false;*/

 	return true;
}

FWorldContext& UEngine::CreateNewWorldContext(EWorldType WorldType)
{
	FWorldContext* NewWorldContext = new FWorldContext();
	WorldContextlist.Add(NewWorldContext);
	NewWorldContext->WorldType = WorldType;
	NewWorldContext->ContextHandle = FName();

	return *NewWorldContext;
}

FWorldContext* UEngine::GetWorldContextFromWorld(const UWorld* InWorld)
{
	for (FWorldContext& WorldContext : WorldContextlist)
	{
		if (WorldContext.World() == InWorld)
		{
			return &WorldContext;
		}
	}
	return nullptr;
}

FWorldContext* UEngine::GetWorldContextFromType(EWorldType WorldType)
{
	for (FWorldContext& Context : WorldContextlist)
		if (Context.WorldType == WorldType) return &Context;
	return nullptr;
}

FWorldContext* UEngine::GetWorldContextFromPIEInstance(int32 PIEInstance)
{
	for (FWorldContext& Context : WorldContextlist)
		if (Context.WorldType == EWorldType::PIE && Context.PIEInstance == PIEInstance) return &Context;
	return nullptr;
}

UWorld* UEngine::GetEditorWorld() const
{
	for (const FWorldContext& Context : WorldContextlist)
		if (Context.WorldType == EWorldType::Editor) return Context.World();
	return nullptr;
}

UWorld* UEngine::CreatePIEWorldByDuplication(FWorldContext& Context, UWorld* SourceWorld)
{
	UWorld* World = UWorld::GetDuplicatedWorldForPIE(SourceWorld);
	if (World) Context.SetCurrentWorld(World);
	return World;
}

void UEngine::DestroyWorldContext(UWorld* InWorld)
{
	for (int32 idx = 0; idx < WorldContextlist.Num(); ++idx)
	{
		if (WorldContextlist[idx].World() == InWorld)
		{
			const EWorldType ContextType = WorldContextlist[idx].WorldType;
			if (WorldContextlist[idx].GameViewport)
			{
				WorldContextlist[idx].GameViewport->LostFocus();
				WorldContextlist[idx].GameViewport->Reset();
			}
			if (InWorld && ContextType == EWorldType::PIE)
			{
				ACameraActor* Camera = InWorld->GetMainCamera();
				const bool bOwnsCamera = Camera && !Camera->GetLevel();
				// ClearWorld가 MainCamera의 컴포넌트를 Unregister하므로, 카메라 해제는 그 다음에 해야 한다.
				// 순서가 반대면 MainCamera가 이미 nullptr라 카메라 컴포넌트가 등록된 채로 delete되어
				// ~UActorComponent의 "Delete Without Unregister" assert가 터진다.
				InWorld->ClearWorld();
				InWorld->SetMainCamera(nullptr);
				for (ULevel* Level : InWorld->GetLevels()) delete Level;
				if (bOwnsCamera) delete Camera;
				delete InWorld;
			}
			if (WorldContextlist[idx].OwningGameInstance)
			{
				UGameInstance* Instance = WorldContextlist[idx].OwningGameInstance;
				Instance->Shutdown();
				delete Instance;
			}
			WorldContextlist[idx].SetCurrentWorld(nullptr);
			WorldContextlist.RemoveAt(idx);
			break;
		}
	}
}

void FWorldContext::SetCurrentWorld(UWorld* World)
{
	ThisCurrentWorld = World;
}
