#include "EnginePCH.h"
#include "LuaBindings.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Actor/DoorActor.h"
#include "GameFramework/Actor/ButtonActorBase.h"
#include "GameFramework/Actor/WeightedButtonActor.h"
#include "GameFramework/Actor/StandButtonActor.h"
#include "GameFramework/Actor/UIManagerActor.h"
#include "GameFramework/Actor/HUDCanvasActor.h"
#include "GameFramework/Actor/TitleCanvasActor.h"
#include "GameFramework/Actor/PauseCanvasActor.h"
#include "GameFramework/Actor/EndingCanvasActor.h"
#include "GameFramework/PortalPawn.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/Level.h"
#include "Core/EngineLog.h"
#include "Lua/LuaManager.h"
#include "UObject/UObjectHash.h"
#include "Serialization/JsonArchive.h"

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
		sol::usertype<UGameInstance> GameInstanceType = Lua.new_usertype<UGameInstance>("GameInstance");
		GameInstanceType["GetWorld"] = &UGameInstance::GetWorld;

		sol::usertype<AActor> ActorType = Lua.new_usertype<AActor>("Actor");

		ActorType["UUID"] = sol::property(&AActor::GetUUID);
		ActorType["GetName"] = &AActor::GetName;
		ActorType["GetGameInstance"] = &AActor::GetGameInstance;

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

		// 3. 충돌 활성화 / 비활성화
		ActorType["SetActorEnableCollision"] = &AActor::SetActorEnableCollision;
		ActorType["GetActorEnableCollision"] = &AActor::GetActorEnableCollision;
		ActorType["EnableCollision"] = sol::property(&AActor::GetActorEnableCollision, &AActor::SetActorEnableCollision);

		// 4. 동적 컴포넌트 추가
		ActorType["AddComponent"] = [](AActor* Self, const FString& ComponentClassName) -> UActorComponent* {
			if (!Self) return nullptr;
			UClass* CompClass = FindClass(ComponentClassName);
			if (!CompClass) CompClass = FindClass("U" + ComponentClassName);
			if (!CompClass || !CompClass->IsChildOf(UActorComponent::StaticClass()))
			{
				HTR_LOG(Warning, "[Lua AddComponent] Component class '{}' not found", ComponentClassName);
				return nullptr;
			}
			return Self->AddComponent(CompClass, NAME_None);
		};

		// 5. 액터 스폰 (SpawnActor)
		auto SpawnActorHelper = [](UWorld* World, const FString& ClassName, const FVector& Location, const FRotator& Rotation) -> AActor* {
			if (!World)
			{
				World = FLuaManager::Get().GetActiveWorld();
			}
			if (!World)
			{
				HTR_LOG(Error, "[Lua SpawnActor] No active world found");
				return nullptr;
			}

			UClass* ActorClass = FindClass(ClassName);
			if (!ActorClass) ActorClass = FindClass("A" + ClassName);
			if (!ActorClass || !ActorClass->IsChildOf(AActor::StaticClass()))
			{
				HTR_LOG(Warning, "[Lua SpawnActor] Actor class '{}' not found", ClassName);
				return nullptr;
			}

			FTransform SpawnTransform(Rotation, Location, FVector::OneVector);
			return World->SpawnActor(ActorClass, NAME_None, &SpawnTransform);
		};

		// 멤버 SpawnActor (this:SpawnActor("ClassName", loc, rot))
		ActorType["SpawnActor"] = sol::overload(
			[SpawnActorHelper](AActor* Self, const FString& ClassName, const FVector& Loc, const FRotator& Rot) -> AActor* {
				UWorld* World = Self ? Self->GetWorld() : nullptr;
				return SpawnActorHelper(World, ClassName, Loc, Rot);
			},
			[SpawnActorHelper](AActor* Self, const FString& ClassName, const FVector& Loc) -> AActor* {
				UWorld* World = Self ? Self->GetWorld() : nullptr;
				return SpawnActorHelper(World, ClassName, Loc, FRotator());
			},
			[SpawnActorHelper](AActor* Self, const FString& ClassName) -> AActor* {
				UWorld* World = Self ? Self->GetWorld() : nullptr;
				FVector Loc = Self ? Self->GetActorLocation() : FVector::ZeroVector;
				FRotator Rot = Self ? Self->GetActorRotation() : FRotator();
				return SpawnActorHelper(World, ClassName, Loc, Rot);
			}
		);

		// 전역 SpawnActor (SpawnActor("ClassName", loc, rot))
		Lua["SpawnActor"] = sol::overload(
			[SpawnActorHelper](const FString& ClassName, const FVector& Loc, const FRotator& Rot) -> AActor* {
				return SpawnActorHelper(nullptr, ClassName, Loc, Rot);
			},
			[SpawnActorHelper](const FString& ClassName, const FVector& Loc) -> AActor* {
				return SpawnActorHelper(nullptr, ClassName, Loc, FRotator());
			},
			[SpawnActorHelper](const FString& ClassName) -> AActor* {
				return SpawnActorHelper(nullptr, ClassName, FVector::ZeroVector, FRotator());
			}
		);

		// ==========================================
		// [도어 액터 (DoorActor) 제어 바인딩]
		// ==========================================
		ActorType["OpenDoor"] = [](AActor* Actor) {
			if (ADoorActor* Door = Cast<ADoorActor>(Actor))
			{
				Door->OpenDoor();
			}
		};

		ActorType["CloseDoor"] = [](AActor* Actor) {
			if (ADoorActor* Door = Cast<ADoorActor>(Actor))
			{
				Door->CloseDoor();
			}
		};

		ActorType["ToggleDoor"] = [](AActor* Actor) {
			if (ADoorActor* Door = Cast<ADoorActor>(Actor))
			{
				Door->ToggleDoor();
			}
		};

		ActorType["SetDoorOpen"] = [](AActor* Actor, bool bOpen) {
			if (ADoorActor* Door = Cast<ADoorActor>(Actor))
			{
				Door->SetOpen(bOpen);
			}
		};

		ActorType["IsDoorOpen"] = [](AActor* Actor) -> bool {
			if (ADoorActor* Door = Cast<ADoorActor>(Actor))
			{
				return Door->IsOpen();
			}
			return false;
		};

		ActorType["SetDoorAutoOpen"] = [](AActor* Actor, bool bAuto) {
			if (ADoorActor* Door = Cast<ADoorActor>(Actor))
			{
				Door->SetAutoOpenOnOverlap(bAuto);
			}
		};

		ActorType["IsDoorAutoOpen"] = [](AActor* Actor) -> bool {
			if (ADoorActor* Door = Cast<ADoorActor>(Actor))
			{
				return Door->IsAutoOpenOnOverlap();
			}
			return false;
		};

		// ==========================================
		// [버튼 액터 (ButtonActorBase) 제어 및 콜백 바인딩]
		// ==========================================
		auto BindOnPressedFunc = [](AActor* Actor, sol::function Callback) {
			if (AButtonActorBase* Button = Cast<AButtonActorBase>(Actor))
			{
				Button->OnButtonPressed.Add([Callback](bool bPressed) {
					if (Callback.valid())
					{
						auto Result = Callback(bPressed);
						if (!Result.valid())
						{
							sol::error Err = Result;
							HTR_LOG(Error, "[Lua BindOnPressed Callback Error]: {}", Err.what());
						}
					}
				});
			}
		};

		ActorType["BindOnPressed"] = BindOnPressedFunc;
		ActorType["OnPressed"] = BindOnPressedFunc;

		ActorType["ClearButtonPressedCallbacks"] = [](AActor* Actor) {
			if (AButtonActorBase* Button = Cast<AButtonActorBase>(Actor))
			{
				Button->OnButtonPressed.Clear();
			}
		};

		ActorType["IsButtonPressed"] = [](AActor* Actor) -> bool {
			if (AButtonActorBase* Button = Cast<AButtonActorBase>(Actor))
			{
				return Button->IsPressed();
			}
			return false;
		};

		ActorType["PressButton"] = [](AActor* Actor) {
			if (AButtonActorBase* Button = Cast<AButtonActorBase>(Actor))
			{
				Button->PressButton();
			}
		};

		ActorType["ReleaseButton"] = [](AActor* Actor) {
			if (AButtonActorBase* Button = Cast<AButtonActorBase>(Actor))
			{
				Button->ReleaseButton();
			}
		};

		ActorType["InteractButton"] = [](AActor* Actor, AActor* Interactor) {
			if (AStandButtonActor* StandBtn = Cast<AStandButtonActor>(Actor))
			{
				StandBtn->Interact(Interactor);
			}
		};

		ActorType["IsBluePortalPlaced"] = [](AActor* Actor) -> bool {
			if (APortalPawn* Pawn = Cast<APortalPawn>(Actor))
			{
				return Pawn->IsBluePortalPlaced();
			}
			return false;
		};
		ActorType["IsOrangePortalPlaced"] = [](AActor* Actor) -> bool {
			if (APortalPawn* Pawn = Cast<APortalPawn>(Actor))
			{
				return Pawn->IsOrangePortalPlaced();
			}
			return false;
		};
		ActorType["ResetPortals"] = [](AActor* Actor) {
			if (APortalPawn* Pawn = Cast<APortalPawn>(Actor))
			{
				Pawn->ResetPortals();
			}
		};

		// ==========================================
		// [개별 유저타입 (Usertype) 등록]
		// ==========================================
		sol::usertype<ADoorActor> DoorType = Lua.new_usertype<ADoorActor>("DoorActor",
			sol::base_classes, sol::bases<AActor>());
		DoorType["OpenDoor"] = &ADoorActor::OpenDoor;
		DoorType["CloseDoor"] = &ADoorActor::CloseDoor;
		DoorType["ToggleDoor"] = &ADoorActor::ToggleDoor;
		DoorType["IsOpen"] = &ADoorActor::IsOpen;
		DoorType["SetOpen"] = &ADoorActor::SetOpen;
		DoorType["SetAutoOpen"] = &ADoorActor::SetAutoOpenOnOverlap;
		DoorType["IsAutoOpen"] = &ADoorActor::IsAutoOpenOnOverlap;

		sol::usertype<AButtonActorBase> ButtonBaseType = Lua.new_usertype<AButtonActorBase>("ButtonActorBase",
			sol::base_classes, sol::bases<AActor>());
		ButtonBaseType["IsPressed"] = &AButtonActorBase::IsPressed;
		ButtonBaseType["PressButton"] = &AButtonActorBase::PressButton;
		ButtonBaseType["ReleaseButton"] = &AButtonActorBase::ReleaseButton;
		ButtonBaseType["BindOnPressed"] = BindOnPressedFunc;
		ButtonBaseType["OnPressed"] = BindOnPressedFunc;

		sol::usertype<AWeightedButtonActor> WeightedButtonType = Lua.new_usertype<AWeightedButtonActor>("WeightedButtonActor",
			sol::base_classes, sol::bases<AButtonActorBase, AActor>());

		sol::usertype<AStandButtonActor> StandButtonType = Lua.new_usertype<AStandButtonActor>("StandButtonActor",
			sol::base_classes, sol::bases<AButtonActorBase, AActor>());
		StandButtonType["Interact"] = &AStandButtonActor::Interact;

		// ==========================================
		// [UI ?≫꽣 諛?留ㅻ땲? 諛붿씤??
		// ==========================================
		sol::usertype<AHUDCanvasActor> HUDType = Lua.new_usertype<AHUDCanvasActor>("HUDCanvasActor",
			sol::base_classes, sol::bases<AActor>());
		HUDType["StartTimer"] = &AHUDCanvasActor::StartTimer;
		HUDType["StopTimer"] = &AHUDCanvasActor::StopTimer;
		HUDType["ResetTimer"] = &AHUDCanvasActor::ResetTimer;
		HUDType["GetElapsedTime"] = &AHUDCanvasActor::GetElapsedTime;
		HUDType["GetFormattedTime"] = &AHUDCanvasActor::GetFormattedTime;
		HUDType["SetTimerText"] = &AHUDCanvasActor::SetTimerText;
		HUDType["SetTimerVisibility"] = &AHUDCanvasActor::SetTimerVisibility;
		HUDType["ShowSubtitle"] = &AHUDCanvasActor::ShowSubtitle;
		HUDType["HideSubtitle"] = &AHUDCanvasActor::HideSubtitle;
		HUDType["ShowInteractPrompt"] = &AHUDCanvasActor::ShowInteractPrompt;
		HUDType["HideInteractPrompt"] = &AHUDCanvasActor::HideInteractPrompt;
		HUDType["StartFade"] = &AHUDCanvasActor::StartFade;
		HUDType["SetCrosshairState"] = [](AHUDCanvasActor* HUD, const FString& State) {
			if (HUD) HUD->SetCrosshairStateByName(State);
		};
		HUDType["GetCrosshairState"] = [](AHUDCanvasActor* HUD) -> FString {
			return HUD ? HUD->GetCrosshairStateName() : FString("unfilled");
		};
		HUDType["SetCrosshairSize"] = [](AHUDCanvasActor* HUD, float W, float H) {
			if (HUD) HUD->SetCrosshairSize(W, H);
		};
		HUDType["SetCrosshairVisibility"] = [](AHUDCanvasActor* HUD, bool bVisible) {
			if (HUD) HUD->SetCrosshairVisibility(bVisible);
		};

		sol::usertype<AEndingCanvasActor> EndingType = Lua.new_usertype<AEndingCanvasActor>("EndingCanvasActor",
			sol::base_classes, sol::bases<AActor>());
		EndingType["SetClearTime"] = &AEndingCanvasActor::SetClearTime;
		EndingType["SetClearTimeText"] = &AEndingCanvasActor::SetClearTimeText;
		EndingType["GetClearTime"] = &AEndingCanvasActor::GetClearTime;

		sol::usertype<AUIManagerActor> UIManagerType = Lua.new_usertype<AUIManagerActor>("UIManagerActor",
			sol::base_classes, sol::bases<AActor>());
		UIManagerType["GetElapsedTime"] = &AUIManagerActor::GetElapsedTime;
		UIManagerType["GetFormattedTime"] = &AUIManagerActor::GetFormattedTime;
		UIManagerType["ResetTimer"] = &AUIManagerActor::ResetTimer;
		UIManagerType["StartTimer"] = &AUIManagerActor::StartTimer;
		UIManagerType["StopTimer"] = &AUIManagerActor::StopTimer;
		UIManagerType["GetHUDCanvas"] = &AUIManagerActor::GetHUDCanvas;
		UIManagerType["GetEndingCanvas"] = &AUIManagerActor::GetEndingCanvas;
		UIManagerType["SetCrosshairState"] = &AUIManagerActor::SetCrosshairState;
		UIManagerType["GetCrosshairState"] = &AUIManagerActor::GetCrosshairState;
		UIManagerType["SetCrosshairSize"] = &AUIManagerActor::SetCrosshairSize;
		UIManagerType["SetCrosshairVisibility"] = &AUIManagerActor::SetCrosshairVisibility;
		UIManagerType["SetUIMode"] = [](AUIManagerActor* Mgr, const FString& ModeStr) {
			if (!Mgr) return;
			if (ModeStr == "InGame") Mgr->SetUIMode(EUIMode::InGame);
			else if (ModeStr == "Title") Mgr->SetUIMode(EUIMode::Title);
			else if (ModeStr == "Pause") Mgr->SetUIMode(EUIMode::Pause);
			else if (ModeStr == "Ending") Mgr->SetUIMode(EUIMode::Ending);
		};

		// ?꾩뿭 UIManager ?몄쓽 ?뚯씠釉?(?ㅽ겕由쏀듃 ?대뵒?쒕뱺 UIManager.GetFormattedTime() ?깆쑝濡???以??몄텧 媛??
		auto UIManagerTable = Lua.create_named_table("UIManager");
		UIManagerTable["Get"] = &AUIManagerActor::Get;
		UIManagerTable["GetElapsedTime"] = []() -> float {
			return AUIManagerActor::Get() ? AUIManagerActor::Get()->GetElapsedTime() : 0.0f;
		};
		UIManagerTable["GetFormattedTime"] = []() -> FString {
			return AUIManagerActor::Get() ? AUIManagerActor::Get()->GetFormattedTime() : FString("00:00.00");
		};
		UIManagerTable["StartTimer"] = []() {
			if (AUIManagerActor::Get()) AUIManagerActor::Get()->StartTimer();
		};
		UIManagerTable["StopTimer"] = []() {
			if (AUIManagerActor::Get()) AUIManagerActor::Get()->StopTimer();
		};
		UIManagerTable["ResetTimer"] = []() {
			if (AUIManagerActor::Get()) AUIManagerActor::Get()->ResetTimer();
		};
		UIManagerTable["SetUIMode"] = [](const FString& ModeStr) {
			if (!AUIManagerActor::Get()) return;
			if (ModeStr == "InGame") AUIManagerActor::Get()->SetUIMode(EUIMode::InGame);
			else if (ModeStr == "Title") AUIManagerActor::Get()->SetUIMode(EUIMode::Title);
			else if (ModeStr == "Pause") AUIManagerActor::Get()->SetUIMode(EUIMode::Pause);
			else if (ModeStr == "Ending") AUIManagerActor::Get()->SetUIMode(EUIMode::Ending);
		};
		UIManagerTable["GetHUD"] = []() -> AHUDCanvasActor* {
			return AUIManagerActor::Get() ? AUIManagerActor::Get()->GetHUDCanvas() : nullptr;
		};
		UIManagerTable["GetEnding"] = []() -> AEndingCanvasActor* {
			return AUIManagerActor::Get() ? AUIManagerActor::Get()->GetEndingCanvas() : nullptr;
		};
		UIManagerTable["SetCrosshairState"] = [](const FString& State) {
			if (AUIManagerActor::Get()) AUIManagerActor::Get()->SetCrosshairState(State);
		};
		UIManagerTable["GetCrosshairState"] = []() -> FString {
			return AUIManagerActor::Get() ? AUIManagerActor::Get()->GetCrosshairState() : FString("unfilled");
		};
		UIManagerTable["SetCrosshairSize"] = [](float W, float H) {
			if (AUIManagerActor::Get()) AUIManagerActor::Get()->SetCrosshairSize(W, H);
		};
		UIManagerTable["SetCrosshairVisibility"] = [](bool bVisible) {
			if (AUIManagerActor::Get()) AUIManagerActor::Get()->SetCrosshairVisibility(bVisible);
		};
		UIManagerTable["GetUIMode"] = []() -> FString {
			if (!AUIManagerActor::Get()) return FString("None");
			switch (AUIManagerActor::Get()->GetCurrentUIMode())
			{
			case EUIMode::Title: return FString("Title");
			case EUIMode::InGame: return FString("InGame");
			case EUIMode::Pause: return FString("Pause");
			case EUIMode::Ending: return FString("Ending");
			default: return FString("None");
			}
		};

		// PortalPawn 유저타입 등록
		sol::usertype<APortalPawn> PortalPawnType = Lua.new_usertype<APortalPawn>("PortalPawn",
			sol::base_classes, sol::bases<AActor>());
		PortalPawnType["IsBluePortalPlaced"] = &APortalPawn::IsBluePortalPlaced;
		PortalPawnType["IsOrangePortalPlaced"] = &APortalPawn::IsOrangePortalPlaced;
		PortalPawnType["ResetPortals"] = &APortalPawn::ResetPortals;

		// 6. Scene 모듈 (씬 로드 및 리로드)
		auto SceneTable = Lua.create_named_table("Scene");

		SceneTable["Load"] = [](const FString& ScenePath) -> bool {
			UWorld* World = FLuaManager::Get().GetActiveWorld();
			if (!World)
			{
				HTR_LOG(Error, "[Lua Scene.Load] No active world found");
				return false;
			}
			ClearTimers();
			bool bSuccess = FJsonArchive::LoadWorld(World, ScenePath);
			if (bSuccess && World->HasBegunPlay())
			{
				World->BeginPlay();
			}
			return bSuccess;
		};

		SceneTable["Reload"] = []() -> bool {
			UWorld* World = FLuaManager::Get().GetActiveWorld();
			if (!World)
			{
				HTR_LOG(Error, "[Lua Scene.Reload] No active world found");
				return false;
			}
			const FString& LastPath = FJsonArchive::GetLastLoadedScenePath();
			if (LastPath.empty())
			{
				HTR_LOG(Warning, "[Lua Scene.Reload] No previous scene path recorded");
				return false;
			}
			ClearTimers();
			bool bSuccess = FJsonArchive::LoadWorld(World, LastPath);
			if (bSuccess && World->HasBegunPlay())
			{
				World->BeginPlay();
			}
			return bSuccess;
		};

		SceneTable["GetCurrentPath"] = []() -> FString {
			return FJsonArchive::GetLastLoadedScenePath();
		};
	}
}
