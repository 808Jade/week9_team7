-- ========================================================
-- Scripts/CakePicker.lua
-- 포탈 엔딩 씬(EndingScene.scene):
-- 1. 엔딩 씬 진입 시 엔딩 BGM 자동 재생
-- 2. 시퀀서(EndingCake.seq, 33.0초)가 CakePicker를 천장에서 케이크로 하강시킴 (30.53초 부근 도착)
-- 3. 도착 후 0.5초 대기(31.03초) 후 4개의 집게 관절 메쉬가 오므라들며 케이크 촛불(APointLightActor_0)을 집어서 끔
-- 4. 시퀀스 연출이 모두 끝나면(33.0초) 검은 페이드(Fade 1.0)를 해제하고 원래 뜨던 엔딩 UI(TEST COMPLETE / 탈출 시간 / 시작 화면으로)를 표시
-- ========================================================

-- ========================================================
-- [엔딩 BGM 설정]
-- 엔딩 브금 파일 경로를 아래 EndingBGMPath에 넣어주시면
-- 엔딩 씬 시작과 동시에 자동으로 브금이 재생됩니다!
-- ========================================================
local EndingBGMPath = "Assets/Audio/StillAlive.wav"
local EndingBGMVolume = 1.0
local EndingBGMLoop = false

-- 엔딩 컷씬 시퀀스 총 길이 (33.0초 시점에 엔딩 UI 출력)
local sequenceDuration = 33.0

local comp5 = nil -- 좌측 상단 집게 암 (UStaticMeshComponent_5)
local comp6 = nil -- 좌측 하단 집게 핑거 (UStaticMeshComponent_6)
local comp7 = nil -- 우측 상단 집게 암 (UStaticMeshComponent_7)
local comp8 = nil -- 우측 하단 집게 핑거 (UStaticMeshComponent_8)
local candleLight = nil

local state = "WAITING" -- "WAITING" -> "CLOSING" -> "CLOSED" -> "ENDING_UI"
local elapsedTime = 0.0
local arrivalTime = nil
local closeTimer = 0.0
local closeDuration = 0.55 -- 집게가 닫히는 데 걸리는 시간(초)
local bEndingUIShown = false
local bgmVoiceID = -1

-- 1. 벌려진 초기 상태 (Open: 에디터에서 배치한 정밀 원본 트랜스폼)
-- 좌측: 상단 암은 바깥(-30도), 하단 핑거는 안쪽(+30도)으로 꺾여 벌어진 형태
local open5_Loc = Vector(0.0, -0.7328, -4.5724)
local open5_Rot = Rotator(0.0, 0.0, -30.0)

local open6_Loc = Vector(0.0, -0.5442, -5.6543)
local open6_Rot = Rotator(0.0, 0.0, 30.0)

-- 우측: 상단 암은 바깥(+30도), 하단 핑거는 안쪽(-30도)으로 꺾여 벌어진 형태
local open7_Loc = Vector(0.0, 1.4117, -4.5687)
local open7_Rot = Rotator(0.0, 0.0, 30.0)

local open8_Loc = Vector(0.0, 1.2121, -5.6456)
local open8_Rot = Rotator(0.0, 0.0, -30.0)

-- 2. 오므라든 집게 상태 (Closed: 기구학적으로 관절이 유지되며 핑거 팁이 촛불 중심 Y=0.355로 맞물림)
-- 상단 암은 안쪽으로 회전(-16도 / +16도), 하단 핑거는 중심을 향해 더 안쪽으로 오므라듦(+38도 / -38도)
local close5_Loc = Vector(0.0, -0.5800, -4.6400)
local close5_Rot = Rotator(0.0, 0.0, -16.0)

local close6_Loc = Vector(0.0, -0.2000, -5.7900)
local close6_Rot = Rotator(0.0, 0.0, 38.0)

local close7_Loc = Vector(0.0, 1.2900, -4.6600)
local close7_Rot = Rotator(0.0, 0.0, 16.0)

local close8_Loc = Vector(0.0, 0.9100, -5.8100)
local close8_Rot = Rotator(0.0, 0.0, -38.0)

local function Lerp(a, b, t)
    return a + (b - a) * t
end

local function LerpVector(v1, v2, t)
    return Vector(Lerp(v1.x, v2.x, t), Lerp(v1.y, v2.y, t), Lerp(v1.z, v2.z, t))
end

local function LerpRotator(r1, r2, t)
    return Rotator(Lerp(r1.pitch, r2.pitch, t), Lerp(r1.yaw, r2.yaw, t), Lerp(r1.roll, r2.roll, t))
end

local function ApplyClaw(t)
    if comp5 then
        comp5:SetRelativeLocation(LerpVector(open5_Loc, close5_Loc, t))
        comp5:SetRelativeRotation(LerpRotator(open5_Rot, close5_Rot, t))
    end
    if comp6 then
        comp6:SetRelativeLocation(LerpVector(open6_Loc, close6_Loc, t))
        comp6:SetRelativeRotation(LerpRotator(open6_Rot, close6_Rot, t))
    end
    if comp7 then
        comp7:SetRelativeLocation(LerpVector(open7_Loc, close7_Loc, t))
        comp7:SetRelativeRotation(LerpRotator(open7_Rot, close7_Rot, t))
    end
    if comp8 then
        comp8:SetRelativeLocation(LerpVector(open8_Loc, close8_Loc, t))
        comp8:SetRelativeRotation(LerpRotator(open8_Rot, close8_Rot, t))
    end
end

