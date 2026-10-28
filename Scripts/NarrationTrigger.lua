-- ========================================================
-- Scripts/NarrationTrigger.lua
-- 씬의 TriggerActor 또는 콜라이더 액터에 부착하여 사용하는 나레이션 재생 스크립트
--
-- [동작 원리]
-- 1. 플레이어(PortalPawn)가 트리거 박스 영역에 닿으면 OnOverlap(OtherActor)이 자동 호출됩니다.
-- 2. 해당 액터 이름(obj:GetName())에 대응하는 나레이션 데이터가 있다면 자동 매핑되고,
--    없다면 기본값(DefaultNarration)을 사용하여 음성과 자막을 출력합니다.
-- 3. bTriggerOnce = true 인 경우 1회만 재생됩니다.
-- ========================================================

-- 1. 액터 이름별 나레이션 대사 & 음성 매핑 테이블
-- 씬에서 TriggerActor의 이름을 아래 Key와 일치시키면 해당 음성과 자막이 출력됩니다.
local NarrationDatabase = {
    ["NoComplete"] = {
        sound = "Assets/Audio/NoComplete.wav",
        text = "터렛이 작동을 안하네요. 이렇게 쉽게 재현이 가능한 버그가 있다는건, \'미완성\' 인거죠",
        duration = 7.0
    },
    ["Curious"] = {
        sound = "Assets/Audio/Curious.wav",
        text = "본인이 하지 않은 부분이라도 궁금해하셔야 해요.",
        duration = 3.0
    },
    ["Gravity"] = {
        sound = "Assets/Audio/Gravity.wav",
        text = "이 스테이지는 중력 가속도를 이용하면 탈출 할 수 있을 겁니다.",
        duration = 3.0
    },
    ["Find"] = {
        sound = "Assets/Audio/Find.wav",
        text = "탈출 방법은 여러분이 직접 한번 찾아보세요.",
        duration = 3.0
    },
    ["BeforeAllThing"] = {
        sound = "Assets/Audio/BeforeAllThing.wav",
        text = "이번 스테이지 클리어는 선택 사항입니다. 물론 전 기수는 다 했어요.",
        duration = 5.0
    },
    ["Welcome"] = {
        sound = "Assets/Audio/Welcome.wav",
        text = "크래프톤 정글 게임 테크랩 350기에 합류하신 걸 환영합니다.",
        duration = 4.0
    },
    ["SoftSkill"] = {
        sound = "Assets/Audio/SoftSkill.wav",
        text = "포탈 퍼즐을 잘 푸는 것도 중요하지만, \"소프트 스킬\"을 통해 협력할 줄 알아야합니다.",
        duration = 8.0
    },
    ["ManyQuestions"] = {
        sound = "Assets/Audio/ManyQuestions.wav",
        text = "물어보고 싶은 게 많은데, 시간이 없으니까 여기까지 하도록 하죠. 예, 수고하셨습니다",
        duration = 6.0
    }
}

-- 2. 액터 이름이 데이터베이스에 없을 때 사용할 기본 설정 (자유롭게 파일 경로와 텍스트 수정 가능)
-- sound = nil 이면 음성 없이 자막만 출력 (예전엔 포탈건 발사음이 기본값이라 엉뚱한 포탈 소리가 났음)
local DefaultNarration = {
    sound = nil,
    text = "Aperture Science 실험 챔버에 진입하셨습니다.",
    duration = 4.0
}

local bTriggerOnce = true
local bHasTriggered = false
local currentVoiceID = -1

function BeginPlay()
    bHasTriggered = false
    local actorName = obj and obj:GetName() or "Unknown"
    print("[NarrationTrigger] Initialized on " .. actorName)
end

function OnOverlap(OtherActor)
    -- 이미 1회 실행된 경우 무시
    if bTriggerOnce and bHasTriggered then
        return
    end

    local actorName = obj and obj:GetName() or ""
    local data = NarrationDatabase[actorName] or DefaultNarration

    bHasTriggered = true
    print("[NarrationTrigger] Triggered on [" .. actorName .. "] -> " .. data.text)

    -- 1. 이전 나레이션 음성 중복 방지 (선택적으로 이전 음성 중지)
    if currentVoiceID ~= -1 and Audio and Audio.Stop then
        Audio.Stop(currentVoiceID)
    end

    -- 2. 2D 음성 재생 (플레이어 귀에 깨끗하게 들리는 방송/글라도스 음성)
    if data.sound then
        if Audio and Audio.Play2D then
            currentVoiceID = Audio.Play2D(data.sound, 1.0)
        elseif obj and obj.PlaySound2D then
            currentVoiceID = obj:PlaySound2D(data.sound, 1.0)
        end
    end

    -- 3. 화면 하단 자막(Subtitle) 표시 (지정된 duration 초 동안 표시 후 자동 페이드/숨김)
    if UIManager and UIManager.ShowSubtitle then
        UIManager.ShowSubtitle(data.text, data.duration)
    elseif UIManager and UIManager.GetHUD and UIManager.GetHUD() then
        UIManager.GetHUD():ShowSubtitle(data.text, data.duration)
    elseif obj and obj.ShowSubtitle then
        obj:ShowSubtitle(data.text, data.duration)
    end
end
