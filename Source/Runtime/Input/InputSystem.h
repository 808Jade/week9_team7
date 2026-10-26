#pragma once

#include "KeyCode.h"
#include "MouseButton.h"
#include "Core/Types.h"

enum class EGameInputAxis : uint8 { MouseX, MouseY, MouseWheel };

class FInputSystem
{
public:
    static bool IsKeyPressed(EKeyCode KeyCode);
    static bool IsKeyReleased(EKeyCode KeyCode);
    static bool IsKeyDown(EKeyCode KeyCode);
	static bool IsAnyKeyDown();
	static bool IsAnyKeyPressed();

    static bool IsMouseDown(EMouseButton MouseButton);
    static bool IsMousePressed(EMouseButton MouseButton);
    static bool IsMouseReleased(EMouseButton MouseButton);

    static void UpdateInputStates();
    static void ClearAllStates();
    static void SyncMousePosition(int32 x, int32 y);
    static void OnKeyUp(int VirtualKey);
    static void OnMouseUp(EMouseButton MouseButton);
    static void OnKeyDown(int VirtualKey);
    static void OnMouseDown(EMouseButton MouseButton);
    static void OnMouseMove(int32 x, int32 y);
    static void OnMouseWheelDelta(int32 Wheel);

    static inline int32 GetMouseX() { return MouseX; }
    static inline int32 GetMouseY() { return MouseY; }
	static inline std::pair<int32, int32> GetMousePosition() { return { MouseX, MouseY }; }

    static inline int32 GetMouseDeltaX() { return DeltaX; }
    static inline int32 GetMouseDeltaY() { return DeltaY; }

    // 마우스 잠금 시 커서를 되돌릴 지점(스크린 좌표). 지정하지 않으면 컨트롤러가 창 중앙을 쓴다.
    // 에디터 PIE가 매 프레임 PIE 뷰 중앙으로 지정한다.
    static inline void SetMouseLockCenter(int32 ScreenX, int32 ScreenY)
    {
        bHasMouseLockCenter = true;
        MouseLockCenterX = ScreenX;
        MouseLockCenterY = ScreenY;
    }
    static inline void ClearMouseLockCenter() { bHasMouseLockCenter = false; }
    static inline bool GetMouseLockCenter(int32& OutX, int32& OutY)
    {
        if (!bHasMouseLockCenter) return false;
        OutX = MouseLockCenterX;
        OutY = MouseLockCenterY;
        return true;
    }

    static inline int32 GetWheelDelta()
    {
        int32 Delta = WheelDelta;
        WheelDelta = 0;
        return Delta;
    }

private:
    FInputSystem() = delete;

    static inline bool bMouseStates[5] = {};
    static inline bool bPrevMouseStates[5] = {};

    static inline bool bKeyStates[256] = {};
    static inline bool bPrevKeyStates[256] = {};

    static inline int32 MouseX = 0, MouseY = 0;
    static inline int32 PrevMouseX = 0, PrevMouseY = 0;
    static inline int32 DeltaX = 0, DeltaY = 0;

    static inline int32 WheelDelta = 0;

    static inline bool bHasMouseLockCenter = false;
    static inline int32 MouseLockCenterX = 0, MouseLockCenterY = 0;
};
