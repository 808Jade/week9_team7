#include "EnginePCH.h"
#include "LuaBindings.h"

#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/AudioComponent.h"
#include "GameFramework/Actor.h"
#include "UObject/Class.h"
#include "UObject/Property.h"
#include "Math/Vector.h"
#include "Math/Rotator.h"
#include "Math/Color.h"
#include "Core/EngineLog.h"
#include "Lua/LuaManager.h"

namespace LuaBindings
{
	namespace
	{
		// 리플렉션 메타데이터를 검색하여 값을 읽어오는 헬퍼
		sol::object GetPropertyInternal(sol::this_state s, UActorComponent* Comp, const std::string& Key)
		{
			sol::state_view Lua(s);
			if (!Comp || !Comp->GetClass()) return sol::make_object(Lua, sol::nil);

			for (UClass* C = Comp->GetClass(); C != nullptr; C = C->Super)
			{
				for (const FProperty& Prop : C->GetProperties())
				{
					if (Prop.Name == Key.c_str())
					{
						void* ValuePtr = reinterpret_cast<char*>(Comp) + Prop.Offset;
						switch (Prop.Type)
						{
						case EPropertyType::Float:
							return sol::make_object(Lua, *reinterpret_cast<float*>(ValuePtr));
						case EPropertyType::Int:
							return sol::make_object(Lua, *reinterpret_cast<int*>(ValuePtr));
						case EPropertyType::Bool:
							return sol::make_object(Lua, *reinterpret_cast<bool*>(ValuePtr));
						case EPropertyType::String:
							return sol::make_object(Lua, *reinterpret_cast<FString*>(ValuePtr));
						case EPropertyType::Vector:
							return sol::make_object(Lua, *reinterpret_cast<FVector*>(ValuePtr));
						case EPropertyType::Rotator:
							return sol::make_object(Lua, *reinterpret_cast<FRotator*>(ValuePtr));
						case EPropertyType::Color:
							return sol::make_object(Lua, *reinterpret_cast<FColor*>(ValuePtr));
						case EPropertyType::LinearColor:
						{
							FLinearColor* LC = reinterpret_cast<FLinearColor*>(ValuePtr);
							return sol::make_object(Lua, FColor(
								static_cast<uint8>(FMath::Clamp(LC->R * 255.0f, 0.0f, 255.0f)),
								static_cast<uint8>(FMath::Clamp(LC->G * 255.0f, 0.0f, 255.0f)),
								static_cast<uint8>(FMath::Clamp(LC->B * 255.0f, 0.0f, 255.0f)),
								static_cast<uint8>(FMath::Clamp(LC->A * 255.0f, 0.0f, 255.0f))
							));
						}
						case EPropertyType::Object:
						{
							UObject** ObjPtr = reinterpret_cast<UObject**>(ValuePtr);
							return sol::make_object(Lua, *ObjPtr);
						}
						default:
							break;
						}
					}
				}
			}

			if (const FFunction* Func = Comp->GetClass()->FindFunction(Key.c_str()))
			{
				return sol::make_object(Lua, [Func, Comp](sol::variadic_args Args) {
					Func->Invoker(Comp, nullptr);
				});
			}
			return sol::make_object(Lua, sol::nil);
		}

		// 리플렉션 메타데이터를 검색하여 값을 쓰는 헬퍼
		bool SetPropertyInternal(UActorComponent* Comp, const std::string& Key, sol::stack_object Value)
		{
			if (!Comp || !Comp->GetClass()) return false;

			for (UClass* C = Comp->GetClass(); C != nullptr; C = C->Super)
			{
				for (const FProperty& Prop : C->GetProperties())
				{
					if (Prop.Name == Key.c_str())
					{
						void* ValuePtr = reinterpret_cast<char*>(Comp) + Prop.Offset;
						switch (Prop.Type)
						{
						case EPropertyType::Float:
							*reinterpret_cast<float*>(ValuePtr) = Value.as<float>();
							break;
						case EPropertyType::Int:
							*reinterpret_cast<int*>(ValuePtr) = Value.as<int>();
							break;
						case EPropertyType::Bool:
							*reinterpret_cast<bool*>(ValuePtr) = Value.as<bool>();
							break;
						case EPropertyType::String:
							*reinterpret_cast<FString*>(ValuePtr) = Value.as<FString>();
							break;
						case EPropertyType::Vector:
							*reinterpret_cast<FVector*>(ValuePtr) = Value.as<FVector>();
							break;
						case EPropertyType::Rotator:
							*reinterpret_cast<FRotator*>(ValuePtr) = Value.as<FRotator>();
							break;
						case EPropertyType::Color:
							*reinterpret_cast<FColor*>(ValuePtr) = Value.as<FColor>();
							break;
						default:
							return false;
						}

						// 컴포넌트 렌더 상태 및 내부 캐시 갱신 알림
						Comp->PostEditChangeProperty(Prop);
						return true;
					}
				}
			}
			return false;
		}
	}

