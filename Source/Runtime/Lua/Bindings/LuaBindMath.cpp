#include "EnginePCH.h"
#include "LuaBindings.h"

#include "Math/Vector.h"
#include "Math/Vector2.h"
#include "Math/Rotator.h"
#include "Math/Quat.h"
#include "Math/Color.h"
#include "Math/EngineMath.h"

namespace LuaBindings
{
	static void BindVector2(sol::state& Lua)
	{
		sol::usertype<FVector2> Vector2Type = Lua.new_usertype<FVector2>("Vector2",
			sol::constructors<FVector2(), FVector2(float), FVector2(float, float)>()
		);

		// 필드
		Vector2Type["x"] = &FVector2::X;
		Vector2Type["y"] = &FVector2::Y;
		Vector2Type["X"] = &FVector2::X;
		Vector2Type["Y"] = &FVector2::Y;

		// 멤버 함수
		Vector2Type["Length"] = [](const FVector2& v) {
			return std::sqrt(v.X * v.X + v.Y * v.Y);
		};
		Vector2Type["LengthSquared"] = [](const FVector2& v) {
			return v.X * v.X + v.Y * v.Y;
		};
		Vector2Type["Normalized"] = [](const FVector2& v) {
			float len = std::sqrt(v.X * v.X + v.Y * v.Y);
			return (len > 1e-4f) ? FVector2(v.X / len, v.Y / len) : FVector2(0.0f, 0.0f);
		};
		Vector2Type["Distance"] = [](const FVector2& a, const FVector2& b) {
			float dx = a.X - b.X;
			float dy = a.Y - b.Y;
			return std::sqrt(dx * dx + dy * dy);
		};

		// 연산자 오버로딩
		Vector2Type[sol::meta_function::addition] = sol::overload(
			[](const FVector2& a, const FVector2& b) { return FVector2(a.X + b.X, a.Y + b.Y); },
			[](const FVector2& a, float b) { return FVector2(a.X + b, a.Y + b); }
		);
		Vector2Type[sol::meta_function::subtraction] = sol::overload(
			[](const FVector2& a, const FVector2& b) { return FVector2(a.X - b.X, a.Y - b.Y); },
			[](const FVector2& a, float b) { return FVector2(a.X - b, a.Y - b); }
		);
		Vector2Type[sol::meta_function::multiplication] = sol::overload(
			[](const FVector2& a, const FVector2& b) { return FVector2(a.X * b.X, a.Y * b.Y); },
			[](const FVector2& a, float b) { return FVector2(a.X * b, a.Y * b); },
			[](float a, const FVector2& b) { return FVector2(a * b.X, a * b.Y); }
		);
		Vector2Type[sol::meta_function::division] = sol::overload(
			[](const FVector2& a, float b) { return FVector2(a.X / b, a.Y / b); }
		);
		Vector2Type[sol::meta_function::to_string] = [](const FVector2& v) {
			return "Vector2(" + std::to_string(v.X) + ", " + std::to_string(v.Y) + ")";
		};
		Vector2Type[sol::meta_function::equal_to] = [](const FVector2& a, const FVector2& b) {
			return a.X == b.X && a.Y == b.Y;
		};
		Vector2Type[sol::meta_function::unary_minus] = [](const FVector2& v) {
			return FVector2(-v.X, -v.Y);
		};

		// 정적 상수 및 생성자 메타테이블
		sol::table Vector2Table = Lua["Vector2"];
		Vector2Table["Zero"] = FVector2(0.0f, 0.0f);
		Vector2Table["One"] = FVector2(1.0f, 1.0f);

		sol::table Vector2Meta = Lua.create_table();
		Vector2Meta[sol::meta_function::call] = [](sol::object /*self*/, sol::variadic_args args) -> FVector2 {
			if (args.size() == 0) return FVector2(0.0f, 0.0f);
			if (args.size() == 1) return FVector2(args.get<float>(0));
			return FVector2(args.get<float>(0), args.get<float>(1));
		};
		Vector2Table[sol::metatable_key] = Vector2Meta;
		Lua["FVector2"] = Vector2Table;
	}

