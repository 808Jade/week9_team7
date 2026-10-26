#include "EnginePCH.h"
#include "GameEngine.h"

#include "Launch/LaunchEngineLoop.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Serialization/JsonArchive.h"
#include "Render/SceneRenderer.h"
#include "Render/RenderCommand.h"
#include "Render/Swapchain.h"
#include "Render/Texture2D.h"
#include "Render/RenderGraph/RenderGraph.h"
#include "Render/SkyboxRenderer.h"
#include "UObject/UObjectIterator.h"
#include "GameFramework/Actor/PortalActor.h"
#include "Engine/Level.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/DefaultPawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PortalPawn.h"
#include "GameFramework/PortalPlayerController.h"
#include "Components/LuaScriptComponent.h"
#include "Core/Window.h"

#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace
{
    bool GameImGuiWndProcHook(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam) != 0;
    }
}

#define U8(str) reinterpret_cast<const char*>(u8##str)

FEngineConfig UGameEngine::GetConfig() const
{
	FEngineConfig Desc;
	Desc.Title = L"Hitori Game";
	Desc.Width = 1280;
	Desc.Height = 720;
	Desc.bBorderless = false;
	Desc.SyncInterval = 0;
	Desc.bCreateDepthBuffer = true;   
	Desc.bExitOnEscape = false; // ESC 누를 때 게임이 즉시 종료되지 않고 설정창을 열도록 false 설정
	return Desc;
}

