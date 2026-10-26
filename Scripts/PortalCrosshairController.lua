-- ========================================================
-- Portal Crosshair State Controller
-- 포탈 폰(APortalPawn)에 부착되는 크로스헤어 상태 관리 스크립트
-- ========================================================

local hasBlue = false
local hasOrange = false

function BeginPlay()
    hasBlue = false
    hasOrange = false
    UpdateCrosshair()
    print("[PortalCrosshair] 스크립트 시작: 상태 초기화 (crosshair_unfilled)")
end

function UpdateCrosshair()
    if hasBlue and hasOrange then
        UIManager.SetCrosshairState("both")
        print("[PortalCrosshair] 크로스헤어 변경: both (블루 & 오렌지 모두 소환됨)")
    elseif hasBlue then
        UIManager.SetCrosshairState("blue")
        print("[PortalCrosshair] 크로스헤어 변경: blue (블루 포탈만 소환됨)")
    elseif hasOrange then
        UIManager.SetCrosshairState("orange")
        print("[PortalCrosshair] 크로스헤어 변경: orange (오렌지 포탈만 소환됨)")
    else
        UIManager.SetCrosshairState("unfilled")
        print("[PortalCrosshair] 크로스헤어 변경: unfilled (소환된 포탈 없음)")
    end
end

function Tick(dt)
    -- UI 모드가 InGame일 때만 포탈 상태 갱신
    if UIManager and UIManager.GetUIMode and UIManager.GetUIMode() ~= "InGame" then
        return
    end

    local stateChanged = false

    -- 1. C++ 엔진 실제 포탈 배치(Placed) 상태와 자동 동기화
    if this and this.IsBluePortalPlaced and this.IsOrangePortalPlaced then
        local placedBlue = this:IsBluePortalPlaced()
        local placedOrange = this:IsOrangePortalPlaced()

        if placedBlue and not hasBlue then
            hasBlue = true
            stateChanged = true
        end

        if placedOrange and not hasOrange then
            hasOrange = true
            stateChanged = true
        end
    end

    -- 2. 첫 마우스 좌클릭 감지 (블루 포탈 소환)
    if Input.IsMousePressed("Left") then
        if not hasBlue then
            hasBlue = true
            stateChanged = true
        end
    end

    -- 3. 첫 마우스 우클릭 감지 (오렌지 포탈 소환)
    if Input.IsMousePressed("Right") then
        if not hasOrange then
            hasOrange = true
            stateChanged = true
        end
    end

    -- 상태가 변했을 때만 크로스헤어 텍스처 갱신
    if stateChanged then
        UpdateCrosshair()
    end
end

-- 리셋 함수 (씬 리로드 또는 리스폰 시 호출 가능)
function ResetPortalCrosshair()
    hasBlue = false
    hasOrange = false
    if this and this.ResetPortals then
        this:ResetPortals()
    end
    UpdateCrosshair()
end

function EndPlay()
end

function OnOverlap(OtherActor)
end

function OnHit(OtherActor)
end

function OnHotReload()
    UpdateCrosshair()
end
