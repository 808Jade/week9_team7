#include "EnginePCH.h"
#include "LuaManager.h"

#include "Math/Vector.h"
#include "Math/Rotator.h" 

#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Engine/Level.h"

#include "Input/InputSystem.h"
#include "Core/EngineLog.h"


namespace
{
	// 문자열을 EKeyCode로 변환하는 맵
	static const std::unordered_map<FString, EKeyCode> StringToKeyMap = {
		// 알파벳 (A ~ Z)
		{ "A", EKeyCode::A },{ "B", EKeyCode::B },{ "C", EKeyCode::C },{ "D", EKeyCode::D },
		{ "E", EKeyCode::E },{ "F", EKeyCode::F },{ "G", EKeyCode::G },{ "H", EKeyCode::H },
		{ "I", EKeyCode::I },{ "J", EKeyCode::J },{ "K", EKeyCode::K },{ "L", EKeyCode::L },
		{ "M", EKeyCode::M },{ "N", EKeyCode::N },{ "O", EKeyCode::O },{ "P", EKeyCode::P },
		{ "Q", EKeyCode::Q },{ "R", EKeyCode::R },{ "S", EKeyCode::S },{ "T", EKeyCode::T },
		{ "U", EKeyCode::U },{ "V", EKeyCode::V },{ "W", EKeyCode::W },{ "X", EKeyCode::X },
		{ "Y", EKeyCode::Y },{ "Z", EKeyCode::Z },

		// 숫자 키 (0 ~ 9)
		{ "0", EKeyCode::Num0 },{ "1", EKeyCode::Num1 },{ "2", EKeyCode::Num2 },{ "3", EKeyCode::Num3 },
		{ "4", EKeyCode::Num4 },{ "5", EKeyCode::Num5 },{ "6", EKeyCode::Num6 },{ "7", EKeyCode::Num7 },
		{ "8", EKeyCode::Num8 },{ "9", EKeyCode::Num9 },
		{ "Num0", EKeyCode::Num0 },{ "Num1", EKeyCode::Num1 }, // "Num0"도 대응

		// 기능 키 (F1 ~ F12)
		{ "F1", EKeyCode::F1 },{ "F2", EKeyCode::F2 },{ "F3", EKeyCode::F3 },{ "F4", EKeyCode::F4 },
		{ "F5", EKeyCode::F5 },{ "F6", EKeyCode::F6 },{ "F7", EKeyCode::F7 },{ "F8", EKeyCode::F8 },
		{ "F9", EKeyCode::F9 },{ "F10", EKeyCode::F10 },{ "F11", EKeyCode::F11 },{ "F12", EKeyCode::F12 },

		// 특수 / 제어 키
		{ "Space", EKeyCode::Space },{ "Enter", EKeyCode::Enter },{ "Escape", EKeyCode::Escape },
		{ "Tab", EKeyCode::Tab },{ "Backspace", EKeyCode::Backspace },{ "CapsLock", EKeyCode::CapsLock },
		{ "Shift", EKeyCode::Shift },{ "LShift", EKeyCode::LShift },{ "RShift", EKeyCode::RShift },
		{ "Control", EKeyCode::Control },{ "Ctrl", EKeyCode::Control },{ "LControl", EKeyCode::LControl },{ "RControl", EKeyCode::RControl },
		{ "Alt", EKeyCode::Alt },{ "LAlt", EKeyCode::LAlt },{ "RAlt", EKeyCode::RAlt },

		// 방향키
		{ "Left", EKeyCode::Left },{ "Right", EKeyCode::Right },{ "Up", EKeyCode::Up },{ "Down", EKeyCode::Down },

		// 넘패드 및 기호
		{ "NumPad0", EKeyCode::NumPad0 },{ "NumPad1", EKeyCode::NumPad1 },{ "NumPad2", EKeyCode::NumPad2 },
		{ "NumPad3", EKeyCode::NumPad3 },{ "NumPad4", EKeyCode::NumPad4 },{ "NumPad5", EKeyCode::NumPad5 },
		{ "NumPad6", EKeyCode::NumPad6 },{ "NumPad7", EKeyCode::NumPad7 },{ "NumPad8", EKeyCode::NumPad8 },
		{ "NumPad9", EKeyCode::NumPad9 },
		{ "Semicolon", EKeyCode::Semicolon },{ "Equals", EKeyCode::Equals },{ "Comma", EKeyCode::Comma },
		{ "Minus", EKeyCode::Minus },{ "Period", EKeyCode::Period },{ "Slash", EKeyCode::Slash },
		{ "Tilde", EKeyCode::Tilde }
	};
}

