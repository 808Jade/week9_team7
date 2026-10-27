-- ========================================================
-- Portal Crosshair & Reserve Cube Summoner Controller
-- 포탈 폰(APortalPawn)에 부착되는 크로스헤어 및 예비 큐브 소환 스크립트
-- ========================================================

local hasBlue = false
local hasOrange = false

-- ========================================================
-- [예비 큐브(Reserve Cube) 관리 및 소환 시스템]
-- ========================================================
local reserveCubes = {}
local nextReserveIndex = 1
local RESERVE_FAR_LOCATION = Vector.new(500.0, 500.0, 50.0) -- 씬 먼 곳 좌표
local DEFAULT_RESERVE_COUNT = 5                              -- 예비 큐브 기본 보관 수량

-- 예비 큐브 초기화 함수 (씬에 이미 존재하는 큐브 탐색 및 미배치 시 먼 곳에 생성)
local function InitReserveCubes()
    reserveCubes = {}
    nextReserveIndex = 1

    -- 1. 씬에 미리 배치된 예비 큐브("ReservePortalCube" 또는 "ReserveCube")가 있는지 탐색
    if this and this.FindActorsWithPrefix then
        local found = this:FindActorsWithPrefix("ReservePortalCube")
        if found and #found > 0 then
            for i = 1, #found do
                local cube = found[i]
                if cube and cube:IsValid() then
                    table.insert(reserveCubes, cube)
                    -- 먼 곳에 보관 중일 때는 물리 시뮬레이션 일시정지 및 비가시화
                    local proj = cube:GetComponent("ProjectileComponent")
                    if proj then
                        proj.bSimulationEnabled = false
                        proj.Velocity = Vector.new(0, 0, 0)
                    end
                    cube:SetVisibility(false)
                end
            end
        end

        local found2 = this:FindActorsWithPrefix("ReserveCube")
        if found2 and #found2 > 0 then
            for i = 1, #found2 do
                local cube = found2[i]
                if cube and cube:IsValid() then
                    table.insert(reserveCubes, cube)
                    local proj = cube:GetComponent("ProjectileComponent")
                    if proj then
                        proj.bSimulationEnabled = false
                        proj.Velocity = Vector.new(0, 0, 0)
                    end
                    cube:SetVisibility(false)
                end
            end
        end
    end

    -- 2. 만약 씬에 예비 큐브가 없다면(다른 씬 등), 씬 먼 곳에 지정된 개수만큼 동적 생성하여 보관
    if #reserveCubes < DEFAULT_RESERVE_COUNT then
        local needed = DEFAULT_RESERVE_COUNT - #reserveCubes
        for i = 1, needed do
            local spawnPos = Vector.new(RESERVE_FAR_LOCATION.x, RESERVE_FAR_LOCATION.y + (i * 5.0), RESERVE_FAR_LOCATION.z)
            local cube = nil
            if this and this.SpawnActor then
                cube = this:SpawnActor("PortalCubeActor", spawnPos, Rotator.new(0, 0, 0))
            elseif SpawnActor then
                cube = SpawnActor("PortalCubeActor", spawnPos, Rotator.new(0, 0, 0))
            end

            if cube and cube:IsValid() then
                local proj = cube:GetComponent("ProjectileComponent")
                if proj then
                    proj.bSimulationEnabled = false
                    proj.Velocity = Vector.new(0, 0, 0)
                end
                cube:SetVisibility(false)
                table.insert(reserveCubes, cube)
            end
        end
    end

    print(string.format("[ReserveCube] 예비 큐브 %d개가 씬 먼 곳에 준비되었습니다.", #reserveCubes))
end

-- R키 입력 시 플레이어 옆으로 큐브 소환
local function SummonReserveCube()
    if not this or not this:IsValid() then
        return
    end

    local playerLoc = this:GetActorLocation()
    local rightVec = this:GetActorRightVector()
    local fwdVec = this:GetActorForwardVector()

    -- 소환 위치 계산: 플레이어 오른쪽 1.5m, 앞쪽 0.5m, 높이 +0.2m (플레이어 콜라이더와 겹치지 않는 옆 공간)
    local targetLoc = playerLoc + (rightVec * 1.5) + (fwdVec * 0.5) + Vector.new(0.0, 0.0, 0.2)
    local targetRot = this:GetActorRotation()

    local cubeToSummon = nil

    -- 1. 보관 중인 예비 큐브 목록에서 하나 가져오기
    while nextReserveIndex <= #reserveCubes do
        local candidate = reserveCubes[nextReserveIndex]
        nextReserveIndex = nextReserveIndex + 1
        if candidate and candidate:IsValid() then
            cubeToSummon = candidate
            break
        end
    end

    -- 2. 예비 큐브가 모두 소진되었을 경우, 추가 큐브를 즉석 생성하여 소환
    if not cubeToSummon then
        print("[ReserveCube] 모든 예비 큐브가 소환되었습니다. 새로운 큐브를 추가 생성하여 소환합니다.")
        if this and this.SpawnActor then
            cubeToSummon = this:SpawnActor("PortalCubeActor", targetLoc, targetRot)
        elseif SpawnActor then
            cubeToSummon = SpawnActor("PortalCubeActor", targetLoc, targetRot)
        end
    end

    -- 3. 큐브 활성화 및 물리 적용
    if cubeToSummon and cubeToSummon:IsValid() then
        cubeToSummon:SetLocation(targetLoc)
        cubeToSummon:SetRotation(targetRot)
        cubeToSummon:SetVisibility(true)
        cubeToSummon:SetActorEnableCollision(true)

        -- 발사체 물리 컴포넌트 활성화 및 속도 0으로 초기화
        local proj = cubeToSummon:GetComponent("ProjectileComponent")
        if proj then
            proj.bSimulationEnabled = true
            proj.Velocity = Vector.new(0.0, 0.0, 0.0)
        end
        cubeToSummon:SetVelocity(Vector.new(0.0, 0.0, 0.0))

        local remaining = math.max(0, #reserveCubes - nextReserveIndex + 1)
        print(string.format("[ReserveCube] 큐브 '%s'가 플레이어 옆에 소환되었습니다! (남은 예비 큐브: %d개)", cubeToSummon:GetName(), remaining))
    else
        print("[ReserveCube] 큐브 소환 실패!")
    end
end

-- ========================================================
-- [엔진 생명주기 이벤트 (Lifecycle Events)]
-- ========================================================

function BeginPlay()
    hasBlue = false
    hasOrange = false
    UpdateCrosshair()
    InitReserveCubes()
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
    -- UI 모드가 InGame일 때만 포탈 상태 및 입력 갱신
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

    -- 4. R 키 감지: 플레이어 옆에 예비 큐브 소환
    if Input.IsKeyPressed("R") then
        SummonReserveCube()
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
    InitReserveCubes()
end
