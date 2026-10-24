#include "EnginePCH.h"
#include "LuaBindings.h"

#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "Core/EngineLog.h"
#include "Lua/LuaManager.h"

namespace LuaBindings
{
	void BindActor(sol::state& Lua)
	{
		// 람다 변수 정의 (위치, 회전, 스케일 등)
		auto GetLoc = &AActor::GetActorLocation;
		auto SetLoc = [](AActor* Actor, const FVector& NewLoc) {
			if (Actor && Actor->GetRootComponent())
				Actor->GetRootComponent()->SetWorldLocation(NewLoc, false);
		};
		auto AddLoc = [](AActor* Actor, const FVector& Delta) {
			if (Actor && Actor->GetRootComponent()) {
				FVector cur = Actor->GetActorLocation();
				Actor->GetRootComponent()->SetWorldLocation(cur + Delta, false);
			}
		};

		auto GetRot = &AActor::GetActorRotation;
		auto SetRot = [](AActor* Actor, const FRotator& NewRot) {
			if (Actor && Actor->GetRootComponent())
				Actor->GetRootComponent()->SetWorldRotation(NewRot, false);
		};
		auto AddRot = [](AActor* Actor, const FRotator& Delta) {
			if (Actor && Actor->GetRootComponent()) {
				FRotator cur = Actor->GetActorRotation();
				Actor->GetRootComponent()->SetWorldRotation(cur + Delta, false);
			}
		};

		auto GetFwd = [](AActor* Actor) -> FVector {
			return (Actor && Actor->GetRootComponent()) ? Actor->GetRootComponent()->GetTransform().GetForward() : ForwardVector;
		};
		auto GetRight = [](AActor* Actor) -> FVector {
			return (Actor && Actor->GetRootComponent()) ? Actor->GetRootComponent()->GetTransform().GetRight() : RightVector;
		};
		auto GetUp = [](AActor* Actor) -> FVector {
			return (Actor && Actor->GetRootComponent()) ? Actor->GetRootComponent()->GetTransform().GetUp() : UpVector;
		};

		auto GetScale = [](AActor* Actor) -> FVector {
			return (Actor && Actor->GetRootComponent()) ? Actor->GetRootComponent()->GetWorldScale3D() : FVector(1, 1, 1);
		};
		auto SetScale = [](AActor* Actor, const FVector& NewScale) {
			if (Actor && Actor->GetRootComponent())
				Actor->GetRootComponent()->SetWorldScale3D(NewScale);
		};

		auto SetVis = [](AActor* Actor, bool bVisible) {
			if (Actor && Actor->GetRootComponent())
				Actor->GetRootComponent()->SetVisibility(bVisible);
		};
		auto IsVis = [](AActor* Actor) -> bool {
			return (Actor && Actor->GetRootComponent()) ? Actor->GetRootComponent()->IsVisible() : false;
		};

		auto DoLineTrace = [](AActor* Actor, const FVector& Start, const FVector& Dir, float MaxDistance) -> sol::table {
			sol::table Result = FLuaManager::Get().GetState().create_table();
			Result["Hit"] = false;

			if (!Actor || !Actor->GetWorld() || !Actor->GetRootComponent())
				return Result;

			FRay Ray;
			Ray.Origin = Start;
			Ray.Direction = Dir;
			FHitResult Hit;
			if (Actor->GetWorld()->LineTraceSingle(Ray, Hit))
			{
				if (Hit.Distance <= MaxDistance)
				{
					Result["Hit"] = true;
					Result["Distance"] = Hit.Distance;
					Result["Point"] = Hit.ImpactPoint;
					Result["Actor"] = Hit.HitComponent ? Hit.HitComponent->GetOwner() : nullptr;
					Result["Normal"] = Hit.ImpactNormal;
				}
			}
			return Result;
		};

		auto FindCameraComp = [](AActor* Actor) -> UCameraComponent* {
			if (!Actor) return nullptr;
			for (UActorComponent* Comp : Actor->GetComponents()) {
				if (auto* Cam = Cast<UCameraComponent>(Comp))
					return Cam;
			}
			if (Actor->GetWorld() && Actor->GetWorld()->GetMainCamera()) {
				for (UActorComponent* Comp : Actor->GetWorld()->GetMainCamera()->GetComponents()) {
					if (auto* Cam = Cast<UCameraComponent>(Comp))
						return Cam;
				}
			}
			return nullptr;
		};

		auto FindCamera = [FindCameraComp](AActor* Actor) -> USceneComponent* {
			if (UCameraComponent* Cam = FindCameraComp(Actor))
				return Cam;
			if (Actor && Actor->GetWorld() && Actor->GetWorld()->GetMainCamera()) {
				return Actor->GetWorld()->GetMainCamera()->GetRootComponent();
			}
			return nullptr;
		};

		auto FindTextComp = [](AActor* Actor) -> UTextRenderComponent* {
			if (!Actor) return nullptr;
			for (UActorComponent* Comp : Actor->GetComponents()) {
				if (auto* TextComp = Cast<UTextRenderComponent>(Comp))
					return TextComp;
			}
			return nullptr;
		};

		// 카메라 위치
		auto GetCamLoc = [FindCamera](AActor* Actor) -> FVector {
			if (USceneComponent* Cam = FindCamera(Actor))
				return Cam->GetWorldLocation();
			return Actor ? (Actor->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f)) : FVector::ZeroVector;
		};