bool FLuaManager::Init()
{
	if (bInitialized) return true;

	// 루아 VM 초기화 및 표준 라이브러리 로드
	Lua.open_libraries(
		sol::lib::base,
		sol::lib::math,
		sol::lib::string,
		sol::lib::table,
		sol::lib::coroutine);

	Lua.set_exception_handler([](lua_State* L, sol::optional<const std::exception&> MaybeException, sol::string_view Description) {
		HTR_LOG(Error, "[Lua Exception] {}", Description);
		return sol::stack::push(L, Description);
		});
	lua_atpanic(Lua.lua_state(), [](lua_State* L) -> int {
		const char* Msg = lua_tostring(L, -1);
		HTR_LOG(Error, "[Lua Panic] Fatal error: {}", Msg ? Msg : "Unknown");
		return 0; // 에디터 강제 종료(abort) 방지
		});

	// 엔진 API 바인딩
	BindEngineAPI();
	bInitialized = true;
	HTR_LOG(Info, "[LuaManager] Successfully initialized LuaJIT & Sol2!");


	return true;
}

void FLuaManager::Shutdown()
{
	if (!bInitialized) return;
	// 루아 VM 종료
	Lua.collect_garbage();
	bInitialized = false;
	HTR_LOG(Info, "[LuaManager] Shutdown completed.");
}

void FLuaManager::BindEngineAPI()
{
	// 기본 print 함수를 엔진 로그 바인딩
	Lua.set_function("print", [](sol::variadic_args args) {
		FString msg;
		for (auto arg : args) {
			msg += arg.as<FString>() + " ";
		}
		HTR_LOG(Info, "[Lua] {}", msg);
		});

	BindVectorAPI();
	BindRotatorAPI();
	BindActorAPI();
	BindInputAPI();
}

void FLuaManager::BindVectorAPI()
{
	Lua.new_usertype<FVector>("Vector",
		sol::constructors<FVector(), FVector(float), FVector(float, float, float)>(),

		"x", &FVector::X,
		"y", &FVector::Y,
		"z", &FVector::Z,

		// 멤버 함수 바인딩, 정규화, 길이, 내적, 외적
		"Normalized", &FVector::Normalized,
		"Length", &FVector::Length,
		"Dot", [](const FVector& a, const FVector& b) { return a.Dot(b); },
		"Cross", [](const FVector& a, const FVector& b) { return a.Cross(b); },
		"Distance", [](const FVector& a, const FVector& b) { return FVector::Distance(a, b); },

		// 기본 사칙 연산자 오버로딩
		sol::meta_function::addition, sol::overload(
			[](const FVector& a, const FVector& b) { return FVector(a.X + b.X, a.Y + b.Y, a.Z + b.Z); },
			[](const FVector& a, float b) { return FVector(a.X + b, a.Y + b, a.Z + b); }
		),
		sol::meta_function::subtraction, sol::overload(
			[](const FVector& a, const FVector& b) { return FVector(a.X - b.X, a.Y - b.Y, a.Z - b.Z); },
			[](const FVector& a, float b) { return FVector(a.X - b, a.Y - b, a.Z - b); }
		),
		sol::meta_function::multiplication, sol::overload(
			[](const FVector& a, const FVector& b) { return FVector(a.X * b.X, a.Y * b.Y, a.Z * b.Z); },
			[](const FVector& a, float b) { return FVector(a.X * b, a.Y * b, a.Z * b); },
			[](float a, const FVector& b) { return FVector(a * b.X, a * b.Y, a * b.Z); }
		),
		sol::meta_function::division, sol::overload(
			[](const FVector& a, const FVector& b) { return FVector(a.X / b.X, a.Y / b.Y, a.Z / b.Z); },
			[](const FVector& a, float b) { return FVector(a.X / b, a.Y / b, a.Z / b); }
		),
		// 문자열 변환
		sol::meta_function::to_string, [](const FVector& v) {
			return "Vector(" + std::to_string(v.X) + ", " + std::to_string(v.Y) + ", " + std::to_string(v.Z) + ")";
		},
		// 비교 연산자 ==, <, <=
		sol::meta_function::equal_to, [](const FVector& a, const FVector& b) {
			return a.X == b.X && a.Y == b.Y && a.Z == b.Z;
		},
		sol::meta_function::less_than, [](const FVector& a, const FVector& b) {
			return a.X < b.X && a.Y < b.Y && a.Z < b.Z;
		},
		sol::meta_function::less_than_or_equal_to, [](const FVector& a, const FVector& b) {
			return a.X <= b.X && a.Y <= b.Y && a.Z <= b.Z;
		},
		sol::meta_function::unary_minus, [](const FVector& v) {
			return FVector(-v.X, -v.Y, -v.Z);
		},

		// 벡터를 회전으로 변환하는 함수 바인딩
		"ToRotator", [](const FVector& V) {
			FRotator Rot;
			Rot.Yaw = std::atan2(V.Y, V.X) * (180.0f / PI);
			Rot.Pitch = std::atan2(V.Z, std::sqrt(V.X * V.X + V.Y * V.Y)) * (180.0f / PI);
			Rot.Roll = 0.0f;
			return Rot;
		}
	);
}