local function SetCandleVisible(bVisible)
    if not candleLight and obj then
        candleLight = obj:FindActorByName("APointLightActor_0")
    end
    if candleLight then
        candleLight:SetVisibility(bVisible)
        local comps = candleLight:GetComponents()
        if comps then
            for i = 1, #comps do
                if comps[i] then
                    comps[i]:SetVisibility(bVisible)
                end
            end
        end
    end
end

local function ShowEndingScreen()
    if bEndingUIShown then
        return
    end
    bEndingUIShown = true
    state = "ENDING_UI"

    -- 컷씬 시퀀스 정지 및 검은 페이드(Fade 1.0) / 레터박스 해제
    if obj then
        local seqActor = obj:FindActorByName("ASequenceTriggerActor_0")
        if seqActor then
            seqActor:StopSequence()
        end
        obj:SetManualFade(0.0)
        local endCam = obj:FindActorByName("EndingCamera")
        if endCam then
            obj:SetViewTarget(endCam)
        end
    end

    -- 원래 뜨던 성공 엔딩 UI (TEST COMPLETE + 탈출 시간 + [Enter] 시작 화면으로) 표시
    if UIManager then
        UIManager.SetUIMode("Ending")
    end
    print("[CakePicker] Ending cutscene finished -> Showing Ending UI!")
end

function BeginPlay()
    print("[CakePicker] Lua script initialized!")
    comp5 = obj:GetComponent("UStaticMeshComponent_5")
    comp6 = obj:GetComponent("UStaticMeshComponent_6")
    comp7 = obj:GetComponent("UStaticMeshComponent_7")
    comp8 = obj:GetComponent("UStaticMeshComponent_8")

    candleLight = obj:FindActorByName("APointLightActor_0")
    SetCandleVisible(true)

    state = "WAITING"
    elapsedTime = 0.0
    arrivalTime = nil
    closeTimer = 0.0
    bEndingUIShown = false

    -- 시작 시 집게를 확실히 벌려진 상태로 유지
    ApplyClaw(0.0)

    -- 1. 엔딩 BGM 즉시 재생 (파일이 존재하면 자동 로드 및 재생)
    if EndingBGMPath and EndingBGMPath ~= "" and Audio then
        bgmVoiceID = Audio.Play2D(EndingBGMPath, EndingBGMVolume, 1.0, EndingBGMLoop)
        if bgmVoiceID ~= -1 then
            print("[CakePicker] Playing Ending BGM: " .. EndingBGMPath)
        end
    end

    -- 2. 컷씬 재생 중에는 모든 UI 숨김 (None 모드)
    if UIManager then
        UIManager.SetUIMode("None")
    end

    -- 3. 기본 스폰된 DefaultPawn 숨김 및 조작 해제 (컷씬 카메라 방해 방지)
    if obj then
        local pawn = obj:GetPlayerPawn()
        if pawn then
            pawn:SetVisibility(false)
            pawn:SetActorEnableCollision(false)
        end
        obj:UnPossess()
        obj:SetMouseCapture(false)
    end
end

function Tick(dt)
    if bEndingUIShown then
        return
    end

    elapsedTime = elapsedTime + dt

    -- 편의를 위해 Space 키로 엔딩 컷씬 스킵 가능 (진입 후 1초 이후부터)
    if elapsedTime > 1.0 and Input and Input.IsKeyPressed("Space") then
        ApplyClaw(1.0)
        SetCandleVisible(false)
        ShowEndingScreen()
        return
    end

    if state == "WAITING" then
        -- 1. Z축 위치 기반으로 케이크 도착 감지 (케이크 위치 Z <= 1.35)
        local curLoc = obj.Location
        if curLoc and curLoc.z <= 1.35 and arrivalTime == nil then
            arrivalTime = elapsedTime
            print(string.format("[CakePicker] Reached cake at %.2fs (Z=%.2f)! Waiting 0.5s to close...", elapsedTime, curLoc.z))
        end

        -- 2. 조건 충족 검사:
        -- 케이크에 도착한 후 0.5초 경과했거나, 또는 전체 재생 시간이 31.03초 이상일 때
        local bTimeReached = (elapsedTime >= 31.03)
        local bArrivedAndWaited = (arrivalTime ~= nil and (elapsedTime - arrivalTime) >= 0.5)

        if bTimeReached or bArrivedAndWaited then
            state = "CLOSING"
            closeTimer = 0.0
            print(string.format("[CakePicker] Closing claws at %.2fs!", elapsedTime))
        end

    elseif state == "CLOSING" then
        closeTimer = closeTimer + dt
        local alpha = math.min(closeTimer / closeDuration, 1.0)
        -- 부드러운 스무스스텝(SmoothStep) 이징
        local eased = alpha * alpha * (3.0 - 2.0 * alpha)
        ApplyClaw(eased)

        -- 집게가 거의 맞닿을 때(80% 이상) 촛불 끄기
        if alpha >= 0.80 then
            SetCandleVisible(false)
        end

        if alpha >= 1.0 then
            state = "CLOSED"
            SetCandleVisible(false)
            print("[CakePicker] Claw fully closed, candle extinguished!")
        end
    end

    -- 3. 시퀀스 전체 재생 시간(33.0초) 완료 시 엔딩 UI 출력
    if elapsedTime >= sequenceDuration then
        ShowEndingScreen()
    end
end

function EndPlay()
    if bgmVoiceID ~= -1 and Audio then
        Audio.Stop(bgmVoiceID)
        bgmVoiceID = -1
    end
    print("[CakePicker] EndPlay")
end

function OnHotReload()
    BeginPlay()
end