		// 카메라 시선 방향 벡터
		auto GetCamFwd = [FindCamera](AActor* Actor) -> FVector {
			if (USceneComponent* Cam = FindCamera(Actor))
				return Cam->GetTransform().GetForward();
			return (Actor && Actor->GetRootComponent()) ? Actor->GetRootComponent()->GetTransform().GetForward() : ForwardVector;
		};

		// 카메라 회전각
		auto GetCamRot = [FindCamera](AActor* Actor) -> FRotator {
			if (USceneComponent* Cam = FindCamera(Actor))
				return Cam->GetWorldRotation();
			return Actor ? Actor->GetActorRotation() : FRotator::Identitiy;
		};

		// Usertype 등록 (인덱스 등록 방식으로 확장성 확보)
		sol::usertype<AActor> ActorType = Lua.new_usertype<AActor>("Actor");

		ActorType["UUID"] = sol::property(&AActor::GetUUID);
		ActorType["GetName"] = &AActor::GetName;

		// 액터 위치
		ActorType["GetLocation"] = GetLoc;
		ActorType["GetActorLocation"] = GetLoc;
		ActorType["SetLocation"] = SetLoc;
		ActorType["SetActorLocation"] = SetLoc;
		ActorType["Location"] = sol::property(GetLoc, SetLoc);
		ActorType["AddLocation"] = AddLoc;

		// 액터 회전
		ActorType["GetRotation"] = GetRot;
		ActorType["GetActorRotation"] = GetRot;
		ActorType["SetRotation"] = SetRot;
		ActorType["SetActorRotation"] = SetRot;
		ActorType["Rotation"] = sol::property(GetRot, SetRot);
		ActorType["AddRotation"] = AddRot;

		// 액터 이동 속도
		auto GetVelocityFunc = [](AActor* Actor) -> FVector {
			return (Actor && Actor->GetRootComponent()) ? Actor->GetRootComponent()->ComponentVelocity : FVector::ZeroVector;
		};
		auto SetVelocityFunc = [](AActor* Actor, const FVector& NewVel) {
			if (Actor && Actor->GetRootComponent())
				Actor->GetRootComponent()->ComponentVelocity = NewVel;
		};

		ActorType["Velocity"] = sol::property(GetVelocityFunc, SetVelocityFunc);
		ActorType["GetVelocity"] = GetVelocityFunc;
		ActorType["SetVelocity"] = SetVelocityFunc;
		ActorType["AddImpulse"] = [](AActor* Actor, const FVector& Impulse) {
			if (Actor && Actor->GetRootComponent())
				Actor->GetRootComponent()->ComponentVelocity += Impulse;
		};