void FLuaManager::BindRotatorAPI()
{
	Lua.new_usertype<FRotator>("Rotator",
		sol::constructors<FRotator(), FRotator(float), FRotator(float, float, float)>(),
		"pitch", &FRotator::Pitch,
		"yaw", &FRotator::Yaw,
		"roll", &FRotator::Roll,

		"Quaternion", &FRotator::Quaternion,

		sol::meta_function::addition, sol::overload(
			[](const FRotator& a, const FRotator& b) { return FRotator(a.Pitch + b.Pitch, a.Yaw + b.Yaw, a.Roll + b.Roll); },
			[](const FRotator& a, const FVector& b) { return FRotator(a.Pitch + b.X, a.Yaw + b.Y, a.Roll + b.Z); }
		),
		sol::meta_function::subtraction, sol::overload(
			[](const FRotator& a, const FRotator& b) { return FRotator(a.Pitch - b.Pitch, a.Yaw - b.Yaw, a.Roll - b.Roll); },
			[](const FRotator& a, const FVector& b) { return FRotator(a.Pitch - b.X, a.Yaw - b.Y, a.Roll - b.Z); }
		),
		sol::meta_function::multiplication, sol::overload(
			[](const FRotator& a, float b) { return FRotator(a.Pitch * b, a.Yaw * b, a.Roll * b); },
			[](float a, const FRotator& b) { return FRotator(a * b.Pitch, a * b.Yaw, a * b.Roll); }
		),
		sol::meta_function::division, sol::overload(
			[](const FRotator& a, float b) { return FRotator(a.Pitch / b, a.Yaw / b, a.Roll / b); }
		)

	);
}