	static void BindColor(sol::state& Lua)
	{
		sol::usertype<FColor> ColorType = Lua.new_usertype<FColor>("Color",
			sol::factories(
				[]() { return FColor(255, 255, 255, 255); },
				[](uint8 r, uint8 g, uint8 b) { return FColor(r, g, b, 255); },
				[](uint8 r, uint8 g, uint8 b, uint8 a) { return FColor(r, g, b, a); }
			)
		);

		// 필드
		ColorType["r"] = &FColor::R;
		ColorType["g"] = &FColor::G;
		ColorType["b"] = &FColor::B;
		ColorType["a"] = &FColor::A;
		ColorType["R"] = &FColor::R;
		ColorType["G"] = &FColor::G;
		ColorType["B"] = &FColor::B;
		ColorType["A"] = &FColor::A;

		ColorType[sol::meta_function::to_string] = [](const FColor& c) {
			return "Color(" + std::to_string(c.R) + ", " + std::to_string(c.G) + ", " + std::to_string(c.B) + ", " + std::to_string(c.A) + ")";
		};
		ColorType[sol::meta_function::equal_to] = [](const FColor& a, const FColor& b) {
			return a == b;
		};

		// 정적 색상 상수 및 생성자 메타테이블
		sol::table ColorTable = Lua["Color"];
		ColorTable["White"] = FColor(255, 255, 255, 255);
		ColorTable["Black"] = FColor(0, 0, 0, 255);
		ColorTable["Red"] = FColor(255, 0, 0, 255);
		ColorTable["Green"] = FColor(0, 255, 0, 255);
		ColorTable["Blue"] = FColor(0, 0, 255, 255);
		ColorTable["Yellow"] = FColor(255, 255, 0, 255);
		ColorTable["Orange"] = FColor(255, 165, 0, 255);
		ColorTable["Purple"] = FColor(128, 0, 128, 255);
		ColorTable["Cyan"] = FColor(0, 255, 255, 255);
		ColorTable["Magenta"] = FColor(255, 0, 255, 255);
		ColorTable["Transparent"] = FColor(0, 0, 0, 0);

		sol::table ColorMeta = Lua.create_table();
		ColorMeta[sol::meta_function::call] = [](sol::object /*self*/, sol::variadic_args args) -> FColor {
			if (args.size() == 0) return FColor(255, 255, 255, 255);
			if (args.size() <= 3) return FColor(args.get<uint8>(0), args.get<uint8>(1), args.get<uint8>(2), 255);
			return FColor(args.get<uint8>(0), args.get<uint8>(1), args.get<uint8>(2), args.get<uint8>(3));
		};
		ColorTable[sol::metatable_key] = ColorMeta;
		Lua["FColor"] = ColorTable;
	}

