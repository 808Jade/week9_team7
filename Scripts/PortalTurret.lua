-- ========================================================
-- PortalTurret.lua
-- 포탈 터렛(APortalTurretActor)의 루아 스크립트 컨트롤러
-- CombatConfig의 파라미터를 참조하고 피격(OnHit), 상태 전이 등을 제어
-- ========================================================

local CombatConfig = require("CombatConfig")

function BeginPlay()
    local name = (this and this.GetName and this:GetName()) or (obj and obj.Name) or "PortalTurret"
    print(string.format("[PortalTurret.lua] Initialized on %s", name))
end

function Tick(dt)
    -- 필요 시 터렛 상태 추적 및 추가 로직 수행
end

function OnHit(OtherActor)
    print("[PortalTurret.lua] Turret received melee strike! Triggering combat response.")
    -- 피격 시 사운드/대사/이펙트 등 루아 확장 가능
end

function OnHotReload()
    print("[PortalTurret.lua] Hot Reloaded!")
end
