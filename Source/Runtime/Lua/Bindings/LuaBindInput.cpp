#include "EnginePCH.h"
#include "LuaBindings.h"

#include "Input/InputSystem.h"

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

namespace LuaBindings
{
	void BindInput(sol::state& Lua)
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

		// 특수 조합키 편의 헬퍼
		InputTable["IsShiftDown"] = []() {
			return FInputSystem::IsKeyDown(EKeyCode::Shift) || FInputSystem::IsKeyDown(EKeyCode::LShift) || FInputSystem::IsKeyDown(EKeyCode::RShift);
		};
		InputTable["IsCtrlDown"] = []() {
			return FInputSystem::IsKeyDown(EKeyCode::Control) || FInputSystem::IsKeyDown(EKeyCode::LControl) || FInputSystem::IsKeyDown(EKeyCode::RControl);
		};
		InputTable["IsAltDown"] = []() {
			return FInputSystem::IsKeyDown(EKeyCode::Alt) || FInputSystem::IsKeyDown(EKeyCode::LAlt) || FInputSystem::IsKeyDown(EKeyCode::RAlt);
		};

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
			[](int btn) { return FInputSystem::IsMousePressed(static_cast<EMouseButton>(btn)); },
			[](const FString& btn) {
				if (btn == "Left" || btn == "left") return FInputSystem::IsMousePressed(EMouseButton::Left);
				if (btn == "Right" || btn == "right") return FInputSystem::IsMousePressed(EMouseButton::Right);
				if (btn == "Middle" || btn == "middle") return FInputSystem::IsMousePressed(EMouseButton::Middle);
				return false;
			}
		);
		InputTable["IsMouseReleased"] = sol::overload(
			[](EMouseButton btn) { return FInputSystem::IsMouseReleased(btn); },
			[](int btn) { return FInputSystem::IsMouseReleased(static_cast<EMouseButton>(btn)); },
			[](const FString& btn) {
				if (btn == "Left" || btn == "left") return FInputSystem::IsMouseReleased(EMouseButton::Left);
				if (btn == "Right" || btn == "right") return FInputSystem::IsMouseReleased(EMouseButton::Right);
				if (btn == "Middle" || btn == "middle") return FInputSystem::IsMouseReleased(EMouseButton::Middle);
				return false;
			}
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
}