		// 방향 벡터
		ActorType["GetForwardVector"] = GetFwd;
		ActorType["GetActorForwardVector"] = GetFwd;
		ActorType["GetRightVector"] = GetRight;
		ActorType["GetActorRightVector"] = GetRight;
		ActorType["GetUpVector"] = GetUp;
		ActorType["GetActorUpVector"] = GetUp;

		// 스케일
		ActorType["GetScale"] = GetScale;
		ActorType["GetActorScale"] = GetScale;
		ActorType["GetActorScale3D"] = GetScale;
		ActorType["SetScale"] = SetScale;
		ActorType["SetActorScale"] = SetScale;
		ActorType["SetActorScale3D"] = SetScale;

		// 가시성 & 상태
		ActorType["SetVisibility"] = SetVis;
		ActorType["SetActorHiddenInGame"] = [SetVis](AActor* Actor, bool bHidden) { SetVis(Actor, !bHidden); };
		ActorType["IsVisible"] = IsVis;
		ActorType["IsActorHiddenInGame"] = [IsVis](AActor* Actor) -> bool { return !IsVis(Actor); };
		ActorType["IsValid"] = [](AActor* Actor) -> bool { return Actor != nullptr && Actor->GetWorld() != nullptr; };

		// 디버그 위치 출력
		ActorType["PrintLocation"] = [](AActor* Actor) {
			if (!Actor) return;
			FVector loc = Actor->GetActorLocation();
			HTR_LOG(Info, "[Actor: {}] Location: X={:.2f}, Y={:.2f}, Z={:.2f}",
				Actor->GetName(), loc.X, loc.Y, loc.Z);
		};

		// 레이캐스트
		ActorType["LineTrace"] = sol::overload(
			[DoLineTrace](AActor* Actor, float MaxDistance) -> sol::table {
				FVector Start = Actor ? Actor->GetActorLocation() : FVector::ZeroVector;
				FVector Dir = (Actor && Actor->GetRootComponent()) ? Actor->GetRootComponent()->GetTransform().GetForward() : ForwardVector;
				return DoLineTrace(Actor, Start, Dir, MaxDistance);
			},
			[DoLineTrace](AActor* Actor, const FVector& Start, const FVector& Dir, float MaxDistance) -> sol::table {
				return DoLineTrace(Actor, Start, Dir, MaxDistance);
			}
		);

		// 씬 액터 검색
		ActorType["FindActorByName"] = [](const AActor* Actor, const FString& Name) -> AActor* {
			if (!Actor || !Actor->GetWorld() || !Actor->GetWorld()->GetPersistentLevel()) return nullptr;

			UWorld* World = Actor->GetWorld();
			for (AActor* FoundActor : World->GetPersistentLevel()->GetActors())
			{
				if (FoundActor && FoundActor->GetName() == Name)
				{
					return FoundActor;
				}
			}
			return nullptr;
		};

		// 접두사로 여러 액터 찾기 (예: "Cube", "Portal")
		ActorType["FindActorsWithPrefix"] = [](const AActor* Actor, const FString& Prefix) -> sol::table {
			sol::table List = FLuaManager::Get().GetState().create_table();
			if (!Actor || !Actor->GetWorld() || !Actor->GetWorld()->GetPersistentLevel()) return List;

			UWorld* World = Actor->GetWorld();
			int Index = 1;
			for (AActor* FoundActor : World->GetPersistentLevel()->GetActors())
			{
				if (FoundActor && FoundActor->GetName().rfind(Prefix, 0) == 0)
				{
					List[Index++] = FoundActor;
				}
			}
			return List;
		};

		// 카메라 관련 위치, 바라보는 방향, 회전 (읽기)
		ActorType["GetCameraLocation"] = GetCamLoc;
		ActorType["GetCameraForwardVector"] = GetCamFwd;
		ActorType["GetCameraRotation"] = GetCamRot;