void FLuaManager::BindActorAPI()
{
	// 람다 변수 정의 (중복 제거, 재사용 및 컴파일 최적화)
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

	Lua.new_usertype<AActor>("Actor",
		"UUID", sol::property(&AActor::GetUUID),
		"GetName", &AActor::GetName,

		// 액터 위치 (Short & UE 네이티브 & 프로퍼티)
		"GetLocation", GetLoc,
		"GetActorLocation", GetLoc,
		"SetLocation", SetLoc,
		"SetActorLocation", SetLoc,
		"Location", sol::property(GetLoc, SetLoc),
		"AddLocation", AddLoc,

		// 액터 회전 (Short & UE 네이티브 & 프로퍼티)
		"GetRotation", GetRot,
		"GetActorRotation", GetRot,
		"SetRotation", SetRot,
		"SetActorRotation", SetRot,
		"Rotation", sol::property(GetRot, SetRot),
		"AddRotation", AddRot,

		// 액터 이동 속도
		"Velocity", sol::property(
			[](AActor* Actor) -> FVector {
				return (Actor && Actor->GetRootComponent()) ? Actor->GetRootComponent()->ComponentVelocity : FVector::ZeroVector;
			},
			[](AActor* Actor, const FVector& NewVel) {
				if (Actor && Actor->GetRootComponent())
					Actor->GetRootComponent()->ComponentVelocity = NewVel;
			}
		),

		// 방향 벡터 (Short & UE 네이티브)
		"GetForwardVector", GetFwd,
		"GetActorForwardVector", GetFwd,
		"GetRightVector", GetRight,
		"GetActorRightVector", GetRight,
		"GetUpVector", GetUp,
		"GetActorUpVector", GetUp,

		// 스케일 (Short & UE 네이티브)
		"GetScale", GetScale,
		"GetActorScale", GetScale,
		"GetActorScale3D", GetScale,
		"SetScale", SetScale,
		"SetActorScale", SetScale,
		"SetActorScale3D", SetScale,

		// 가시성 & 상태 (Short & UE 네이티브)
		"SetVisibility", SetVis,
		"SetActorHiddenInGame", [SetVis](AActor* Actor, bool bHidden) { SetVis(Actor, !bHidden); },
		"IsVisible", IsVis,
		"IsActorHiddenInGame", [IsVis](AActor* Actor) -> bool { return !IsVis(Actor); },
		"IsValid", [](AActor* Actor) -> bool { return Actor != nullptr && Actor->GetWorld() != nullptr; },

		// 디버그 위치 출력
		"PrintLocation", [](AActor* Actor) {
			if (!Actor) return;
			FVector loc = Actor->GetActorLocation();
			HTR_LOG(Info, "[Actor: {}] Location: X={:.2f}, Y={:.2f}, Z={:.2f}",
				Actor->GetName(), loc.X, loc.Y, loc.Z);
		},

		// 레이캐스트
		"LineTrace", [](AActor* Actor, float MaxDistance) -> sol::table {
			sol::table Result = FLuaManager::Get().GetState().create_table();
			Result["hit"] = false;

			if (!Actor || !Actor->GetWorld() || !Actor->GetRootComponent())
				return Result;

			// 액터 위치에서 전방 방향으로 광선 생성
			FVector Start = Actor->GetActorLocation();
			FVector Dir = Actor->GetRootComponent()->GetTransform().GetForward();

			FRay Ray;
			Ray.Origin = Start;
			Ray.Direction = Dir;

			FHitResult Hit;
			if (Actor->GetWorld()->LineTraceSingle(Ray, Hit))
			{
				if (Hit.Distance <= MaxDistance)
				{
					Result["hit"] = true;
					Result["distance"] = Hit.Distance;
					Result["point"] = Hit.ImpactPoint;
					Result["actor"] = Hit.HitComponent ? Hit.HitComponent->GetOwner() : nullptr;
					Result["normal"] = Hit.ImpactNormal;
				}
			}
			return Result;
		},

		// 씬 액터 검색
		"FindActorByName", [](const AActor* Actor, const FString& Name) -> AActor* {
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
		},

		// 액터 파괴
		"Destroy", &AActor::Destroy
	);
}

