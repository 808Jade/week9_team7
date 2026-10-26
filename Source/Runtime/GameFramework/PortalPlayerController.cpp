#include "EnginePCH.h"
#include "GameFramework/PortalPlayerController.h"
#include "GameFramework/PortalPawn.h"
#include "Input/InputSystem.h"

APortalPlayerController::APortalPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	LookSensitivity = 0.1f;
}

APortalPlayerController::~APortalPlayerController()
{
	// 컨트롤러 파괴 시 마우스 커서 원상복구
	SetMouseCapture(false);
}

void APortalPlayerController::BeginPlay()
{
	Super::BeginPlay();
	// 게임 시작 시 마우스 잠금 & 커서 숨김
	SetMouseCapture(true);
}

void APortalPlayerController::EndPlay()
{
	Super::EndPlay();
	// 게임 종료 / PIE 정지 시 마우스 커서 반드시 복원
	SetMouseCapture(false);
}

APortalPawn* APortalPlayerController::GetPortalPawn() const
{
	return Cast<APortalPawn>(PossessedPawn);
}

void APortalPlayerController::SetMouseCapture(bool bEnable)
{
	if (bMouseCaptured == bEnable) return;
	bMouseCaptured = bEnable;

	if (bMouseCaptured)
	{
		// Windows ShowCursor는 내부 카운터를 쓰므로 음수가 될 때까지 호출해 완전히 숨김
		while (::ShowCursor(FALSE) >= 0);
	}
	else
	{
		// 마우스 보이기: 카운터가 0 이상이 될 때까지 호출
		while (::ShowCursor(TRUE) < 0);
	}
}

void APortalPlayerController::ProcessPlayerInput(float DeltaTime)
{
	// 1. 윈도우 키(LWin, RWin)를 눌렀거나, 다른 창/시작메뉴로 포커스가 빠져나가면 즉시 마우스 잠금 해제 & 커서 표시
	const bool bWinKeyPressed = ((::GetAsyncKeyState(VK_LWIN) & 0x8000) != 0) ||
		((::GetAsyncKeyState(VK_RWIN) & 0x8000) != 0) ||
		FInputSystem::IsKeyDown(EKeyCode::LWin) ||
		FInputSystem::IsKeyDown(EKeyCode::RWin);

	HWND hWnd = ::GetActiveWindow();
	const bool bIsAppFocused = (hWnd != nullptr) && (::GetForegroundWindow() == hWnd);

	if (bMouseCaptured && (bWinKeyPressed || !bIsAppFocused))
	{
		SetMouseCapture(false);
	}

	if (!PossessedPawn) return;

	// 2. WASD 이동
	const float Forward = static_cast<float>(bKeyDown[static_cast<int32>(EKeyCode::W)]) -
		static_cast<float>(bKeyDown[static_cast<int32>(EKeyCode::S)]);
	const float Right = static_cast<float>(bKeyDown[static_cast<int32>(EKeyCode::D)]) -
		static_cast<float>(bKeyDown[static_cast<int32>(EKeyCode::A)]);

	const FQuat Rotation = PossessedPawn->GetActorRotation().Quaternion();
	if (Forward != 0.0f) PossessedPawn->AddMovementInput(Rotation.GetForwardVector(), Forward * DeltaTime);
	if (Right != 0.0f) PossessedPawn->AddMovementInput(Rotation.GetRightVector(), Right * DeltaTime);

	// 3. 마우스 시선 회전 (bMouseCaptured 및 포커스 상태일 때만 1인칭 회전 & 중앙 재고정)
	if (bMouseCaptured && bIsAppFocused)
	{
		PossessedPawn->AddControllerYawInput(MouseAxisX * LookSensitivity);
		PossessedPawn->AddControllerPitchInput(MouseAxisY * LookSensitivity);

		// 마우스를 창 중앙으로 재고정하여 화면 밖 탈출 방지
		RECT rc;
		if (::GetClientRect(hWnd, &rc))
		{
			const POINT ClientCenter = { (rc.right - rc.left) / 2, (rc.bottom - rc.top) / 2 };
			POINT ScreenCenter = ClientCenter;
			::ClientToScreen(hWnd, &ScreenCenter);
			::SetCursorPos(ScreenCenter.x, ScreenCenter.y);
			FInputSystem::SyncMousePosition(ClientCenter.x, ClientCenter.y);
		}
	}

	MouseAxisX = MouseAxisY = 0.0f;
}

void APortalPlayerController::InputKey(int32 Key, bool bDown)
{
	if (Key < 0 || Key >= 256) return;

	const bool bPressed = bDown && !bKeyDown[Key];
	bKeyDown[Key] = bDown;

	if (!bPressed) return;

	// 윈도우 키나 Tab 키 입력 시 마우스 잠금 해제 (커서 보임)
	if (Key == static_cast<int32>(EKeyCode::LWin) || Key == static_cast<int32>(EKeyCode::RWin) || Key == static_cast<int32>(EKeyCode::Tab))
	{
		SetMouseCapture(false);
		return;
	}

	// 마우스가 풀려있는 상태에서 뷰포트를 좌클릭하면 다시 게임 모드로 잠금
	if (!bMouseCaptured && Key == static_cast<int32>(EKeyCode::LButton))
	{
		SetMouseCapture(true);
		return;
	}

	APortalPawn* PortalPawn = GetPortalPawn();
	if (!PortalPawn) return;

	// Space 키: 점프
	if (Key == static_cast<int32>(EKeyCode::Space))
	{
		PortalPawn->Jump();
	}
	// 마우스 좌클릭: 주요 액션 (큐브 들고 있으면 던지기, 빈손이면 포탈 발사)
	else if (Key == static_cast<int32>(EKeyCode::LButton) && bMouseCaptured)
	{
		PortalPawn->PrimaryAction();
	}
	// 마우스 우클릭: 포탈 타입 변경 (Blue <-> Orange 모드 토글)
	else if (Key == static_cast<int32>(EKeyCode::RButton) && bMouseCaptured)
	{
		PortalPawn->TogglePortalType();
	}
	// E 키: 상호작용 (큐브 잡기/놓기)
	else if (Key == static_cast<int32>(EKeyCode::E))
	{
		PortalPawn->Interact();
	}
}
