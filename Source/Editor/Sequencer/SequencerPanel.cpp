#include "EnginePCH.h"
#include "SequencerPanel.h"
#include "Asset/AssetManager.h"
#include "Render/Texture2D.h"
#include "Editor/Outliner/OutlinerPanel.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include <cmath>
#include <algorithm>

FSequencerPanel::FSequencerPanel()
{
    Player = MakeShared<FSequencePlayer>();
}

bool FSequencerPanel::Init()
{
    // 툴바 재생/일시정지/정지 아이콘 로드
    PlayIcon = UAssetManager::GetAssetByPath<UTexture2D>("Assets/Icons/Toolbar/Play.png");
    PauseIcon = UAssetManager::GetAssetByPath<UTexture2D>("Assets/Icons/Toolbar/Pause.png");
    StopIcon = UAssetManager::GetAssetByPath<UTexture2D>("Assets/Icons/Toolbar/Stop.png");

    return true;
}

void FSequencerPanel::Tick(float DeltaTime)
{
    const bool bInPIE = IsPlayingQuery ? IsPlayingQuery() : false;
    if (bInPIE)
    {
        // 게임 플레이(PIE) 중에는 에디터 시퀀서 재생 자동 일시정지 및 틱 중단
        if (Player && Player->IsPlaying())
        {
            Player->Pause();
        }
    }
    else
    {
        if (Player)
        {
            Player->Tick(DeltaTime, World);
        }
    }

    if (StatusMessageTimer > 0.0f)
    {
        StatusMessageTimer -= DeltaTime;
    }
}

void FSequencerPanel::OnRender()
{
    ImGui::SetNextWindowSize(ImVec2(850, 320), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin(GetPanelName(), nullptr, ImGuiWindowFlags_NoScrollbar))
    {
        ImGui::End();
        return;
    }

    // 1. 상단 툴바 컨트롤
    DrawToolbar();

    ImGui::Separator();

    // 2. 메인 바디: 좌측(트랙 헤더) + 우측(타임라인 캔버스)
    const float TrackListWidth = 200.0f;
    const float AvailableWidth = ImGui::GetContentRegionAvail().x;
    const float AvailableHeight = ImGui::GetContentRegionAvail().y;

    // 좌측 트랙 리스트
    DrawTrackList(TrackListWidth);

    ImGui::SameLine();

    // 우측 타임라인 캔버스
    DrawTimelineCanvas(AvailableWidth - TrackListWidth - 8.0f, AvailableHeight);

    ImGui::End();
}

