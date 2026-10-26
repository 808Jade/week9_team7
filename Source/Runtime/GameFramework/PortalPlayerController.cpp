#include "EnginePCH.h"
#include "GameFramework/PortalPlayerController.h"
#include "GameFramework/PortalPawn.h"
#include "GameFramework/Actor/UIManagerActor.h"
#include "GameFramework/Actor/HUDCanvasActor.h"
#include "Input/InputSystem.h"
#include "Engine/World.h"

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

	// UIManagerActor가 없다면 자동 생성 보장
	if (!AUIManagerActor::Get() && GetWorld())
	{
		GetWorld()->SpawnActor<AUIManagerActor>();
	}

	// UI 모드가 InGame일 때만 마우스 잠금, Title/Pause/Ending에서는 마우스 해제
	if (AUIManagerActor::Get() && AUIManagerActor::Get()->GetCurrentUIMode() != EUIMode::InGame)
	{
		SetMouseCapture(false);
	}
	else
	{
		SetMouseCapture(true);
	}
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
	// 0. UI 모드 (Title, Pause, Ending 등)일 때는 플레이어 시선 회전 및 이동 전면 차단
	if (AUIManagerActor::Get() && AUIManagerActor::Get()->GetCurrentUIMode() != EUIMode::InGame)
	{
		if (bMouseCaptured)
		{
			SetMouseCapture(false);
		}
		MouseAxisX = MouseAxisY = 0.0f;
		return;
	}

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

	APortalPawn* PortalPawn = GetPortalPawn();
	if (!PortalPawn) return;

	// 2. 웅크리기(Ctrl) / 달리기(Shift) 입력 갱신
	const bool bSprint = bKeyDown[static_cast<int32>(EKeyCode::LShift)] ||
		bKeyDown[static_cast<int32>(EKeyCode::RShift)] ||
		bKeyDown[static_cast<int32>(EKeyCode::Shift)];
	const bool bCrouch = bKeyDown[static_cast<int32>(EKeyCode::LControl)] ||
		bKeyDown[static_cast<int32>(EKeyCode::RControl)] ||
		bKeyDown[static_cast<int32>(EKeyCode::Control)];

	PortalPawn->SetSprinting(bSprint);
	PortalPawn->SetCrouching(bCrouch);

	// 3. WASD 수평 이동 (카메라 Pitch 무관하게 수평 평면 기준 방향 벡터)
	// WASD 또는 방향키
	const auto Down = [this](EKeyCode A, EKeyCode B)
	{
		return static_cast<float>(bKeyDown[static_cast<int32>(A)] || bKeyDown[static_cast<int32>(B)]);
	};
	const float Forward = Down(EKeyCode::W, EKeyCode::Up)    - Down(EKeyCode::S, EKeyCode::Down);
	const float Right   = Down(EKeyCode::D, EKeyCode::Right) - Down(EKeyCode::A, EKeyCode::Left);

	const FRotator ActorRot = PortalPawn->GetActorRotation();
	const FQuat YawQuat = FRotator(0.0f, ActorRot.Yaw, 0.0f).Quaternion();
	if (Forward != 0.0f) PortalPawn->AddMovementInput(YawQuat.GetForwardVector(), Forward);
	if (Right != 0.0f) PortalPawn->AddMovementInput(YawQuat.GetRightVector(), Right);

	// 4. 마우스 시선 회전 (bMouseCaptured 및 포커스 상태일 때만 1인칭 회전 & 화면 중앙 재고정)
	if (bMouseCaptured && bIsAppFocused)
	{
		PortalPawn->AddControllerYawInput(MouseAxisX * LookSensitivity);
		PortalPawn->AddControllerPitchInput(MouseAxisY * LookSensitivity);

		// 마우스를 게임 화면 중앙으로 재고정하여 화면 밖 탈출 방지
		// 에디터 PIE는 창 중앙이 PIE 뷰포트 밖(디테일 패널 등)일 수 있으므로
		// 에디터가 지정한 PIE 뷰 중앙(스크린 좌표)을 우선 사용한다.
		POINT ScreenCenter{};
		int32 LockX = 0, LockY = 0;
		bool bHasCenter = false;
		if (FInputSystem::GetMouseLockCenter(LockX, LockY))
		{
			ScreenCenter = { LockX, LockY };
			bHasCenter = true;
		}
		else
		{
			RECT rc;
			if (::GetClientRect(hWnd, &rc))
			{
				ScreenCenter = { (rc.right - rc.left) / 2, (rc.bottom - rc.top) / 2 };
				::ClientToScreen(hWnd, &ScreenCenter);
				bHasCenter = true;
			}
		}
		if (bHasCenter)
		{
			::SetCursorPos(ScreenCenter.x, ScreenCenter.y);
			POINT ClientCenter = ScreenCenter;
			::ScreenToClient(hWnd, &ClientCenter);
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

	// UI 모드 (Title, Pause, Ending)일 때는 플레이어 액션 차단
	if (AUIManagerActor::Get() && AUIManagerActor::Get()->GetCurrentUIMode() != EUIMode::InGame)
	{
		return;
	}

	// 1. ESC 키: HUDCanvasActor의 ImGui 설정창에서 처리하므로 Controller에서는 무시
	if (Key == static_cast<int32>(EKeyCode::Escape))
	{
		return;
	}

	// 2. 윈도우 키나 Tab 키 입력 시 마우스 잠금 해제 (커서 보임)
	if (Key == static_cast<int32>(EKeyCode::LWin) || Key == static_cast<int32>(EKeyCode::RWin) || Key == static_cast<int32>(EKeyCode::Tab))
	{
		SetMouseCapture(false);
		return;
	}

	// 3. 마우스가 풀려있는 상태에서 뷰포트를 좌클릭하면 다시 게임 모드로 잠금 (설정창이 열려있을 때는 잠금하지 않음)
	if (!bMouseCaptured && Key == static_cast<int32>(EKeyCode::LButton))
	{
		if (AUIManagerActor::Get() && AUIManagerActor::Get()->GetHUDCanvas() && AUIManagerActor::Get()->GetHUDCanvas()->IsSettingsWindowVisible())
		{
			return;
		}
		SetMouseCapture(true);
		return;
	}

	APortalPawn* PortalPawn = GetPortalPawn();
	if (!PortalPawn) return;

	// 4. Space 키: 점프
	if (Key == static_cast<int32>(EKeyCode::Space))
	{
		PortalPawn->Jump();
	}
	// 5. 마우스 좌클릭: 주요 액션 (큐브 들고 있으면 던지기, 빈손이면 포탈 발사)
	else if (Key == static_cast<int32>(EKeyCode::LButton) && bMouseCaptured)
	{
		PortalPawn->PrimaryAction();
	}
	// 6. 마우스 우클릭: 주황 포탈 발사
	else if (Key == static_cast<int32>(EKeyCode::RButton) && bMouseCaptured)
	{
		PortalPawn->SecondaryAction();
	}
	// 7. E 키: 상호작용 (큐브 잡기/놓기)
	else if (Key == static_cast<int32>(EKeyCode::E))
	{
		PortalPawn->Interact();
	}
	// 8. Z 키 또는 마우스 휠 클릭(MButton): 줌(Zoom) 기능 토글
	else if (Key == static_cast<int32>(EKeyCode::Z) || Key == static_cast<int32>(EKeyCode::MButton))
	{
		PortalPawn->ToggleZoom();
	}
}
