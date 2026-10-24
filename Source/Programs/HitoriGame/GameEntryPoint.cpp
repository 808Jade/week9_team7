#include "EnginePCH.h"
#include "Launch/EntryPoint.h"
#include "Engine/GameEngine.h"

#include "GameFramework/Actor/DirectionalLightActor.h"
#include "GameFramework/Actor/SkyLightActor.h"
#include "GameFramework/Actor/PointLightActor.h"
#include "GameFramework/Actor/SpotLightActor.h"
#include "GameFramework/Actor/RectLightActor.h"
#include "GameFramework/Actor/StaticMeshActor.h"
#include "GameFramework/Actor/ParticleActor.h"
#include "GameFramework/Actor/TextRenderActor.h"

UClass* GetEngineClass()
{
    // [리플렉션 클래스 링커 제거 방지 (Dead Code Elimination Prevention)]
    // HitoriEngine은 정적 라이브러리(.lib)이므로, C++ 코드에서 직접 참조되지 않은 액터는
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

    return UGameEngine::StaticClass();
}
