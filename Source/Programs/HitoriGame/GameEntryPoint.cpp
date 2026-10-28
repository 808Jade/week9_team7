#include "EnginePCH.h"
#include "Launch/EntryPoint.h"
#include "Engine/GameEngine.h"

// Lights
#include "GameFramework/Actor/DirectionalLightActor.h"
#include "GameFramework/Actor/SkyLightActor.h"
#include "GameFramework/Actor/PointLightActor.h"
#include "GameFramework/Actor/SpotLightActor.h"
#include "GameFramework/Actor/RectLightActor.h"

// Meshes & VFX
#include "GameFramework/Actor/StaticMeshActor.h"
#include "GameFramework/Actor/ParticleActor.h"
#include "GameFramework/Actor/TextRenderActor.h"
#include "GameFramework/Actor/MapCubeActor.h"
#include "GameFramework/Actor/PortalCubeActor.h"

// Gameplay & Puzzles
#include "GameFramework/Actor/DoorActor.h"
#include "GameFramework/Actor/CubeDropActor.h"
#include "GameFramework/Actor/PortalCameraActor.h"
#include "GameFramework/Actor/PortalTurretActor.h"
#include "GameFramework/Actor/StandButtonActor.h"
#include "GameFramework/Actor/WeightedButtonActor.h"
#include "GameFramework/Actor/ElevatorTriggerActor.h"
#include "GameFramework/Actor/TriggerActor.h"
#include "GameFramework/PlayerStart.h"

// Pawn & Controller & Portals
#include "GameFramework/PortalPawn.h"
#include "GameFramework/PortalPlayerController.h"
#include "GameFramework/Actor/PortalGunActor.h"
#include "GameFramework/Actor/PortalActor.h"

// UI Actors
#include "GameFramework/Actor/UIManagerActor.h"
#include "GameFramework/Actor/HUDCanvasActor.h"
#include "GameFramework/Actor/TitleCanvasActor.h"
#include "GameFramework/Actor/EndingCanvasActor.h"
#include "GameFramework/Actor/PauseCanvasActor.h"

// Components (Lua scripting, UI, Collision)
#include "Components/LuaScriptComponent.h"
#include "Components/UIImageComponent.h"
#include "Components/UITextComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"

UClass* GetEngineClass()
{
    // [리플렉션 클래스 링커 제거 방지 (Dead Code Elimination Prevention)]
    // HitoriEngine은 정적 라이브러리(.lib)이므로, C++ 코드에서 직접 참조되지 않은 액터/컴포넌트는
    // MSVC 링커 최적화 과정에서 최종 바이너리(.exe)에서 누락(Strip)됩니다.
    // 씬 파일(.scene) 로드 시 FindClass()를 통해 동적 생성(리플렉션)이 가능하도록
    // StaticClass()를 호출하여 링커가 해당 .obj 파일들을 포함하도록 강제합니다.
    AParticleActor::StaticClass();
    ATextRenderActor::StaticClass();
    ADirectionalLightActor::StaticClass();
    ASkyLightActor::StaticClass();
    APointLightActor::StaticClass();
    ASpotLightActor::StaticClass();
    ARectLightActor::StaticClass();
    AStaticMeshActor::StaticClass();
    AMapCubeActor::StaticClass();
    APortalCubeActor::StaticClass();

    ADoorActor::StaticClass();
    ACubeDropActor::StaticClass();
    APortalCameraActor::StaticClass();
    APortalTurretActor::StaticClass();
    AStandButtonActor::StaticClass();
    AWeightedButtonActor::StaticClass();
    AElevatorTriggerActor::StaticClass();
    ATriggerActor::StaticClass();
    APlayerStart::StaticClass();

    APortalPawn::StaticClass();
    APortalPlayerController::StaticClass();
    APortalGunActor::StaticClass();
    APortalActor::StaticClass();

    AUIManagerActor::StaticClass();
    AHUDCanvasActor::StaticClass();
    ATitleCanvasActor::StaticClass();
    AEndingCanvasActor::StaticClass();
    APauseCanvasActor::StaticClass();

    ULuaScriptComponent::StaticClass();
    UUIImageComponent::StaticClass();
    UUITextComponent::StaticClass();
    UBoxComponent::StaticClass();
    USphereComponent::StaticClass();
    UCapsuleComponent::StaticClass();

    return UGameEngine::StaticClass();
}