void FSequencerPanel::DrawToolbar()
{
    const bool bInPIE = IsPlayingQuery ? IsPlayingQuery() : false;
    if (bInPIE)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.1f, 1.0f), "[PIE Active - Sequencer Paused]");
        ImGui::SameLine(0, 15);
        ImGui::BeginDisabled(true);
    }

    auto* Seq = Player ? Player->GetSequence().get() : nullptr;

    // 아이콘 버튼 헬퍼 람다
    auto DrawIconButton = [](const char* StrId, UTexture2D* Texture, const char* FallbackText, ImVec2 Size) -> bool
    {
        if (Texture && Texture->GetResource() && Texture->GetResource()->GetSRV())
        {
            ImTextureID TexId = (ImTextureID)Texture->GetResource()->GetSRV();
            return ImGui::ImageButton(StrId, TexId, Size, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1));
        }
        return ImGui::Button(FallbackText, ImVec2(Size.x + 6, Size.y + 6));
    };

    const ImVec2 IconSize(18, 18);

    // [재생] / [일시정지]
    if (Player && Player->IsPlaying())
    {
        if (DrawIconButton("##PauseBtn", PauseIcon, "Pause", IconSize))
        {
            Player->Pause();
        }
    }
    else
    {
        if (DrawIconButton("##PlayBtn", PlayIcon, "Play", IconSize))
        {
            if (Player) Player->Play();
        }
    }

    ImGui::SameLine();

    // [정지]
    if (DrawIconButton("##StopBtn", StopIcon, "Stop", IconSize))
    {
        if (Player) Player->Stop(World);
    }

    ImGui::SameLine();

    // [루프 체크박스]
    if (Player)
    {
        bool bLoop = Player->IsLooping();
        if (ImGui::Checkbox("Loop", &bLoop))
        {
            Player->SetLooping(bLoop);
        }
    }

    ImGui::SameLine(0, 20);

    // 현재 재생 시간 표시 (예: 02.40s / 10.00s)
    const float CurTime = Player ? Player->GetCurrentTime() : 0.0f;
    const float MaxTime = Seq ? Seq->Duration : 10.0f;
    ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Time: %05.2fs / %05.2fs", CurTime, MaxTime);

    ImGui::SameLine(0, 25);

    // [+ Track (아웃라이너에서 선택된 액터 추가)]
    if (ImGui::Button("+ Track"))
    {
        AActor* SelectedActor = OutlinerPanel ? OutlinerPanel->GetSelectedActor() : nullptr;
        if (SelectedActor && Seq)
        {
            Seq->AddTrack(SelectedActor->GetName());
        }
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Add current selected actor from Outliner as a track");
    }

    ImGui::SameLine();

    // [ Key (현재 액터 위치/회전을 현재 시간에 기록)]
    if (ImGui::Button("<> Key"))
    {
        AActor* SelectedActor = OutlinerPanel ? OutlinerPanel->GetSelectedActor() : nullptr;
        if (SelectedActor && Seq && Player)
        {
            FSequencerTrack* Track = Seq->FindTrack(SelectedActor->GetName());
            if (!Track)
            {
                Track = Seq->AddTrack(SelectedActor->GetName());
            }
            if (Track)
            {
                Track->AddOrUpdateTransformKey(Player->GetCurrentTime(), SelectedActor->GetActorLocation(), SelectedActor->GetActorRotation(), SelectedActor->GetActorScale3D());
            }
        }
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Add Keyframe at current time for selected actor");
    }

    ImGui::SameLine(0, 30);

    // 시퀀스 길이 (Duration) 조절
    if (Seq)
    {
        ImGui::SetNextItemWidth(70.0f);
        ImGui::DragFloat("Duration", &Seq->Duration, 0.5f, 1.0f, 300.0f, "%.1fs");
    }

    ImGui::SameLine(0, 25);

    // 시퀀스 파일 이름 및 저장/로드
    ImGui::SetNextItemWidth(110.0f);
    ImGui::InputText("##SeqFileName", SequenceFileName, sizeof(SequenceFileName));

    ImGui::SameLine();
    if (ImGui::Button("Save"))
    {
        if (Seq)
        {
            Seq->Name = SequenceFileName;
            FString Path = FString("Assets/Sequences/") + SequenceFileName + ".seq";
            if (Seq->SaveToFile(Path))
            {
                StatusMessage = FString("Saved: ") + SequenceFileName + ".seq";
            }
            else
            {
                StatusMessage = FString("Save failed!");
            }
            StatusMessageTimer = 3.0f;
        }
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Save sequence to Assets/Sequences/[Name].seq");
    }

    ImGui::SameLine();
    if (ImGui::Button("Load"))
    {
        if (Seq)
        {
            FString Path = FString("Assets/Sequences/") + SequenceFileName + ".seq";
            if (Seq->LoadFromFile(Path))
            {
                strncpy_s(SequenceFileName, Seq->Name.c_str(), sizeof(SequenceFileName) - 1);
                if (Player)
                {
                    Player->ScrubTo(0.0f, World);
                }
                StatusMessage = FString("Loaded: ") + SequenceFileName + ".seq";
            }
            else
            {
                StatusMessage = FString("Load failed (File not found)!");
            }
            StatusMessageTimer = 3.0f;
        }
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Load sequence from Assets/Sequences/[Name].seq");
    }

    if (StatusMessageTimer > 0.0f)
    {
        ImGui::SameLine(0, 15);
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "%s", StatusMessage.c_str());
    }

    if (bInPIE)
    {
        ImGui::EndDisabled();
    }
}

