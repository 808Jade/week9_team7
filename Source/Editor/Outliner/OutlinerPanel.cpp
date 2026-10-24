#include "EnginePCH.h"
#include "Editor/Outliner/OutlinerPanel.h"

#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Core/EngineLog.h"

static ImGuiTextFilter OutlinerSearchFilter;

bool FOutlinerPanel::Init()
{
    return true;
}

void FOutlinerPanel::Tick(float DeltaTime)
{
}

void FOutlinerPanel::OnRender()
{
    if (!World)
        return;

    ULevel* Level = World->GetCurrentLevel();
    if (!Level)
        return;

    ImGui::Begin("Outliner");

    // 1. 액터 검색 필터
    OutlinerSearchFilter.Draw("##OutlinerFilter", -1.0f);
    if (!OutlinerSearchFilter.IsActive())
    {
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 200.0f);
        ImGui::TextDisabled("Search Actors...");
    }
    ImGui::Separator();

    // 2. F2 단축키로 이름 변경 진입
    if (!ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_F2))
    {
        if (AActor* Sel = GetSelectedActor())
        {
            StartRenaming(Sel);
        }
    }

    DrawActors(Level);

    ImGui::End();

    // 지연 삭제 처리
    if (!PendingDeleteActors.IsEmpty())
    {
        if (DeleteCallback)
        {
            for (AActor* Act : PendingDeleteActors)
            {
                if (Act)
                {
                    DeleteCallback(Act);
                }
            }
        }
        PendingDeleteActors.Empty();
        ClearSelection();
    }
}

void FOutlinerPanel::DrawActors(ULevel* Level)
{
    if (!Level)
        return;

    const TArray<AActor*>& Actors = Level->GetActors();

    ImGuiListClipper Clipper;
    Clipper.Begin(Actors.Num(), ImGui::GetTextLineHeightWithSpacing());

    while (Clipper.Step())
    {
        for (int32 Index = Clipper.DisplayStart; Index < Clipper.DisplayEnd; ++Index)
        {
            AActor* Actor = Actors[Index];
            if (Actor)
            {
                if (OutlinerSearchFilter.IsActive() && !OutlinerSearchFilter.PassFilter(Actor->GetName().c_str()))
                    continue;

                DrawActorNode(Actor);
            }
        }
    }
}