	static void BindVector(sol::state& Lua)
	{
		sol::usertype<FVector> VectorType = Lua.new_usertype<FVector>("Vector",
			sol::constructors<FVector(), FVector(float), FVector(float, float, float)>()
		);

		// 필드
		VectorType["x"] = &FVector::X;
		VectorType["y"] = &FVector::Y;
		VectorType["z"] = &FVector::Z;
		VectorType["X"] = &FVector::X;
		VectorType["Y"] = &FVector::Y;
		VectorType["Z"] = &FVector::Z;

		// 람다 정의
		auto GetLenSq = [](const FVector& v) {
			return v.X * v.X + v.Y * v.Y + v.Z * v.Z;
		};

		// 멤버 함수
		VectorType["Normalized"] = &FVector::Normalized;
		VectorType["Length"] = &FVector::Length;
		VectorType["LengthSquared"] = GetLenSq;
		VectorType["SizeSquared"] = GetLenSq;
		VectorType["IsNearlyZero"] = [](const FVector& v) {
			return std::abs(v.X) < 1e-4f && std::abs(v.Y) < 1e-4f && std::abs(v.Z) < 1e-4f;
		};
		VectorType["Reflect"] = [](const FVector& V, const FVector& Normal) -> FVector {
			return V - (Normal * (2.0f * V.Dot(Normal)));
		};
		VectorType["ClampLength"] = [](const FVector& V, float MaxLength) -> FVector {
			float LenSq = V.X * V.X + V.Y * V.Y + V.Z * V.Z;
			if (LenSq > (MaxLength * MaxLength) && LenSq > 1e-8f) {
				float Scale = MaxLength / std::sqrt(LenSq);
				return FVector(V.X * Scale, V.Y * Scale, V.Z * Scale);
			}
			return V;
		};
		VectorType["Dot"] = [](const FVector& a, const FVector& b) { return a.Dot(b); };
		VectorType["Cross"] = [](const FVector& a, const FVector& b) { return a.Cross(b); };
		VectorType["Distance"] = [](const FVector& a, const FVector& b) { return FVector::Distance(a, b); };

		// 연산자 오버로딩
		VectorType[sol::meta_function::addition] = sol::overload(
			[](const FVector& a, const FVector& b) { return FVector(a.X + b.X, a.Y + b.Y, a.Z + b.Z); },
			[](const FVector& a, float b) { return FVector(a.X + b, a.Y + b, a.Z + b); }
		);
		VectorType[sol::meta_function::subtraction] = sol::overload(
			[](const FVector& a, const FVector& b) { return FVector(a.X - b.X, a.Y - b.Y, a.Z - b.Z); },
			[](const FVector& a, float b) { return FVector(a.X - b, a.Y - b, a.Z - b); }
		);
		VectorType[sol::meta_function::multiplication] = sol::overload(
			[](const FVector& a, const FVector& b) { return FVector(a.X * b.X, a.Y * b.Y, a.Z * b.Z); },
			[](const FVector& a, float b) { return FVector(a.X * b, a.Y * b, a.Z * b); },
			[](float a, const FVector& b) { return FVector(a * b.X, a * b.Y, a * b.Z); }
		);
		VectorType[sol::meta_function::division] = sol::overload(
			[](const FVector& a, const FVector& b) { return FVector(a.X / b.X, a.Y / b.Y, a.Z / b.Z); },
			[](const FVector& a, float b) { return FVector(a.X / b, a.Y / b, a.Z / b); }
		);
		VectorType[sol::meta_function::to_string] = [](const FVector& v) {
			return "Vector(" + std::to_string(v.X) + ", " + std::to_string(v.Y) + ", " + std::to_string(v.Z) + ")";
		};
		VectorType[sol::meta_function::equal_to] = [](const FVector& a, const FVector& b) {
			return a.X == b.X && a.Y == b.Y && a.Z == b.Z;
		};
		VectorType[sol::meta_function::less_than] = [](const FVector& a, const FVector& b) {
			return a.X < b.X && a.Y < b.Y && a.Z < b.Z;
		};
		VectorType[sol::meta_function::less_than_or_equal_to] = [](const FVector& a, const FVector& b) {
			return a.X <= b.X && a.Y <= b.Y && a.Z <= b.Z;
		};
		VectorType[sol::meta_function::unary_minus] = [](const FVector& v) {
			return FVector(-v.X, -v.Y, -v.Z);
		};

		// 벡터 -> 회전 변환
		VectorType["ToRotator"] = [](const FVector& V) {
			FRotator Rot;
			Rot.Yaw = std::atan2(V.Y, V.X) * (180.0f / PI);
			Rot.Pitch = std::atan2(V.Z, std::sqrt(V.X * V.X + V.Y * V.Y)) * (180.0f / PI);
			Rot.Roll = 0.0f;
			return Rot;
		};

		// 벡터 선형 보간 (멤버 함수 및 정적 함수)
		VectorType["Lerp"] = sol::overload(
			[](const FVector& A, const FVector& B, float Alpha) {
				return A + (B - A) * Alpha;
			}
		);

		// 스타일 정적 벡터 상수 등록
		sol::table VectorTable = Lua["Vector"];
		VectorTable["Zero"] = FVector(0.0f, 0.0f, 0.0f);
		VectorTable["ZeroVector"] = FVector(0.0f, 0.0f, 0.0f);
		VectorTable["One"] = FVector(1.0f, 1.0f, 1.0f);
		VectorTable["Forward"] = FVector(1.0f, 0.0f, 0.0f);
		VectorTable["ForwardVector"] = FVector(1.0f, 0.0f, 0.0f);
		VectorTable["Right"] = FVector(0.0f, 1.0f, 0.0f);
		VectorTable["RightVector"] = FVector(0.0f, 1.0f, 0.0f);
		VectorTable["Up"] = FVector(0.0f, 0.0f, 1.0f);
		VectorTable["UpVector"] = FVector(0.0f, 0.0f, 1.0f);

		// 함수처럼 Vector(x, y, z) 직접 호출 가능하도록 메타테이블 __call 설정
		sol::table VectorMeta = Lua.create_table();
		VectorMeta[sol::meta_function::call] = [](sol::object /*self*/, sol::variadic_args args) -> FVector {
			if (args.size() == 0) return FVector(0.0f, 0.0f, 0.0f);
			if (args.size() == 1) return FVector(args.get<float>(0));
			return FVector(args.get<float>(0), args.get<float>(1), args.get<float>(2));
		};
		VectorTable[sol::metatable_key] = VectorMeta;
		Lua["FVector"] = VectorTable;
	}

