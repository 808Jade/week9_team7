-- ========================================================
-- CombatConfig.lua
-- 타격감 및 전투 연출 파라미터 설정 테이블
-- C++ 및 Lua에서 이 테이블을 참조하여 타격감 수치를 실시간 튜닝합니다.
-- ========================================================

local CombatConfig = {
    -- 1. 근접 공격 (Melee Swing)
    Melee = {
        Range = 2.0,            -- 근접 공격 사거리 (m)
        Cooldown = 0.4,         -- 공격 후 재공격 대기 시간 (초)
    },

    -- 2. 히트스탑 (Hit Stop - 피격 시 찰나의 역경직)
    HitStop = {
        Duration = 0.06,        -- 역경직 지속 시간 (초)
        TimeDilation = 0.02,    -- 역경직 시 전역 시간 배율 (0에 가까울수록 순간 정지)
    },

    -- 3. 슬로모션 (Slomo - H키 입력 시 발동)
    Slomo = {
        Duration = 2.0,         -- 슬로모션 지속 시간 (초)
        TimeDilation = 0.25,    -- 슬로모션 시간 배율 (0.25배속)
    },

    -- 4. 넉백 물리 (Knockback Physics)
    Knockback = {
        HorizontalImpulse = 4.0, -- 수평 넉백 속도 (m/s)
        VerticalImpulse = 0.8,   -- 수직 살짝 띄우는 속도 (m/s)
        Gravity = 9.8,           -- 중력 가속도 (m/s^2)
        GroundFriction = 8.0,    -- 착지 후 지면 마찰 감속 계수
    },

    -- 5. 히트 스쿼시 (Hit Squash & Stretch - 입체적 찌그러짐)
    Squash = {
        MinScale = 0.65,        -- 피격 정면 압축 최소 배율
        MaxScale = 1.25,        -- 수직/측면 팽창 최대 배율 (부피 보존)
        UpScale = 1.15,         -- 상단(Z축) 팽창 배율
        RecoverySpeed = 6.0,    -- 원래 크기(1.0) 복원 속도
    }
}

return CombatConfig