void FOutlinerPanel::DrawActorNode(AActor* Actor)
{
    if (!Actor)
        return;

    const FString& ActorName = Actor->GetName();

    // 1. 이름 변경 중인 경우: InputText 인라인 표시
    if (Actor == RenamingActor)
    {
        char NameBuffer[128] = {};
        strncpy_s(NameBuffer, sizeof(NameBuffer), ActorName.c_str(), _TRUNCATE);
        ImGui::SetKeyboardFocusHere();
        if (ImGui::InputText("##RenameActorInput", NameBuffer, sizeof(NameBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
        {
            if (strlen(NameBuffer) > 0)
            {
                Actor->SetName(NameBuffer);
            }
            RenamingActor = nullptr;
        }
        if (ImGui::IsItemDeactivated() && !ImGui::IsItemDeactivatedAfterEdit())
        {
            RenamingActor = nullptr;
        }
        return;
    }

    // 2. 기본 TreeNode 표시
    ImGuiTreeNodeFlags Flags = 
        ImGuiTreeNodeFlags_Leaf |
        ImGuiTreeNodeFlags_NoTreePushOnOpen |
        ImGuiTreeNodeFlags_SpanAvailWidth;

    if (IsActorSelected(Actor))
        Flags |= ImGuiTreeNodeFlags_Selected;

    ImGui::TreeNodeEx(Actor, Flags, "%s", ActorName.c_str());

    // 3. 마우스 클릭 선택 (Ctrl + 클릭: 다중 선택 토글, Shift + 클릭: 범위 선택)
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        if (ImGui::GetIO().KeyShift && LastClickedActor && World)
        {
            SelectRange(LastClickedActor, Actor);
        }
        else if (ImGui::GetIO().KeyCtrl)
        {
            if (IsActorSelected(Actor))
                RemoveFromSelection(Actor);
            else
                AddToSelection(Actor);
            LastClickedActor = Actor;
        }
        else
        {
            SelectActor(Actor);
            LastClickedActor = Actor;
        }
    }

    // 4. 우클릭 메뉴 (이름 변경, 삭제 등)
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Rename"))
        {
            RenamingActor = Actor;
        }
        if (ImGui::MenuItem("Delete"))
        {
            if (IsActorSelected(Actor) && SelectedActors.Num() > 1)
            {
                PendingDeleteActors = SelectedActors;
            }
            else
            {
                PendingDeleteActors.Empty();
                PendingDeleteActors.Add(Actor);
            }
        }
        ImGui::EndPopup();
    }
}

void FOutlinerPanel::SelectRange(AActor* FromActor, AActor* ToActor)
{
    if (!World || !FromActor || !ToActor) return;
    ULevel* Level = World->GetPersistentLevel();
    if (!Level) return;

    TArray<AActor*> VisibleActors;
    for (AActor* Act : Level->GetActors())
    {
        if (!Act) continue;
        if (OutlinerSearchFilter.IsActive() && !OutlinerSearchFilter.PassFilter(Act->GetName().c_str()))
            continue;
        VisibleActors.Add(Act);
    }

    int32 FromIndex = VisibleActors.Find(FromActor);
    int32 ToIndex = VisibleActors.Find(ToActor);
    if (FromIndex == -1 || ToIndex == -1)
    {
        SelectActor(ToActor);
        LastClickedActor = ToActor;
        return;
    }

    int32 StartIdx = std::min(FromIndex, ToIndex);
    int32 EndIdx = std::max(FromIndex, ToIndex);

    ClearSelection();
    for (int32 i = StartIdx; i <= EndIdx; ++i)
    {
        if (!SelectedActors.Contains(VisibleActors[i]))
        {
            SelectedActors.Add(VisibleActors[i]);
        }
    }

    SelectedObject = ToActor;
    USceneComponent* Scene = Cast<USceneComponent>(ToActor->GetRootComponent());
    if (Callback)
        Callback(Scene);
}

void FOutlinerPanel::ClearSelection()
{
    SelectedActors.Empty();
    SelectedObject = nullptr;
    if (Callback)
        Callback(nullptr);
}

void FOutlinerPanel::AddToSelection(AActor* Actor)
{
    if (!Actor) return;
    if (!SelectedActors.Contains(Actor))
    {
        SelectedActors.Add(Actor);
    }
    SelectedObject = Actor;
    USceneComponent* Scene = Cast<USceneComponent>(Actor->GetRootComponent());
    if (Callback)
        Callback(Scene);
}

void FOutlinerPanel::RemoveFromSelection(AActor* Actor)
{
    SelectedActors.Remove(Actor);
    if (SelectedObject == Actor)
    {
        SelectedObject = SelectedActors.IsEmpty() ? nullptr : SelectedActors.Last();
        USceneComponent* Scene = SelectedObject ? Cast<USceneComponent>(Cast<AActor>(SelectedObject)->GetRootComponent()) : nullptr;
        if (Callback)
            Callback(Scene);
    }
}

bool FOutlinerPanel::IsActorSelected(AActor* Actor) const
{
    return SelectedActors.Contains(Actor) || SelectedObject == Actor;
}

void FOutlinerPanel::SelectActor(AActor* Actor)
{
    ClearSelection();
    if (Actor)
    {
        AddToSelection(Actor);
        LastClickedActor = Actor;
        HTR_LOG(Info, "{} UUID {} is selected", Actor->GetName(), Actor->GetUUID());
    }
    else
    {
        LastClickedActor = nullptr;
    }
}

AActor* FOutlinerPanel::GetSelectedActor() const
{
    if (!SelectedActors.IsEmpty())
        return SelectedActors.Last();
    return Cast<AActor>(SelectedObject);
}