void FSequencerPanel::DrawTrackList(float TrackListWidth)
{
    ImGui::BeginChild("TrackListChild", ImVec2(TrackListWidth, 0), true);

    // 상단 눈금자와 높이를 맞추기 위한 여백
    ImGui::Dummy(ImVec2(0, RulerHeight));
    ImGui::Separator();

    auto* Seq = Player ? Player->GetSequence().get() : nullptr;
    if (Seq)
    {
        int32 TrackToRemove = -1;
        for (int32 i = 0; i < Seq->Tracks.Num(); ++i)
        {
            ImGui::PushID(i);

            // 각 트랙 행의 고정 높이 맞춤
            ImGui::BeginGroup();
            
            // 삭제 버튼 [X]
            if (ImGui::SmallButton("x"))
            {
                TrackToRemove = i;
            }
            ImGui::SameLine();

            // 액터 이름 출력
            const FString& TrackName = Seq->Tracks[i].ActorName;
            ImGui::TextUnformatted(TrackName.c_str());

            ImGui::EndGroup();

            ImGui::PopID();

            // 트랙 높이 일정하게 유지
            ImGui::Dummy(ImVec2(0, TrackRowHeight - ImGui::GetItemRectSize().y));
            ImGui::Separator();
        }

        if (TrackToRemove >= 0)
        {
            Seq->Tracks.RemoveAt(static_cast<uint32>(TrackToRemove));
        }
    }

    ImGui::EndChild();
}

