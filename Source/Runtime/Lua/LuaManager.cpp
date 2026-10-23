#include "EnginePCH.h"
#include "LuaManager.h"

#include "Math/Vector.h"
#include "Math/Rotator.h" 
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Input/InputSystem.h"
#include "Core/EngineLog.h"

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
		std::string msg;
		for (auto arg : args) {
			msg += arg.as<std::string>() + " ";
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
		"Normalized", & FVector::Normalized,
		"Length", & FVector::Length,
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

		"Quaternion", & FRotator::Quaternion,
		
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
	Lua.new_usertype<AActor>("Actor",
		"UUID", sol::property(&AActor::GetUUID),
		"GetName", &AActor::GetName,

		// 액터 위치
		"GetLocation", &AActor::GetActorLocation,
		"SetLocation", [](AActor* Actor, const FVector& NewLoc) {
			if (Actor->GetRootComponent())
				Actor->GetRootComponent()->SetWorldLocation(NewLoc, false);
		},
		"Location", sol::property(
			&AActor::GetActorLocation,
			[](AActor* Actor, const FVector& NewLoc) {
				if (Actor->GetRootComponent())
					Actor->GetRootComponent()->SetWorldLocation(NewLoc, false);
			}
		),

		"AddLocation", [](AActor* Actor, const FVector& Delta) {
			if (Actor->GetRootComponent()) {
				FVector cur = Actor->GetActorLocation();
				Actor->GetRootComponent()->SetWorldLocation(cur + Delta, false);
			}
		},

		// 액터 회전
		"GetRotation", &AActor::GetActorRotation,
		"SetRotation", [](AActor* Actor, const FRotator& NewRot) {
			if (Actor->GetRootComponent())
				Actor->GetRootComponent()->SetWorldRotation(NewRot, false);
		},
		"Rotation", sol::property(
			&AActor::GetActorRotation,
			[](AActor* Actor, const FRotator& NewRot) {
				if (Actor->GetRootComponent())
					Actor->GetRootComponent()->SetWorldRotation(NewRot, false);
			}
		),

		"AddRotation", [](AActor* Actor, const FRotator& Delta) {
			if (Actor->GetRootComponent()) {
				FRotator cur = Actor->GetActorRotation();
				Actor->GetRootComponent()->SetWorldRotation(cur + Delta, false);
			}
		},

		"PrintLocation", [](AActor* Actor) {
			FVector loc = Actor->GetActorLocation();
			HTR_LOG(Info, "[Actor: {}] Location: X={:.2f}, Y={:.2f}, Z={:.2f}",
				Actor->GetName(), loc.X, loc.Y, loc.Z);
		},

		"Velocity", sol::property(
			[](AActor* Actor) -> FVector {
				return Actor->GetRootComponent() ? Actor->GetRootComponent()->ComponentVelocity : FVector::ZeroVector;
			},
			[](AActor* Actor, const FVector& NewVel) {
				if (Actor->GetRootComponent())
					Actor->GetRootComponent()->ComponentVelocity = NewVel;
			}),

		// 4. 액터 파괴
		"Destroy", & AActor::Destroy
	);
}

void FLuaManager::BindInputAPI()
{
	Lua.new_enum<EKeyCode>("Key", {
	   { "W",      EKeyCode::W },
	   { "A",      EKeyCode::A },
	   { "S",      EKeyCode::S },
	   { "D",      EKeyCode::D },
	   { "Space",  EKeyCode::Space },
	   { "Shift",  EKeyCode::Shift },
	   { "Escape", EKeyCode::Escape },
	   { "Enter",  EKeyCode::Enter }
		});

	auto inputTable = Lua.create_named_table("Input");
	inputTable["IsKeyDown"] = &FInputSystem::IsKeyDown;
	inputTable["IsKeyPressed"] = &FInputSystem::IsKeyPressed;
}