	static void BindRotator(sol::state& Lua)
	{
		sol::usertype<FRotator> RotatorType = Lua.new_usertype<FRotator>("Rotator",
			sol::constructors<FRotator(), FRotator(float), FRotator(float, float, float)>()
		);

		// 필드
		RotatorType["pitch"] = &FRotator::Pitch;
		RotatorType["yaw"] = &FRotator::Yaw;
		RotatorType["roll"] = &FRotator::Roll;
		RotatorType["Pitch"] = &FRotator::Pitch;
		RotatorType["Yaw"] = &FRotator::Yaw;
		RotatorType["Roll"] = &FRotator::Roll;

		RotatorType["Quaternion"] = &FRotator::Quaternion;

		// 연산자 오버로딩
		RotatorType[sol::meta_function::addition] = sol::overload(
			[](const FRotator& a, const FRotator& b) { return FRotator(a.Pitch + b.Pitch, a.Yaw + b.Yaw, a.Roll + b.Roll); },
			[](const FRotator& a, const FVector& b) { return FRotator(a.Pitch + b.X, a.Yaw + b.Y, a.Roll + b.Z); }
		);
		RotatorType[sol::meta_function::subtraction] = sol::overload(
			[](const FRotator& a, const FRotator& b) { return FRotator(a.Pitch - b.Pitch, a.Yaw - b.Yaw, a.Roll - b.Roll); },
			[](const FRotator& a, const FVector& b) { return FRotator(a.Pitch - b.X, a.Yaw - b.Y, a.Roll - b.Z); }
		);
		RotatorType[sol::meta_function::multiplication] = sol::overload(
			[](const FRotator& a, float b) { return FRotator(a.Pitch * b, a.Yaw * b, a.Roll * b); },
			[](float a, const FRotator& b) { return FRotator(a * b.Pitch, a * b.Yaw, a * b.Roll); }
		);
		RotatorType[sol::meta_function::division] = sol::overload(
			[](const FRotator& a, float b) { return FRotator(a.Pitch / b, a.Yaw / b, a.Roll / b); }
		);

		// 각도 정규화 (-180 ~ +180)
		auto NormalizeAxis = [](float Angle) -> float {
			Angle = std::fmod(Angle, 360.0f);
			if (Angle > 180.0f) Angle -= 360.0f;
			else if (Angle < -180.0f) Angle += 360.0f;
			return Angle;
		};
		auto NormalizeRot = [NormalizeAxis](const FRotator& R) -> FRotator {
			return FRotator(NormalizeAxis(R.Pitch), NormalizeAxis(R.Yaw), NormalizeAxis(R.Roll));
		};

		RotatorType["Normalized"] = NormalizeRot;
		RotatorType["Clamp"] = NormalizeRot;

		// UnLua 스타일: Rotator -> Direction Vectors
		auto GetRotForward = [](const FRotator& R) -> FVector {
			return R.Quaternion().RotateVector(FVector(1.0f, 0.0f, 0.0f));
		};
		auto GetRotRight = [](const FRotator& R) -> FVector {
			return R.Quaternion().RotateVector(FVector(0.0f, 1.0f, 0.0f));
		};
		auto GetRotUp = [](const FRotator& R) -> FVector {
			return R.Quaternion().RotateVector(FVector(0.0f, 0.0f, 1.0f));
		};

		RotatorType["GetForwardVector"] = GetRotForward;
		RotatorType["Vector"] = GetRotForward; // 언리얼 네이티브 별칭
		RotatorType["GetRightVector"] = GetRotRight;
		RotatorType["GetUpVector"] = GetRotUp;

		// 스타일 정적 로테이터 상수 등록
		sol::table RotatorTable = Lua["Rotator"];
		RotatorTable["Zero"] = FRotator(0.0f, 0.0f, 0.0f);
		RotatorTable["ZeroRotator"] = FRotator(0.0f, 0.0f, 0.0f);

		// 함수처럼 Rotator(p, y, r) 직접 호출 가능하도록 메타테이블 __call 설정
		sol::table RotatorMeta = Lua.create_table();
		RotatorMeta[sol::meta_function::call] = [](sol::object /*self*/, sol::variadic_args args) -> FRotator {
			if (args.size() == 0) return FRotator(0.0f, 0.0f, 0.0f);
			if (args.size() == 1) return FRotator(args.get<float>(0));
			return FRotator(args.get<float>(0), args.get<float>(1), args.get<float>(2));
		};
		RotatorTable[sol::metatable_key] = RotatorMeta;
		Lua["FRotator"] = RotatorTable;
	}