// 기본 월드(UWorld) 생성 및 시작 씬(Default Scene) 파일 역직렬화(로드)
// 플레이어 카메라 및 FGameViewportClient 초기화
bool UGameEngine::Init()
{
	if (!Super::Init()) return false;

    // 1. 월드 컨텍스트 및 Game World 생성
    FWorldContext& Context = CreateNewWorldContext(EWorldType::Game);
    GameWorld = UWorld::CreateWorld(EWorldType::Game, true);
    Context.SetCurrentWorld(GameWorld);

    // 2. 게임 인스턴스(UGameInstance) 생성 및 초기화 (FLuaManager 인스턴스화 포함)
    ULuaScriptComponent::StaticClass();
    GameInstance = NewObject<UGameInstance>();
    GameInstance->SetWorldContext(&Context);
    Context.OwningGameInstance = GameInstance;
    GameWorld->SetGameInstance(GameInstance);
    GameInstance->Init();

    // 3. 게임 뷰포트 클라이언트 생성 및 초기화
    GameViewport = MakeShared<FGameViewportClient>();
    GameViewport->Init(Context, this);
    GameViewport->SetGameInstance(GameInstance);
    Context.GameViewport = GameViewport;

    // 4. 기본 씬 로드 (PortalComplex.scene 우선 탐색, 없으면 Bocchi.scene 또는 상대 경로 탐색)
    FString DefaultScene = "Scenes/PortalComplex.scene";
    if (!std::filesystem::exists(DefaultScene))
    {
        if (std::filesystem::exists("../../../" + DefaultScene))
        {
            DefaultScene = "../../../" + DefaultScene;
        }
        else if (std::filesystem::exists("Scenes/Bocchi.scene"))
        {
            DefaultScene = "Scenes/Bocchi.scene";
        }
        else if (std::filesystem::exists("../../../Scenes/Bocchi.scene"))
        {
            DefaultScene = "../../../Scenes/Bocchi.scene";
        }
    }

    if (!FJsonArchive::LoadWorld(GameWorld, DefaultScene))
    {
        // 파일 로드 실패 시에도 빈 월드로 계속 진행
        OutputDebugStringA("Scene Load Failed!\n");
    }

    if (DefaultScene.find("Bocchi") != std::string::npos)
    {
        if (ACameraActor* MainCam = GameWorld->GetMainCamera())
        {
            if (UCameraComponent* CamCom = MainCam->GetCameraComponent())
            {
                CamCom->SetRelativeLocation(FVector(-6.7f, -11.2f, 11.3f));
                CamCom->SetRelativeRotation(FRotator(30.0f, 45.0f, 0.0f));
                CamCom->SetMoveSpeed(10.0f);
                CamCom->SetMouseSensitivity(0.2f);
            }
        }
    }

    SkyboxRenderer = MakeUnique<FSkyboxRenderer>();
    FString SkyPath = "Assets/SkySphere/Sky.hdr";
    if (!std::filesystem::exists(SkyPath))
    {
        if (std::filesystem::exists("../../../" + SkyPath)) SkyPath = "../../../" + SkyPath;
        else if (std::filesystem::exists("../../" + SkyPath)) SkyPath = "../../" + SkyPath;
    }
    SkyboxRenderer->Init(SkyPath);

    TextRenderer = MakeUnique<FTextRenderer>();
    TextRenderer->Init();

    FXAARenderer = MakeUnique<FFXAARenderer>();
    FXAARenderer->Init(Renderer);

    APawn* PlayerPawn = GameWorld->GetPlayerPawn();
    if (!PlayerPawn)
    {
        if (ULevel* PersistentLevel = GameWorld->GetPersistentLevel())
        {
            for (AActor* Actor : PersistentLevel->GetActors())
            {
                if (APawn* Pawn = Cast<APawn>(Actor))
                {
                    PlayerPawn = Pawn;
                    break;
                }
            }
        }
    }
    if (!PlayerPawn)
    {
        ACameraActor* MainCamera = GameWorld->GetMainCamera();
        FTransform Transform = MainCamera ? MainCamera->GetActorTransform() : FTransform::Identity;
        PlayerPawn = GameWorld->SpawnActor<ADefaultPawn>(NAME_None, &Transform);
    }
    GameWorld->SetPlayerPawn(PlayerPawn);

    APlayerController* PlayerController = GameWorld->GetPlayerController();
    if (!PlayerController)
    {
        if (Cast<APortalPawn>(PlayerPawn))
            PlayerController = GameWorld->SpawnActor<APortalPlayerController>();
        else
            PlayerController = GameWorld->SpawnActor<APlayerController>();
    }
    GameWorld->SetPlayerController(PlayerController);

    if (PlayerController && PlayerPawn)
    {
        PlayerController->Possess(PlayerPawn);
    }

    // 5. ImGui 초기화 (ESC 설정창 및 인게임 디버그 오버레이용)
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // 한글 폰트 로드 시도
    const ImWchar* KoreanRanges = io.Fonts->GetGlyphRangesKorean();
    if (std::filesystem::exists("HMKMRHD.ttf"))
    {
        io.Fonts->AddFontFromFileTTF("HMKMRHD.ttf", 18.0f, nullptr, KoreanRanges);
    }
    else if (std::filesystem::exists("../../../HMKMRHD.ttf"))
    {
        io.Fonts->AddFontFromFileTTF("../../../HMKMRHD.ttf", 18.0f, nullptr, KoreanRanges);
    }
    else if (std::filesystem::exists("C:/Windows/Fonts/malgun.ttf"))
    {
        io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/malgun.ttf", 18.0f, nullptr, KoreanRanges);
    }
    else
    {
        io.Fonts->AddFontDefault();
    }

    ImGui_ImplWin32_Init(GetEngineLoop().GetMainWindow()->GetHandle());
    ImGui_ImplDX11_Init(GetEngineLoop().GetRenderDevice()->GetDevice(), GetEngineLoop().GetRenderDevice()->GetContext());
    FWindow::SetWndProcHook(&GameImGuiWndProcHook);

    ImGui::StyleColorsDark();
    ImGuiStyle& Style = ImGui::GetStyle();
    Style.WindowRounding = 8.0f;
    Style.FrameRounding = 5.0f;
    Style.PopupRounding = 6.0f;
    Style.GrabRounding = 5.0f;
    Style.WindowPadding = ImVec2(24.0f, 20.0f);
    Style.ItemSpacing = ImVec2(12.0f, 14.0f);

    bImGuiInitialized = true;

    // 초기 마우스 감도 동기화
    ApplyMouseSensitivity();

    // 6. Start gameplay
    GameWorld->BeginPlay();
    return true;
}

void UGameEngine::SetSettingsWindowVisible(bool bVisible)
{
    bShowSettingsWindow = bVisible;

    if (bShowSettingsWindow)
    {
        // 설정창 열림: 마우스 커서 표시 및 잠금 해제
        if (GameWorld)
        {
            if (APortalPlayerController* PPC = Cast<APortalPlayerController>(GameWorld->GetPlayerController()))
            {
                PPC->SetMouseCapture(false);
            }
        }
        while (::ShowCursor(TRUE) < 0);
    }
    else
    {
        // 설정창 닫힘 (게임 복귀): 마우스 잠금 및 커서 숨김
        if (GameWorld)
        {
            if (APortalPlayerController* PPC = Cast<APortalPlayerController>(GameWorld->GetPlayerController()))
            {
                PPC->SetMouseCapture(true);
            }
        }
        else
        {
            while (::ShowCursor(FALSE) >= 0);
        }
    }
}