void FLuaManager::BindInputAPI()
{
	// Key 테이블에 모든 키 등록
	sol::table KeyTable = Lua.create_named_table("Key");
	for (const auto& [name, code] : StringToKeyMap)
	{
		KeyTable[name] = code;
	}

	// MouseButton 열거형 테이블 등록
	Lua.new_enum<EMouseButton>("MouseButton", {
		{ "Left",   EMouseButton::Left },
		{ "Right",  EMouseButton::Right },
		{ "Middle", EMouseButton::Middle },
		{ "Side1",  EMouseButton::Side1 },
		{ "Side2",  EMouseButton::Side2 }
		});

	// Input 테이블 생성
	auto InputTable = Lua.create_named_table("Input");

	// 키보드 바인딩: IsKeyDown, IsKeyPressed, IsKeyReleased
	InputTable["IsKeyDown"] = sol::overload(
		[](EKeyCode code) { return FInputSystem::IsKeyDown(code); },
		[](const FString& keyName) {
			auto it = StringToKeyMap.find(keyName);
			return (it != StringToKeyMap.end()) ? FInputSystem::IsKeyDown(it->second) : false;
		}
	);

	InputTable["IsKeyPressed"] = sol::overload(
		[](EKeyCode code) { return FInputSystem::IsKeyPressed(code); },
		[](const FString& keyName) {
			auto it = StringToKeyMap.find(keyName);
			return (it != StringToKeyMap.end()) ? FInputSystem::IsKeyPressed(it->second) : false;
		}
	);

	InputTable["IsKeyReleased"] = sol::overload(
		[](EKeyCode code) { return FInputSystem::IsKeyReleased(code); },
		[](const FString& keyName) {
			auto it = StringToKeyMap.find(keyName);
			return (it != StringToKeyMap.end()) ? FInputSystem::IsKeyReleased(it->second) : false;
		}
	);

	// 마우스 바인딩: IsMouseDown, IsMousePressed, IsMouseReleased
	InputTable["IsMouseDown"] = sol::overload(
		[](EMouseButton btn) { return FInputSystem::IsMouseDown(btn); },
		[](int btn) { return FInputSystem::IsMouseDown(static_cast<EMouseButton>(btn)); },
		[](const FString& btn) {
			if (btn == "Left" || btn == "left") return FInputSystem::IsMouseDown(EMouseButton::Left);
			if (btn == "Right" || btn == "right") return FInputSystem::IsMouseDown(EMouseButton::Right);
			if (btn == "Middle" || btn == "middle") return FInputSystem::IsMouseDown(EMouseButton::Middle);
			return false;
		}
	);
	InputTable["IsMousePressed"] = sol::overload(
		[](EMouseButton btn) { return FInputSystem::IsMousePressed(btn); },
		[](int btn) { return FInputSystem::IsMousePressed(static_cast<EMouseButton>(btn)); }
	);
	InputTable["IsMouseReleased"] = sol::overload(
		[](EMouseButton btn) { return FInputSystem::IsMouseReleased(btn); },
		[](int btn) { return FInputSystem::IsMouseReleased(static_cast<EMouseButton>(btn)); }
	);

	InputTable["GetAxis"] = [](const FString& AxisName) -> float {
		if (AxisName == "Horizontal" || AxisName == "horizontal") {
			float Value = 0.0f;
			if (FInputSystem::IsKeyDown(EKeyCode::D) || FInputSystem::IsKeyDown(EKeyCode::Right)) Value += 1.0f;
			if (FInputSystem::IsKeyDown(EKeyCode::A) || FInputSystem::IsKeyDown(EKeyCode::Left)) Value -= 1.0f;
			return Value;
		}
		if (AxisName == "Vertical" || AxisName == "vertical") {
			float Value = 0.0f;
			if (FInputSystem::IsKeyDown(EKeyCode::W) || FInputSystem::IsKeyDown(EKeyCode::Up)) Value += 1.0f;
			if (FInputSystem::IsKeyDown(EKeyCode::S) || FInputSystem::IsKeyDown(EKeyCode::Down)) Value -= 1.0f;
			return Value;
		}
		return 0.0f;
		};

	// 모든 키 입력 상태 확인
	InputTable["IsAnyKeyDown"] = &FInputSystem::IsAnyKeyDown;
	InputTable["IsAnyKeyPressed"] = &FInputSystem::IsAnyKeyPressed;

	// 마우스 위치 및 휠 델타 바인딩
	InputTable["GetMouseDeltaX"] = &FInputSystem::GetMouseDeltaX;
	InputTable["GetMouseDeltaY"] = &FInputSystem::GetMouseDeltaY;
	InputTable["GetMouseX"] = &FInputSystem::GetMouseX;
	InputTable["GetMouseY"] = &FInputSystem::GetMouseY;
	InputTable["GetMousePosition"] = []() {
		return std::make_tuple(FInputSystem::GetMouseX(), FInputSystem::GetMouseY());
		};
	InputTable["GetMouseDelta"] = []() {
		return std::make_tuple(FInputSystem::GetMouseDeltaX(), FInputSystem::GetMouseDeltaY());
		};
	InputTable["GetWheelDelta"] = &FInputSystem::GetWheelDelta;


}