	void BindMath(sol::state& Lua)
	{
		// 기존 루아 내장 math 테이블을 가져와 유용한 게임 수학 함수 추가
		sol::table MathTable = Lua["math"];

		MathTable["lerp"] = [](float A, float B, float Alpha) {
			return A + (B - A) * Alpha;
		};

		MathTable["clamp"] = [](float Value, float Min, float Max) {
			return FMath::Clamp(Value, Min, Max);
		};

		MathTable["clamp01"] = [](float Value) {
			return FMath::Clamp(Value, 0.0f, 1.0f);
		};

		MathTable["sign"] = [](float X) -> float {
			return (X > 0.0f) ? 1.0f : ((X < 0.0f) ? -1.0f : 0.0f);
		};

		MathTable["approx"] = sol::overload(
			[](float A, float B) { return std::abs(A - B) <= 1e-4f; },
			[](float A, float B, float Tolerance) { return std::abs(A - B) <= Tolerance; }
		);

		MathTable["remap"] = [](float Val, float InMin, float InMax, float OutMin, float OutMax) -> float {
			if (std::abs(InMax - InMin) < 1e-6f) return OutMin;
			float T = (Val - InMin) / (InMax - InMin);
			return OutMin + (OutMax - OutMin) * T;
		};

		MathTable["pingpong"] = [](float T, float Length) -> float {
			if (Length <= 0.0f) return 0.0f;
			float Cycle = std::fmod(T, Length * 2.0f);
			if (Cycle < 0.0f) Cycle += Length * 2.0f;
			return Length - std::abs(Cycle - Length);
		};

		BindVector(Lua);
		BindVector2(Lua);
		BindRotator(Lua);
		BindColor(Lua);
	}
}