void UGameEngine::ApplyMouseSensitivity()
{
    if (!GameWorld) return;

    if (APlayerController* PC = GameWorld->GetPlayerController())
    {
        PC->SetLookSensitivity(MouseSensitivity);
    }
    if (APortalPawn* Pawn = Cast<APortalPawn>(GameWorld->GetPlayerPawn()))
    {
        Pawn->SetLookSensitivity(MouseSensitivity * 10.0f);
    }
    if (ACameraActor* Cam = GameWorld->GetMainCamera())
    {
        if (UCameraComponent* CamCom = Cam->GetCameraComponent())
        {
            CamCom->SetMouseSensitivity(MouseSensitivity);
        }
    }
}

void UGameEngine::DrawSettingsWindow()
{
    const uint32 ViewportW = GetEngineLoop().GetViewportWidth();
    const uint32 ViewportH = GetEngineLoop().GetViewportHeight();

    ImVec2 Center(ViewportW * 0.5f, ViewportH * 0.5f);
    ImGui::SetNextWindowPos(Center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(440.0f, 240.0f), ImGuiCond_Always);

    ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    if (ImGui::Begin(U8("설정 (Settings)"), nullptr, WindowFlags))
    {
        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        ImGui::Text(U8("마우스 조작 설정"));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, 5.0f));

        // 마우스 감도 슬라이더
        ImGui::Text(U8("마우스 감도:"));
        ImGui::SameLine();
        if (ImGui::SliderFloat("##MouseSensitivity", &MouseSensitivity, 0.01f, 1.0f, "%.2f"))
        {
            ApplyMouseSensitivity();
        }

        ImGui::Dummy(ImVec2(0.0f, 15.0f));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, 10.0f));

        // 버튼 배치
        float ButtonWidth = 185.0f;
        float ButtonHeight = 38.0f;

        if (ImGui::Button(U8("닫기 (계속하기)"), ImVec2(ButtonWidth, ButtonHeight)))
        {
            SetSettingsWindowVisible(false);
        }

        ImGui::SameLine(0.0f, 20.0f);

        // 종료 버튼은 붉은색 계열로 강조
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.20f, 0.20f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.25f, 0.25f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.55f, 0.15f, 0.15f, 1.00f));
        if (ImGui::Button(U8("게임 종료"), ImVec2(ButtonWidth, ButtonHeight)))
        {
            GetEngineLoop().RequestExit();
        }
        ImGui::PopStyleColor(3);
    }
    ImGui::End();
}

void UGameEngine::TickInput(float DeltaTime)
{
    if (!GameViewport) return;

    // ESC 키: 설정창 토글
    if (FInputSystem::IsKeyPressed(EKeyCode::Escape))
    {
        SetSettingsWindowVisible(!bShowSettingsWindow);
    }

    // 설정창이 열려있는 동안에는 인게임 마우스 시점 회전 및 플레이어 조작 차단 (ImGui만 조작)
    if (bShowSettingsWindow)
    {
        return;
    }

    // 키보드 입력
    for (int32 Key = 0; Key < 256; ++Key)
    {
        EKeyCode KeyCode = static_cast<EKeyCode>(Key);
        if (FInputSystem::IsKeyPressed(KeyCode))
            GameViewport->InputKey(nullptr, Key, true);
        if (FInputSystem::IsKeyReleased(KeyCode))
            GameViewport->InputKey(nullptr, Key, false);
    }

    // 마우스 입력
    constexpr EMouseButton MouseButtons[] = { EMouseButton::Left, EMouseButton::Right, EMouseButton::Middle };
    for (EMouseButton Button : MouseButtons)
    {
        const int32 Key = Button == EMouseButton::Left ? static_cast<int32>(EKeyCode::LButton)
            : (Button == EMouseButton::Right ? static_cast<int32>(EKeyCode::RButton) : static_cast<int32>(EKeyCode::MButton));
        if (FInputSystem::IsMousePressed(Button))
            GameViewport->InputKey(nullptr, Key, true);
        if (FInputSystem::IsMouseReleased(Button))
            GameViewport->InputKey(nullptr, Key, false);
    }
    const float MouseX = static_cast<float>(FInputSystem::GetMouseDeltaX());
    const float MouseY = static_cast<float>(FInputSystem::GetMouseDeltaY());
    GameViewport->InputAxis(nullptr, 0, EGameInputAxis::MouseX, MouseX, DeltaTime);
    GameViewport->InputAxis(nullptr, 0, EGameInputAxis::MouseY, MouseY, DeltaTime);
}

