-- ============================================================================
-- Scripts/TestController.lua
-- 테스트 스크립트:
-- 1. WASD 키 입력으로 큐브 액터 이동 (Input.IsKeyDown + this:AddLocation)
-- 2. 마우스 좌클릭 시 ScoreText 액터를 찾아 텍스트 숫자를 0부터 1씩 증가 (Input.IsMousePressed + textActor:SetText)
-- 3. 스페이스바 입력 시 리플렉션 함수 호출 테스트 (moveComp:StopMovementImmediately)
-- ============================================================================

local count = 0
local textActor = nil
local moveComp = nil

function BeginPlay()
    print("==================================================")
    print("[TestController] BeginPlay Initialized!")
    print("==================================================")

    -- 1. 씬에서 "ScoreText"라는 이름의 텍스트 액터 찾기
    textActor = this:FindActorByName("ScoreText")
    if textActor then
        textActor:SetText("Count: 0")
        print("[TestController] Successfully found 'ScoreText' Actor!")
    else
        print("[TestController] Warning: Could not find 'ScoreText' Actor.")
    end

    -- 2. 현재 큐브에 부착된 MovementComponent 가져오기
    moveComp = this:GetComponent("ProjectileMovementComponent")
    if not moveComp then
        moveComp = this:GetComponent("MovementComponent")
    end

    if moveComp then
        print("[TestController] Found MovementComponent: " .. moveComp:GetClassName())
    else
        print("[TestController] Notice: No MovementComponent attached to Cube.")
    end
end

function Tick(dt)
    -- ----------------------------------------------------
    -- [1] WASD 이동 로직
    -- ----------------------------------------------------
    local moveSpeed = 10.0
    local moveDelta = Vector(0, 0, 0)

    -- W/S: X축 (전진/후진)
    if Input.IsKeyDown("W") then
        moveDelta.X = moveDelta.X + moveSpeed * dt
    end
    if Input.IsKeyDown("S") then
        moveDelta.X = moveDelta.X - moveSpeed * dt
    end

    -- A/D: Y축 (좌/우)
    if Input.IsKeyDown("D") then
        moveDelta.Y = moveDelta.Y + moveSpeed * dt
    end
    if Input.IsKeyDown("A") then
        moveDelta.Y = moveDelta.Y - moveSpeed * dt
    end

    -- 이동량이 있으면 액터 위치 갱신
    if moveDelta:LengthSquared() > 0 then
        this:AddLocation(moveDelta)
    end

    -- ----------------------------------------------------
    -- [2] 마우스 좌클릭 시: 카운트 1씩 증가 및 텍스트 갱신
    -- ----------------------------------------------------
    if Input.IsMousePressed("Left") then
        count = count + 2
        local newText = "Count: " .. count
        print("[TestController] Mouse Left Clicked! " .. newText)

        if textActor then
            -- 액터 바인딩 함수로 텍스트 변경
            textActor:SetText(newText)

            -- 또는 컴포넌트 동적 프로퍼티(FProperty) 리플렉션으로도 변경 가능:
            local textComp = textActor:GetComponent("TextRenderComponent")
            if textComp then
                textComp.Text = newText
            end
        end
    end

    -- ----------------------------------------------------
    -- [3] 스페이스바 입력 시: UFUNCTION 리플렉션 함수 호출 테스트
    -- ----------------------------------------------------
    if Input.IsKeyPressed("Space") then
        if moveComp then
            print("[TestController] Calling UFUNCTION 'StopMovementImmediately()' via Reflection...")
            moveComp:StopMovementImmediately()
            print("[TestController] >>> StopMovementImmediately() executed successfully! <<<")
        else
            print("[TestController] No moveComp available to test.")
        end
    end
end

function EndPlay()
    print("[TestController] EndPlay called.")
end
