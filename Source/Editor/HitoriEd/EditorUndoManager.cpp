#include "EnginePCH.h"
#include "EditorUndoManager.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"

// FTransformUndoAction
FTransformUndoAction::FTransformUndoAction(const TArray<FActorTransformRecord>& InRecords, const FString& InDesc)
	: Records(InRecords), Description(InDesc)
{
}

void FTransformUndoAction::Undo()
{
	for (const FActorTransformRecord& Rec : Records)
	{
		if (Rec.Component && GUObjectArray.Contains(Rec.Component))
		{
			Rec.Component->SetTransform(Rec.BeforeTransform);
		}
		else if (Rec.Actor && GUObjectArray.Contains(Rec.Actor) && Rec.Actor->GetRootComponent())
		{
			Rec.Actor->GetRootComponent()->SetTransform(Rec.BeforeTransform);
		}
	}
}

void FTransformUndoAction::Redo()
{
	for (const FActorTransformRecord& Rec : Records)
	{
		if (Rec.Component && GUObjectArray.Contains(Rec.Component))
		{
			Rec.Component->SetTransform(Rec.AfterTransform);
		}
		else if (Rec.Actor && GUObjectArray.Contains(Rec.Actor) && Rec.Actor->GetRootComponent())
		{
			Rec.Actor->GetRootComponent()->SetTransform(Rec.AfterTransform);
		}
	}
}

// FActorLifetimeUndoAction
FActorLifetimeUndoAction::FActorLifetimeUndoAction(UWorld* InWorld, ULevel* InLevel, const TArray<AActor*>& InActors, bool bInIsSpawnAction, bool bInCurrentlyInWorld, const FString& InDesc)
	: World(InWorld), Level(InLevel), Actors(InActors), bIsSpawnAction(bInIsSpawnAction), bInWorld(bInCurrentlyInWorld), Description(InDesc)
{
}

FActorLifetimeUndoAction::~FActorLifetimeUndoAction()
{
	if (!bInWorld)
	{
		for (AActor* Act : Actors)
		{
			if (Act && GUObjectArray.Contains(Act))
			{
				delete Act;
			}
		}
		Actors.Empty();
	}
}

void FActorLifetimeUndoAction::DetachActors()
{
	if (!Level) return;

	for (AActor* Act : Actors)
	{
		if (!Act || !GUObjectArray.Contains(Act)) continue;
		Act->UnregisterAllComponents();
		Act->RegisterAllActorTickFunctions(false);
		Level->RemoveActor(Act);
	}
	bInWorld = false;
}

void FActorLifetimeUndoAction::AttachActors()
{
	if (!Level) return;

	for (AActor* Act : Actors)
	{
		if (!Act || !GUObjectArray.Contains(Act)) continue;
		Level->AddActor(Act);
		Act->RegisterAllComponents();
		Act->RegisterAllActorTickFunctions(true);
	}
	bInWorld = true;
}

void FActorLifetimeUndoAction::Undo()
{
	if (bIsSpawnAction)
	{
		DetachActors();
	}
	else
	{
		AttachActors();
	}
}

void FActorLifetimeUndoAction::Redo()
{
	if (bIsSpawnAction)
	{
		AttachActors();
	}
	else
	{
		DetachActors();
	}
}

// FEditorUndoManager
FEditorUndoManager& FEditorUndoManager::Get()
{
	static FEditorUndoManager Instance;
	return Instance;
}

void FEditorUndoManager::PushAction(std::unique_ptr<IEditorUndoAction> Action)
{
	if (!Action) return;

	RedoStack.clear();
	UndoStack.push_back(std::move(Action));

	if (UndoStack.size() > MaxHistorySize)
	{
		UndoStack.erase(UndoStack.begin());
	}

	HTR_LOG(Info, "[UndoManager] Recorded action: {}", UndoStack.back()->GetDescription());
}

bool FEditorUndoManager::Undo()
{
	if (UndoStack.empty()) return false;

	std::unique_ptr<IEditorUndoAction> Action = std::move(UndoStack.back());
	UndoStack.pop_back();

	FString Desc = Action->GetDescription();
	Action->Undo();
	RedoStack.push_back(std::move(Action));

	HTR_LOG(Info, "[UndoManager] Undid: {}", Desc);

	if (PostUndoRedoCallback)
	{
		PostUndoRedoCallback();
	}

	return true;
}

bool FEditorUndoManager::Redo()
{
	if (RedoStack.empty()) return false;

	std::unique_ptr<IEditorUndoAction> Action = std::move(RedoStack.back());
	RedoStack.pop_back();

	FString Desc = Action->GetDescription();
	Action->Redo();
	UndoStack.push_back(std::move(Action));

	HTR_LOG(Info, "[UndoManager] Redid: {}", Desc);

	if (PostUndoRedoCallback)
	{
		PostUndoRedoCallback();
	}

	return true;
}

void FEditorUndoManager::Clear()
{
	UndoStack.clear();
	RedoStack.clear();
}

FString FEditorUndoManager::GetTopUndoDescription() const
{
	if (UndoStack.empty()) return "";
	return UndoStack.back()->GetDescription();
}

FString FEditorUndoManager::GetTopRedoDescription() const
{
	if (RedoStack.empty()) return "";
	return RedoStack.back()->GetDescription();
}