void FSequencerPanel::DrawTimelineCanvas(float CanvasWidth, float CanvasHeight)
{
    const bool bInPIE = IsPlayingQuery ? IsPlayingQuery() : false;
    auto* Seq = Player ? Player->GetSequence().get() : nullptr;
    if (!Seq) return;

    ImGui::BeginChild("TimelineScrollChild", ImVec2(CanvasWidth, CanvasHeight), true, ImGuiWindowFlags_HorizontalScrollbar);

    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    const ImVec2 CanvasP0 = ImGui::GetCursorScreenPos();
    const float TotalTimelineWidth = std::max(CanvasWidth, Seq->Duration * PixelsPerSecond + 150.0f);
    const float TotalTimelineHeight = std::max(CanvasHeight, RulerHeight + (Seq->Tracks.Num() * TrackRowHeight) + 50.0f);

    // 스크롤 및 클릭 감지용 더미 영역 확보
    ImGui::InvisibleButton("##TimelineCanvasArea", ImVec2(TotalTimelineWidth, TotalTimelineHeight));
    const bool bCanvasHovered = ImGui::IsItemHovered();
    const ImVec2 MousePos = ImGui::GetIO().MousePos;

    // -------------------------------------------------------------
    // 1. 상단 시간 눈금자 (Time Ruler) 렌더링
    // -------------------------------------------------------------
    const ImVec2 RulerMin = CanvasP0;
    const ImVec2 RulerMax = ImVec2(CanvasP0.x + TotalTimelineWidth, CanvasP0.y + RulerHeight);
    DrawList->AddRectFilled(RulerMin, RulerMax, IM_COL32(32, 33, 36, 255));
    DrawList->AddLine(ImVec2(RulerMin.x, RulerMax.y), RulerMax, IM_COL32(60, 60, 65, 255));

    // 0.2초 단위 보조 눈금 및 1.0초 단위 주 눈금선/텍스트
    for (float t = 0.0f; t <= Seq->Duration + 0.05f; t += 0.2f)
    {
        const float TickX = CanvasP0.x + (t * PixelsPerSecond);
        const bool bMajorTick = (std::fmod(t + 0.01f, 1.0f) < 0.05f);

        if (bMajorTick)
        {
            // 1초 단위 주 눈금
            DrawList->AddLine(ImVec2(TickX, RulerMax.y - 12.0f), ImVec2(TickX, RulerMax.y), IM_COL32(200, 200, 200, 255), 1.0f);
            char TimeStr[16];
            snprintf(TimeStr, sizeof(TimeStr), "%.0fs", t);
            DrawList->AddText(ImVec2(TickX + 3.0f, RulerMin.y + 2.0f), IM_COL32(190, 190, 190, 255), TimeStr);
        }
        else
        {
            // 0.2초 단위 보조 눈금
            DrawList->AddLine(ImVec2(TickX, RulerMax.y - 6.0f), ImVec2(TickX, RulerMax.y), IM_COL32(110, 110, 115, 255), 1.0f);
        }
    }

    // -------------------------------------------------------------
    // 2. 트랙별 레인(Lanes) 및 키프레임 다이아몬드 렌더링
    // -------------------------------------------------------------
    int32 KeyToRemoveTrackIdx = -1;
    int32 KeyToRemoveKeyIdx = -1;

    for (int32 i = 0; i < Seq->Tracks.Num(); ++i)
    {
        auto& Track = Seq->Tracks[i];
        const float LaneTop = CanvasP0.y + RulerHeight + (i * TrackRowHeight);
        const float LaneBottom = LaneTop + TrackRowHeight;

        // 행 배경 및 구분선
        const ImU32 RowBg = (i % 2 == 0) ? IM_COL32(24, 25, 28, 255) : IM_COL32(28, 29, 32, 255);
        DrawList->AddRectFilled(ImVec2(CanvasP0.x, LaneTop), ImVec2(CanvasP0.x + TotalTimelineWidth, LaneBottom), RowBg);
        DrawList->AddLine(ImVec2(CanvasP0.x, LaneBottom), ImVec2(CanvasP0.x + TotalTimelineWidth, LaneBottom), IM_COL32(45, 45, 50, 255));

        // 키프레임 다이아몬드 그리기 및 인터랙션
        for (int32 k = 0; k < Track.Keys.Num(); ++k)
        {
            auto& Key = Track.Keys[k];
            const float KeyX = CanvasP0.x + (Key.Time * PixelsPerSecond);
            const float KeyY = LaneTop + (TrackRowHeight * 0.5f);
            const ImVec2 KeyCenter(KeyX, KeyY);

            // 마우스 호버 검사 (반경 7픽셀)
            const float DistSq = (MousePos.x - KeyX) * (MousePos.x - KeyX) + (MousePos.y - KeyY) * (MousePos.y - KeyY);
            const bool bKeyHovered = DistSq <= (7.0f * 7.0f);

            // 키프레임 드래그 시작
            if (bKeyHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                DraggingTrackIdx = i;
                DraggingKeyIdx = k;
            }

            // 키프레임 우클릭 시 삭제
            if (bKeyHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                KeyToRemoveTrackIdx = i;
                KeyToRemoveKeyIdx = k;
            }

            // 다이아몬드 색상 결정 (호버/드래그 시 밝은 노란색 강조)
            ImU32 FillColor = IM_COL32(235, 170, 50, 255);
            ImU32 BorderColor = IM_COL32(255, 255, 255, 255);
            if (DraggingTrackIdx == i && DraggingKeyIdx == k)
            {
                FillColor = IM_COL32(255, 220, 80, 255);
            }
            else if (bKeyHovered)
            {
                FillColor = IM_COL32(255, 200, 70, 255);
            }

            // ImDrawList 기반 키프레임 다이아몬드 렌더링
            DrawDiamond(DrawList, KeyCenter, 6.0f, FillColor, BorderColor);
        }
    }

    // 키프레임 삭제 처리
    if (KeyToRemoveTrackIdx >= 0 && KeyToRemoveKeyIdx >= 0)
    {
        Seq->Tracks[KeyToRemoveTrackIdx].RemoveKey(KeyToRemoveKeyIdx);
    }

    // -------------------------------------------------------------
    // 3. 마우스 드래그 조작 (플레이헤드 스크러빙 및 키프레임 시간 이동)
    // -------------------------------------------------------------
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        bIsScrubbing = false;
        if (DraggingTrackIdx >= 0 && DraggingKeyIdx >= 0)
        {
            Seq->Tracks[DraggingTrackIdx].SortKeys();
        }
        DraggingTrackIdx = -1;
        DraggingKeyIdx = -1;
    }

    // 키프레임 드래그 시간 변경
    if (DraggingTrackIdx >= 0 && DraggingKeyIdx >= 0 && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        float NewTime = (MousePos.x - CanvasP0.x) / PixelsPerSecond;
        NewTime = FMath::Clamp(NewTime, 0.0f, Seq->Duration);
        Seq->Tracks[DraggingTrackIdx].Keys[DraggingKeyIdx].Time = NewTime;
    }
    // 타임라인 / 눈금자 클릭으로 플레이헤드 스크러빙
    else if (!bInPIE && bCanvasHovered && ImGui::IsMouseDown(ImGuiMouseButton_Left) && DraggingTrackIdx == -1)
    {
        float ClickedTime = (MousePos.x - CanvasP0.x) / PixelsPerSecond;
        if (Player)
        {
            Player->ScrubTo(ClickedTime, World);
        }
        bIsScrubbing = true;
    }

    // -------------------------------------------------------------
    // 4. 붉은색 세로 플레이헤드 및 상단 마커 렌더링
    // -------------------------------------------------------------
    const float CurrentTime = Player ? Player->GetCurrentTime() : 0.0f;
    const float PlayheadX = CanvasP0.x + (CurrentTime * PixelsPerSecond);

    // 전체 세로 관통 붉은선
    DrawList->AddLine(ImVec2(PlayheadX, CanvasP0.y), ImVec2(PlayheadX, CanvasP0.y + TotalTimelineHeight), IM_COL32(235, 55, 50, 255), 1.5f);

    // 상단 눈금자에 위치하는 붉은색 헤더 마커
    DrawPlayheadMarker(DrawList, ImVec2(PlayheadX, CanvasP0.y + (RulerHeight * 0.5f)), 7.0f, IM_COL32(235, 55, 50, 255));

    ImGui::EndChild();
}

