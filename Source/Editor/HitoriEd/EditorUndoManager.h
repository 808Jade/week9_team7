#pragma once

#include "Core/EngineString.h"
#include "Core/Types.h"
#include "Containers/Array.h"
#include "Math/Transform.h"
#include <memory>
#include <vector>
#include <functional>
#include <string>

class AActor;
class USceneComponent;
class UWorld;
class ULevel;

// Base class for an undoable/redoable action
class IEditorUndoAction
{
public:
	virtual ~IEditorUndoAction() = default;
	virtual void Undo() = 0;
	virtual void Redo() = 0;
	virtual FString GetDescription() const = 0;
};

// 1. Transform Action (Gizmo translation, rotation, scale, snap to floor)
struct FActorTransformRecord
{
	AActor* Actor = nullptr;
	USceneComponent* Component = nullptr;
	FTransform BeforeTransform;
	FTransform AfterTransform;
};

class FTransformUndoAction : public IEditorUndoAction
{
public:
	FTransformUndoAction(const TArray<FActorTransformRecord>& InRecords, const FString& InDesc = "Transform");
	virtual ~FTransformUndoAction() override = default;

	virtual void Undo() override;
	virtual void Redo() override;
	virtual FString GetDescription() const override { return Description; }

private:
	TArray<FActorTransformRecord> Records;
	FString Description;
};

// 2. Actor Lifetime Action (Spawn, Duplicate, Delete)
class FActorLifetimeUndoAction : public IEditorUndoAction
{
public:
	// bInIsSpawnAction: true for Spawn/Duplicate (Undo detaches, Redo re-attaches)
	//                   false for Delete (Undo re-attaches, Redo detaches)
	// bInCurrentlyInWorld: whether actors are currently in the level
	FActorLifetimeUndoAction(UWorld* InWorld, ULevel* InLevel, const TArray<AActor*>& InActors, bool bInIsSpawnAction, bool bInCurrentlyInWorld, const FString& InDesc);
	virtual ~FActorLifetimeUndoAction() override;

	virtual void Undo() override;
	virtual void Redo() override;
	virtual FString GetDescription() const override { return Description; }

	bool IsInWorld() const { return bInWorld; }

private:
	void DetachActors();
	void AttachActors();

	UWorld* World = nullptr;
	ULevel* Level = nullptr;
	TArray<AActor*> Actors;
	bool bIsSpawnAction = true;
	bool bInWorld = true;
	FString Description;
};

// Singleton manager for editor undo/redo operations
class FEditorUndoManager
{
public:
	static FEditorUndoManager& Get();

	void PushAction(std::unique_ptr<IEditorUndoAction> Action);
	bool CanUndo() const { return !UndoStack.empty(); }
	bool CanRedo() const { return !RedoStack.empty(); }
	bool Undo();
	bool Redo();
	void Clear();

	size_t GetUndoCount() const { return UndoStack.size(); }
	size_t GetRedoCount() const { return RedoStack.size(); }
	FString GetTopUndoDescription() const;
	FString GetTopRedoDescription() const;

	void SetPostUndoRedoCallback(std::function<void()> InCallback) { PostUndoRedoCallback = InCallback; }

private:
	FEditorUndoManager() = default;
	~FEditorUndoManager() { Clear(); }

	std::vector<std::unique_ptr<IEditorUndoAction>> UndoStack;
	std::vector<std::unique_ptr<IEditorUndoAction>> RedoStack;
	const size_t MaxHistorySize = 100;
	std::function<void()> PostUndoRedoCallback;
};