// 입력 업데이트 및 액터/컴포넌트 틱 (World->Tick(DeltaTime))
// 백버퍼에 게임 씬 렌더링 후 ImGui 설정창 오버레이 렌더링
void UGameEngine::Tick(float DeltaTime)
{
    if (!GameWorld || !GameViewport) return;

    TickInput(DeltaTime);
    GameWorld->Tick(DeltaTime);
    GameWorld->GetScene().UpdateAllTransforms();

    RenderFrame();
}

void UGameEngine::RenderFrame()
{
    if (!GameWorld || !GameViewport) return;
    const uint32 Width = GetEngineLoop().GetViewportWidth();
    const uint32 Height = GetEngineLoop().GetViewportHeight();
    if (Width == 0 || Height == 0) return;

    // 1. 카메라 시점 생성 (MainCamera의 위치/회전/행렬을 자동으로 가져옴)
    FRect ViewRect{ 0.0f, 0.0f, static_cast<float>(Width), static_cast<float>(Height) };
    FSceneView SceneView = GameViewport->CalcSceneView(ViewRect);
    SceneView.bGameView = true; // 게임 뷰 모드 활성화

    // 2. 렌더 옵션 설정 (그림자 퀄리티 향상)
    FSceneRenderOptions Options;
    Options.DirectionalShadowMapSize = 4096;
    Options.PointShadowMapSize = 512;
    Options.SpotShadowMapSize = 512;
    Options.PcfMode = EPcfMode::PCSS;

    // 2.5 포탈 뷰 먼저 렌더링
    APortalActor::RenderPortalViews(GameWorld, GetEngineLoop().GetRenderer(),
        SkyboxRenderer.get(), SceneView, Options);

    // 3. 씬 렌더링
    FSceneRenderer SceneRenderer(GameWorld, SceneView, GameViewport->GetRenderData());
    SceneRenderer.InitViews(GetEngineLoop().GetRenderer(), Options);

    FRenderGraphBuilder Graph;
    FScenePassCallbacks Callbacks;
    Callbacks.PortalSurfaces = [this, &SceneView] 
        {
            APortalActor::DrawPortalSurfaces(GameWorld, SceneView);
        };

    Callbacks.SceneText = [this, &SceneView]
        {
            if (!TextRenderer) return;
            for (TObjectIterator<UTextRenderComponent> Text; Text; ++Text)
            {
                if (!Text || !Text->IsRegistered() || !Text->GetFont() || !Text->IsShown(SceneView.bGameView) ||
                    Text->GetOwner()->GetWorld() != GameWorld) continue;
                TextRenderer->OnRender(Text->GetText(), Text->GetWorldMatrix(), Text->GetTextSize(),
                    *Text->GetFont(), SceneView.ViewProjectionMatrix);
            }
        };

    if (SceneRenderer.AddViewportPasses(Graph, GetEngineLoop().GetRenderer(),
        GameViewport->GetViewTargets(), Options, SkyboxRenderer.get(), Callbacks))
    {
        Graph.Execute();
        RenderCommand::EndRenderPass();

        // 백버퍼로 블릿
        FTexture2D* Backbuffer = GetEngineLoop().GetSwapchain()->GetBackbuffer();
        FTexture2D* ColorTarget = GameViewport->GetColorTarget();
        if (Backbuffer && ColorTarget &&
            Backbuffer->GetWidth() == ColorTarget->GetWidth() &&
            Backbuffer->GetHeight() == ColorTarget->GetHeight())
        {
            RenderCommand::GetContext()->CopyResource(Backbuffer->GetRawPtr(), ColorTarget->GetRawPtr());
        }

        // ImGui 설정창 렌더링
        if (bImGuiInitialized)
        {
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            if (bShowSettingsWindow)
            {
                DrawSettingsWindow();
            }

            ImGui::Render();

            if (bShowSettingsWindow)
            {
                RenderCommand::BindRenderPassNoClear(Backbuffer, nullptr, Width, Height);
                ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
                RenderCommand::EndRenderPass();
            }
        }
    }
}

// 로드된 월드 및 UObject 자원 정리
void UGameEngine::PreExit()
{
    if (bImGuiInitialized)
    {
        FWindow::SetWndProcHook(nullptr);
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        bImGuiInitialized = false;
    }

    if (GameInstance)
    {
        GameInstance->Shutdown();
        GameInstance = nullptr;
    }
    if (GameWorld)
    {
        GameWorld->ClearWorld();
        GameWorld = nullptr;
    }
    if (GameViewport)
    {
        GameViewport->Reset();
    }
}
