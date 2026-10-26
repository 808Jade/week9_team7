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

FEngineConfig UGameEngine::GetConfig() const
{
	FEngineConfig Desc;
	Desc.Title = L"Hitori Game";
	Desc.Width = 1280;
	Desc.Height = 720;
	Desc.bBorderless = false;
	Desc.SyncInterval = 0;
	Desc.bCreateDepthBuffer = true;   
	Desc.bExitOnEscape = true;        
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

    // 2. 게임 뷰포트 클라이언트 생성 및 초기화
    GameViewport = MakeShared<FGameViewportClient>();
    GameViewport->Init(Context, this);
    Context.GameViewport = GameViewport;

    // 2-1. 게임 인스턴스(UGameInstance) 생성 및 LuaManager 초기화
    UGameInstance* GameInstance = NewObject<UGameInstance>();
    GameInstance->SetWorldContext(&Context);
    Context.OwningGameInstance = GameInstance;
    GameWorld->SetGameInstance(GameInstance);
    GameViewport->SetGameInstance(GameInstance);
    GameInstance->Init();

    // 3. 기본 씬 로드 (PortalComplex.scene 우선 탐색, 없으면 Bocchi.scene 또는 상대 경로 탐색)
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

    // 4. 게임 플레이 시작
    GameWorld->BeginPlay();
    return true;
}

// 입력 업데이트 및 액터/컴포넌트 틱 (World->Tick(DeltaTime))
// ImGui 오버레이 없이 스왑체인 백버퍼에 게임 씬 직접 렌더링(SceneRenderer)
void UGameEngine::Tick(float DeltaTime)
{
    if (!GameWorld || !GameViewport) return;

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
    }
}

// 로드된 월드 및 UObject 자원 정리
void UGameEngine::PreExit()
{
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
