-- ========================================================
-- Scripts/StartSeq.lua
-- PortalFlingChamber 시작 연출 및 시퀀스-플레이어 빙의 제어 스크립트
--
-- 동작 흐름:
-- 1. [BeginPlay]: 시작 UI(TitleCanvas) 상태 유지, 플레이어 UnPossess 및 마우스 캡처 해제
-- 2. [Tick - WAITING_ENTER]: 시작 UI에서 Enter 키 입력 감지 시 StartSeq 시퀀스 재생 시작
-- 3. [Tick - PLAYING_CUTSCENE]: 18초 동안 카메라 컷씬 재생 (HUD 및 크로스헤어 숨김 유지)
--    (편의를 위해 Space 키로 컷씬 스킵 가능)
-- 4. [Tick - IN_GAME]: 18초 완료 후 PortalPawn에 Possess, InGame HUD/크로스헤어 활성화, 마우스 캡처 켜기
-- ========================================================

local portalPawn = nil
local state = "WAITING_ENTER" -- "WAITING_ENTER" -> "PLAYING_CUTSCENE" -> "IN_GAME"
local timer = 0.0
local duration = 18.03 -- StartSeq.seq 재생 길이 (~18초)

function BeginPlay()
    print("[StartSeq] Initialized on " .. (obj and obj:GetName() or "Unknown"))
    
    -- 1. 씬 내의 PortalPawn 탐색 및 캐싱
    if obj then
        portalPawn = obj:FindActorByName("APortalPawn_1")
        if not portalPawn then
            portalPawn = obj:FindActorByName("APortalPawn")
        end
        if not portalPawn then
            portalPawn = obj:GetPlayerPawn()
        end
    end
    
    if portalPawn then
        print("[StartSeq] Found target pawn: " .. portalPawn:GetName())
    else
        print("[StartSeq] Target pawn not yet found in BeginPlay (will retry on Enter)")
    end

    -- 2. 시작 시점에는 폰 빙의를 해제하여 움직임/시점 조작 방지
    if obj and obj.UnPossess then
        obj:UnPossess()
    end
    if obj and obj.SetMouseCapture then
        obj:SetMouseCapture(false)
    end

    state = "WAITING_ENTER"
    timer = 0.0
end

function Tick(dt)
    if state == "WAITING_ENTER" then
        -- 1. 시작 UI에서 Enter 키가 눌렸는지 감지
        local enterPressed = false

        if Input and Input.IsKeyPressed and Input.IsKeyPressed("Enter") then
            enterPressed = true
        end

        -- TitleCanvas에서 Enter를 받아 UIMode가 InGame으로 바뀌려 한 경우도 감지
        if UIManager and UIManager.GetUIMode and UIManager.GetUIMode() == "InGame" then
            enterPressed = true
        end

        if enterPressed then
            print("[StartSeq] Enter detected! Starting intro cutscene (" .. tostring(duration) .. "s)...")

            -- ASequenceTriggerActor의 시퀀스 재생
            if obj and obj.PlaySequence then
                obj:PlaySequence()
            end

            -- 플레이어 조작 차단 및 마우스 캡처 해제 유지
            if obj and obj.UnPossess then
                obj:UnPossess()
            end
            if obj and obj.SetMouseCapture then
                obj:SetMouseCapture(false)
            end

            -- 컷씬 동안 인게임 HUD, 크로스헤어, 타이머 숨김
            if UIManager then
                if UIManager.GetHUD and UIManager.GetHUD() then
                    UIManager.GetHUD():SetCanvasVisibility(false)
                end
                if UIManager.SetCrosshairVisibility then
                    UIManager.SetCrosshairVisibility(false)
                end
                if UIManager.StopTimer then
                    UIManager.StopTimer()
                end
                if UIManager.ResetTimer then
                    UIManager.ResetTimer()
                end
            end

            state = "PLAYING_CUTSCENE"
            timer = 0.0
        end

    elseif state == "PLAYING_CUTSCENE" then
        timer = timer + dt

        -- 컷씬 재생 중 인게임 HUD와 크로스헤어가 뜨지 않도록 유지
        if UIManager then
            if UIManager.GetHUD and UIManager.GetHUD() then
                UIManager.GetHUD():SetCanvasVisibility(false)
            end
            if UIManager.SetCrosshairVisibility then
                UIManager.SetCrosshairVisibility(false)
            end
        end

        -- 마우스 캡처 계속 해제
        if obj and obj.SetMouseCapture then
            obj:SetMouseCapture(false)
        end

        local isFinished = false

        -- 1) 18초(duration) 경과 시 완료
        if timer >= duration then
            isFinished = true
        end

        -- 2) 시퀀스 플레이어가 재생 완료되었는지 체크
        if timer > 1.0 and obj and obj.IsSequencePlaying and not obj:IsSequencePlaying() then
            isFinished = true
        end

        -- 3) 편의 기능: 1초 후 Space 키를 누르면 컷씬 즉시 스킵 가능
        if timer > 1.0 and Input and Input.IsKeyPressed and Input.IsKeyPressed("Space") then
            print("[StartSeq] Cutscene skipped by player.")
            if obj and obj.StopSequence then
                obj:StopSequence()
            end
            isFinished = true
        end

        if isFinished then
            print("[StartSeq] Cutscene finished! Possessing PortalPawn...")

            -- 1. PortalPawn 재탐색 (필요 시)
            if not portalPawn or not portalPawn:IsValid() then
                portalPawn = obj:FindActorByName("APortalPawn_1") or obj:FindActorByName("APortalPawn")
            end

            -- 2. PortalPawn에 빙의 (Possess)
            if portalPawn then
                obj:Possess(portalPawn)
                print("[StartSeq] Possessed: " .. portalPawn:GetName())
            else
                print("[StartSeq] Error: PortalPawn not found for possession!")
            end

            -- 3. InGame UI 및 타이머, 크로스헤어 활성화
            if UIManager then
                UIManager.SetUIMode("InGame")
                if UIManager.GetHUD and UIManager.GetHUD() then
                    UIManager.GetHUD():SetCanvasVisibility(true)
                end
                if UIManager.SetCrosshairVisibility then
                    UIManager.SetCrosshairVisibility(true)
                end
                if UIManager.ResetTimer then
                    UIManager.ResetTimer()
                end
                if UIManager.StartTimer then
                    UIManager.StartTimer()
                end
            end

            -- 4. 마우스 캡처 활성화 (게임플레이 컨트롤 시작)
            if obj and obj.SetMouseCapture then
                obj:SetMouseCapture(true)
            end

            state = "IN_GAME"
            print("[StartSeq] Game started successfully! Player is now in control.")
        end

    elseif state == "IN_GAME" then
        -- 게임 진행 중: 플레이어가 조작
    end
end

function EndPlay()
    print("[StartSeq] EndPlay")
end
