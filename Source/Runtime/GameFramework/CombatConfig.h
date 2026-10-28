#pragma once
#include "Core/Types.h"

struct FCombatConfig
{
    // 1. 근접 공격 (Melee Swing)
    float MeleeRange = 2.0f;
    float MeleeCooldown = 0.4f;

    // 2. 히트스탑 (Hit Stop)
    float HitStopDuration = 0.06f;
    float HitStopTimeDilation = 0.02f;

    // 3. 슬로모션 (Slomo)
    float SlomoDuration = 2.0f;
    float SlomoTimeDilation = 0.25f;

    // 4. 넉백 물리 (Knockback Physics)
    float KnockbackHorizontal = 4.0f;
    float KnockbackVertical = 0.8f;
    float Gravity = 9.8f;
    float GroundFriction = 8.0f;

    // 5. 히트 스쿼시 (Hit Squash & Stretch)
    float SquashMinScale = 0.65f;
    float SquashMaxScale = 1.25f;
    float SquashUpScale = 1.15f;
    float SquashRecoverySpeed = 6.0f;

    static FCombatConfig& Get();
    void LoadFromLua();
};
