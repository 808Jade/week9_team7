#pragma once
#include "Editor/EditorUI/EditorPanel.h"

#include "Editor/Gizmo/Gizmo.h"

#include "GameFramework/Actor/ParticleActor.h"
#include "GameFramework/Actor/StaticMeshActor.h"
#include "GameFramework/Actor/DirectionalLightActor.h"
#include "GameFramework/Actor/PointLightActor.h"
#include "GameFramework/Actor/SpotLightActor.h"
#include "GameFramework/Actor/RectLightActor.h"
#include "GameFramework/Actor/SkyLightActor.h"
#include "GameFramework/Actor/TextRenderActor.h"
#include "GameFramework/Actor/ExponentialHeightFogActor.h"
#include "GameFramework/Actor/FireBallActor.h"
#include "GameFramework/Actor/ProjectileActor.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PortalPawn.h"
#include "GameFramework/Actor/PortalCubeActor.h"
#include "GameFramework/Actor/PortalGunActor.h"
#include "GameFramework/Actor/PortalActor.h"
#include "GameFramework/Actor/MapCubeActor.h"
#include "GameFramework/Actor/StandButtonActor.h"
#include "GameFramework/Actor/WeightedButtonActor.h"
#include "GameFramework/Actor/DoorActor.h"
#include "GameFramework/Actor/CubeDropActor.h"
#include "GameFramework/Actor/PortalCameraActor.h"
#include "GameFramework/Actor/PortalTurretActor.h"

class FViewportsPanel;

// 액터 생성과 카메라·기즈모 편집에 필요한 패널 상태를 보관한다.
class FEditorControlsPanel : public IEditorPanel
{
public:
	bool Init() override;
	void Tick(float DeltaTime)override;
	void OnRender() override;
	const char* GetPanelName() const override { return "Editor Controls"; }
	inline void SetGizmo(FGizmo* InGizmo) { Gizmo = InGizmo; }
	inline void SetWorld(UWorld* InWorld) { World = InWorld; }
	inline void SetViewportsPanel(FViewportsPanel* InPanel) { ViewportsPanel = InPanel; }
	inline void SetIsPlayingQuery(std::function<bool()> InQuery) { IsPlayingQuery = InQuery; }

	float DeltaTime = 1.0f;
	UWorld* World = nullptr;
	FViewportsPanel* ViewportsPanel = nullptr;

	void AddActor(uint32 Index);

	int32 SelectedIndex = 0;

	const char* Items[18] = {"StaticMesh", "Particle", "Text", "HeightFog", "FireBall", "Projectile", "PlayerStart", "PortalPawn", "PortalCube", "PortalGun", "Portal", "MapCube", "StandButton", "WeightedButton", "Door", "CubeDrop", "PortalCamera", "PortalTurret"};

	FGizmo* Gizmo = nullptr;
	int32 GizmoSelectedIndex = 0;
	const char* GizmoItems[3] ={"Location","Rotation","Scale"};

	int32 SpaceSelectedIndex = 0;
	const char* SpaceItems[2] ={"Local","World"};

	int32 CurrentSnapIdx = 4; // 1.0m
	int32 CurrentRotIdx = 2; // 15 deg

	TArray<UClass*> Classes
	{
		AStaticMeshActor::StaticClass(),
		AParticleActor::StaticClass(),
		ATextRenderActor::StaticClass(),
		AExponentialHeightFogActor::StaticClass(),
		AFireBallActor::StaticClass(),
		AProjectileActor::StaticClass(),
		APlayerStart::StaticClass(),
		APortalPawn::StaticClass(),
		APortalCubeActor::StaticClass(),
		APortalGunActor::StaticClass(),
		APortalActor::StaticClass(),
		AMapCubeActor::StaticClass(),
		AStandButtonActor::StaticClass(),
		AWeightedButtonActor::StaticClass(),
		ADoorActor::StaticClass(),
		ACubeDropActor::StaticClass(),
		APortalCameraActor::StaticClass(),
		APortalTurretActor::StaticClass(),
	};

private:
	static constexpr float SectionGap = 10.0f;
	static constexpr float SubsectionGap = 4.0f;

    void DrawCameraProperties();
	std::function<bool()> IsPlayingQuery;
};