void FSequencerPanel::DrawDiamond(ImDrawList* DrawList, ImVec2 Center, float Radius, ImU32 FillColor, ImU32 BorderColor)
{
    const ImVec2 Pts[4] = {
        ImVec2(Center.x, Center.y - Radius),
        ImVec2(Center.x + Radius, Center.y),
        ImVec2(Center.x, Center.y + Radius),
        ImVec2(Center.x - Radius, Center.y)
    };
    DrawList->AddConvexPolyFilled(Pts, 4, FillColor);
    DrawList->AddPolyline(Pts, 4, BorderColor, ImDrawFlags_Closed, 1.2f);
}

void FSequencerPanel::DrawPlayheadMarker(ImDrawList* DrawList, ImVec2 Center, float Size, ImU32 FillColor)
{
    // 눈금자 상단 역삼각형 형태 마커
    const ImVec2 Pts[3] = {
        ImVec2(Center.x - Size, Center.y - Size),
        ImVec2(Center.x + Size, Center.y - Size),
        ImVec2(Center.x, Center.y + Size)
    };
    DrawList->AddTriangleFilled(Pts[0], Pts[1], Pts[2], FillColor);
    DrawList->AddTriangle(Pts[0], Pts[1], Pts[2], IM_COL32(255, 255, 255, 255), 1.0f);
}