		// 카메라 조작 (쓰기: FPS 마우스 시점 및 FOV 조작)
		ActorType["SetCameraRotation"] = [FindCameraComp](AActor* Actor, const FRotator& NewRot) {
			if (UCameraComponent* Cam = FindCameraComp(Actor)) {
				Cam->SetWorldRotation(NewRot, false);
			}
		};
		ActorType["AddCameraRotation"] = [FindCameraComp](AActor* Actor, const FRotator& Delta) {
			if (UCameraComponent* Cam = FindCameraComp(Actor)) {
				Cam->SetWorldRotation(Cam->GetWorldRotation() + Delta, false);
			}
		};
		ActorType["SetCameraFOV"] = [FindCameraComp](AActor* Actor, float FOV) {
			if (UCameraComponent* Cam = FindCameraComp(Actor)) {
				Cam->SetFieldOfView(FOV);
			}
		};
		ActorType["GetCameraFOV"] = [FindCameraComp](AActor* Actor) -> float {
			if (UCameraComponent* Cam = FindCameraComp(Actor)) {
				return Cam->GetFieldOfView();
			}
			return 60.0f;
		};

		// 3D 텍스트 조작 (TextRenderComponent)
		ActorType["SetText"] = [FindTextComp](AActor* Actor, const FString& Text) {
			if (UTextRenderComponent* Comp = FindTextComp(Actor)) {
				Comp->SetText(Text);
			}
		};
		ActorType["GetText"] = [FindTextComp](AActor* Actor) -> FString {
			if (UTextRenderComponent* Comp = FindTextComp(Actor)) {
				return Comp->GetText();
			}
			return "";
		};
		ActorType["SetTextSize"] = [FindTextComp](AActor* Actor, float Size) {
			if (UTextRenderComponent* Comp = FindTextComp(Actor)) {
				Comp->SetTextSize(Size);
			}
		};

		// 액터 부착 & 분리 (오브젝트 집기/내려놓기)
		auto AttachFunc = [](AActor* Self, AActor* Parent) -> bool {
			if (!Self || !Parent || !Self->GetRootComponent() || !Parent->GetRootComponent()) return false;
			Self->GetRootComponent()->SetupAttachment(Parent->GetRootComponent(), EAttachmentRule::KeepRelative);
			return true;
		};
		auto DetachFunc = [](AActor* Self) -> bool {
			if (!Self || !Self->GetRootComponent()) return false;
			Self->GetRootComponent()->DetachFromParent(EAttachmentRule::KeepRelative);
			return true;
		};

		ActorType["AttachToActor"] = AttachFunc;
		ActorType["AttachTo"] = AttachFunc;
		ActorType["DetachFromActor"] = DetachFunc;
		ActorType["Detach"] = DetachFunc;

		// 액터 파괴
		ActorType["Destroy"] = &AActor::Destroy;

		// ==========================================
		// [UnLua 스타일 게임플레이 편의 함수]
		// ==========================================

		// 1. GetDistanceTo (두 액터 간 거리)
		ActorType["GetDistanceTo"] = [](AActor* Self, AActor* Other) -> float {
			if (!Self || !Other) return 0.0f;
			return FVector::Distance(Self->GetActorLocation(), Other->GetActorLocation());
		};

		// 2. TeleportTo / Teleport (순간이동: 위치와 회전을 원자적으로 갱신)
		auto TeleportFunc = sol::overload(
			[](AActor* Self, const FVector& NewLoc, const FRotator& NewRot) -> bool {
				if (!Self || !Self->GetRootComponent()) return false;
				Self->GetRootComponent()->SetWorldLocation(NewLoc, false);
				Self->GetRootComponent()->SetWorldRotation(NewRot, false);
				return true;
			},
			[](AActor* Self, const FVector& NewLoc) -> bool {
				if (!Self || !Self->GetRootComponent()) return false;
				Self->GetRootComponent()->SetWorldLocation(NewLoc, false);
				return true;
			}
		);

		ActorType["TeleportTo"] = TeleportFunc;
		ActorType["Teleport"] = TeleportFunc;
	}
}
