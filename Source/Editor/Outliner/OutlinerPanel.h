#pragma once

#include "Editor/EditorUI/EditorPanel.h"
#include "Containers/Array.h"
#include <functional>

class UWorld;
class ULevel;
class AActor;
class UActorComponent;
class UPrimitiveComponent;
class USceneComponent;

using SelectionCallback = std::function<void(USceneComponent*)>;
using DeleteActorCallback = std::function<void(AActor*)>;

class FOutlinerPanel : public IEditorPanel
{
public:
    FOutlinerPanel() = default;
    ~FOutlinerPanel() = default;

    void SetSelectionCallback(SelectionCallback InCallback) { Callback = InCallback; }
    void SetDeleteActorCallback(DeleteActorCallback InCallback) { DeleteCallback = InCallback; }

public:
    bool Init() override;
    void Tick(float DeltaTime) override;
    void OnRender() override;
    const char* GetPanelName() const override { return "Outliner"; }

    void SetWorld(UWorld* InWorld) { World = InWorld; };

    void DrawActors(ULevel* Level);
    void DrawActorNode(AActor* Actor);
    void SelectActor(AActor* Actor);
    void SelectRange(AActor* FromActor, AActor* ToActor);

    AActor* GetSelectedActor() const;
    const TArray<AActor*>& GetSelectedActors() const { return SelectedActors; }
    void ClearSelection();
    void AddToSelection(AActor* Actor);
    void RemoveFromSelection(AActor* Actor);
    bool IsActorSelected(AActor* Actor) const;

    void StartRenaming(AActor* Actor) { RenamingActor = Actor; }

private:
    UWorld* World = nullptr;
    UObject* SelectedObject = nullptr;
    TArray<AActor*> SelectedActors;
    AActor* RenamingActor = nullptr;
    AActor* LastClickedActor = nullptr;

    SelectionCallback Callback;
    DeleteActorCallback DeleteCallback;
    TArray<AActor*> PendingDeleteActors;
};
