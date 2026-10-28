#pragma once

#include "Editor/EditorUI/EditorPanel.h"
#include "Sequencer/SequencePlayer.h"
#include <memory>
#include <functional>

class UWorld;
class FOutlinerPanel;
class UTexture2D;

class FSequencerPanel : public IEditorPanel
{
public:
    FSequencerPanel();
    virtual ~FSequencerPanel() override = default;

    virtual bool Init() override;
    virtual void Tick(float DeltaTime) override;
    virtual void OnRender() override;
    virtual const char* GetPanelName() const override { return "Sequencer"; }

    void SetWorld(UWorld* InWorld) { World = InWorld; }
    void SetOutlinerPanel(FOutlinerPanel* InOutliner) { OutlinerPanel = InOutliner; }
    void SetIsPlayingQuery(std::function<bool()> InQuery) { IsPlayingQuery = InQuery; }

    TSharedPtr<FSequencePlayer> GetPlayer() const { return Player; }

private:
    void DrawToolbar();
    void DrawTrackList(float TrackListWidth);
    void DrawTimelineCanvas(float CanvasWidth, float CanvasHeight);

    // ImDrawList 기반 다이아몬드 & 마커 드로잉 헬퍼
    void DrawDiamond(ImDrawList* DrawList, ImVec2 Center, float Radius, ImU32 FillColor, ImU32 BorderColor);
    void DrawPlayheadMarker(ImDrawList* DrawList, ImVec2 Center, float Size, ImU32 FillColor);

private:
    TSharedPtr<FSequencePlayer> Player;
    UWorld* World = nullptr;
    FOutlinerPanel* OutlinerPanel = nullptr;
    std::function<bool()> IsPlayingQuery;

    // 툴바 아이콘 텍스처
    UTexture2D* PlayIcon = nullptr;
    UTexture2D* PauseIcon = nullptr;
    UTexture2D* StopIcon = nullptr;

    // 타임라인 렌더링 및 뷰 파라미터
    float PixelsPerSecond = 100.0f;    // 1초당 가로 픽셀 폭
    float TrackRowHeight = 32.0f;      // 각 트랙 행 세로 높이
    float RulerHeight = 28.0f;         // 상단 시간 눈금자 높이
    float TimelineScrollX = 0.0f;      // 타임라인 가로 스크롤 오프셋

    // 드래그 조작 상태
    bool bIsScrubbing = false;         // 플레이헤드 드래그 중
    int32 DraggingTrackIdx = -1;       // 키프레임 드래그 중인 트랙 인덱스
    int32 DraggingKeyIdx = -1;         // 키프레임 드래그 중인 키 인덱스

    // 저장 / 로드 파일명 및 상태 알림
    char SequenceFileName[64] = "MySequence";
    FString StatusMessage;
    float StatusMessageTimer = 0.0f;
};