	void BindComponent(sol::state& Lua)
	{
		// UActorComponent 기본 유저타입 등록
		sol::usertype<UActorComponent> CompType = Lua.new_usertype<UActorComponent>("ActorComponent");

		CompType["GetName"] = &UActorComponent::GetName;
		CompType["GetOwner"] = &UActorComponent::GetOwner;
		CompType["IsActive"] = &UActorComponent::IsActive;
		CompType["SetActive"] = &UActorComponent::SetActive;
		CompType["IsVisible"] = &UActorComponent::IsVisible;
		CompType["SetVisibility"] = &UActorComponent::SetVisibility;
		CompType["GetClassName"] = [](UActorComponent* Comp) -> FString {
			return (Comp && Comp->GetClass()) ? Comp->GetClass()->Name : "";
		};

		// USceneComponent Transform 제어 바인딩
		CompType["SetRelativeLocation"] = [](UActorComponent* Comp, const FVector& Loc) {
			if (USceneComponent* SceneComp = Cast<USceneComponent>(Comp))
			{
				SceneComp->SetRelativeLocation(Loc);
			}
		};
		CompType["SetRelativeRotation"] = [](UActorComponent* Comp, const FRotator& Rot) {
			if (USceneComponent* SceneComp = Cast<USceneComponent>(Comp))
			{
				SceneComp->SetRelativeRotation(Rot);
			}
		};
		CompType["SetRelativeScale3D"] = [](UActorComponent* Comp, const FVector& Scale) {
			if (USceneComponent* SceneComp = Cast<USceneComponent>(Comp))
			{
				SceneComp->SetRelativeScale3D(Scale);
			}
		};
		CompType["GetRelativeLocation"] = [](UActorComponent* Comp) -> FVector {
			if (USceneComponent* SceneComp = Cast<USceneComponent>(Comp))
			{
				return SceneComp->GetRelativeLocation();
			}
			return FVector::ZeroVector;
		};
		CompType["GetRelativeRotation"] = [](UActorComponent* Comp) -> FRotator {
			if (USceneComponent* SceneComp = Cast<USceneComponent>(Comp))
			{
				return SceneComp->GetRelativeRotation();
			}
			return FRotator::ZeroRotator;
		};
		CompType["GetRelativeScale3D"] = [](UActorComponent* Comp) -> FVector {
			if (USceneComponent* SceneComp = Cast<USceneComponent>(Comp))
			{
				return SceneComp->GetRelativeScale3D();
			}
			return FVector(1.0f, 1.0f, 1.0f);
		};

		// 명시적 프로퍼티 함수
		CompType["GetProperty"] = [](sol::this_state s, UActorComponent* Comp, const std::string& Key) -> sol::object {
			return GetPropertyInternal(s, Comp, Key);
		};
		CompType["SetProperty"] = [](UActorComponent* Comp, const std::string& Key, sol::stack_object Value) -> bool {
			return SetPropertyInternal(Comp, Key, Value);
		};

		// 동적 인덱싱
		CompType[sol::meta_function::index] = [](sol::this_state s, UActorComponent* Comp, const std::string& Key) -> sol::object {
			return GetPropertyInternal(s, Comp, Key);
		};
		CompType[sol::meta_function::new_index] = [](UActorComponent* Comp, const std::string& Key, sol::stack_object Value) {
			SetPropertyInternal(Comp, Key, Value);
		};

		// AActor에 GetComponent / GetComponents 바인딩 추가
		sol::usertype<AActor> ActorType = Lua["Actor"];

		ActorType["GetComponent"] = [](AActor* Actor, const FString& CompName) -> UActorComponent* {
			if (!Actor) return nullptr;
			for (UActorComponent* Comp : Actor->GetComponents())
			{
				if (!Comp || !Comp->GetClass()) continue;
				if (Comp->GetClass()->Name == CompName || Comp->GetName() == CompName || ("U" + Comp->GetClass()->Name) == CompName)
				{
					return Comp;
				}
			}
			return nullptr;
		};

		ActorType["GetComponents"] = [](AActor* Actor) -> sol::table {
			sol::table List = FLuaManager::Get().GetState().create_table();
			if (!Actor) return List;
			int Index = 1;
			for (UActorComponent* Comp : Actor->GetComponents())
			{
				if (Comp) List[Index++] = Comp;
			}
			return List;
		};

		// AudioComponent 바인딩
		CompType["Play"] = [](UActorComponent* Comp) {
			if (UAudioComponent* Audio = Cast<UAudioComponent>(Comp)) Audio->Play();
		};
		CompType["Stop"] = [](UActorComponent* Comp) {
			if (UAudioComponent* Audio = Cast<UAudioComponent>(Comp)) Audio->Stop();
		};

		sol::usertype<UAudioComponent> AudioCompType = Lua.new_usertype<UAudioComponent>("AudioComponent",
			sol::base_classes, sol::bases<UActorComponent, USceneComponent>());
		AudioCompType["Play"] = &UAudioComponent::Play;
		AudioCompType["Stop"] = &UAudioComponent::Stop;
		AudioCompType["IsPlaying"] = &UAudioComponent::IsPlaying;
		AudioCompType["SetPaused"] = &UAudioComponent::SetPaused;
		AudioCompType["SetVolume"] = &UAudioComponent::SetVolume;
		AudioCompType["SetPitch"] = &UAudioComponent::SetPitch;
		AudioCompType["SetSoundPath"] = &UAudioComponent::SetSoundPath;
		AudioCompType["GetSoundPath"] = &UAudioComponent::GetSoundPath;
		AudioCompType["SetIs3D"] = &UAudioComponent::SetIs3D;
		AudioCompType["GetIs3D"] = &UAudioComponent::GetIs3D;
	}
}