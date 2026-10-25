-- Coroutine: StartCoroutine(fn) / StopCoroutine(handle) / StopAllCoroutines() 
--		      wait(sec) / wait_frames(n) / wait_until(fn)

function BeginPlay()
print("[BeginPlay] " .. obj.UUID)
obj:PrintLocation()
end

function EndPlay()
print("[EndPlay] " .. obj.UUID)
obj:PrintLocation()
end

function OnOverlap(OtherActor)
OtherActor:PrintLocation();
end

function Tick(dt)
obj.Location = obj.Location + obj.Velocity * dt
--obj:PrintLocation()
end

function OnHit(OtherActor)
end

-- Hot Reload
function OnHotReload()
end
