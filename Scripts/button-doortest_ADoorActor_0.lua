-- Coroutine: StartCoroutine(fn) / StopCoroutine(handle) / StopAllCoroutines() 
--		      wait(sec) / wait_frames(n) / wait_until(fn)

function BeginPlay()
    -- 1. 씬에 배치된 버튼과 도어 액터 검색
    local button = obj:FindActorByName("AWeightedButtonActor_0")
    local door = obj:FindActorByName("ADoorActor_0")
    if button and door then
        -- 2. (선택) 버튼으로만 열리게 하려면 문 앞 자동 열림 비활성화
        door:SetDoorAutoOpen(false)
        -- 3. 버튼이 눌리거나 떼어질 때 문 개폐 연결
        button:BindOnPressed(function(bIsPressed)
            if bIsPressed then
                print("[Lua] Button Pressed! Opening door...")
                door:OpenDoor()
            else
                print("[Lua] Button Released! Closing door...")
                door:CloseDoor()
            end
        end)
    else
        print("[Lua Error] Button or Door actor not found!")
    end
end

function EndPlay()
print("[EndPlay] " .. obj.UUID)
obj:PrintLocation()
end

function OnOverlap(OtherActor)
OtherActor:PrintLocation();
end

function Tick(dt)
    -- obj.Location = obj.Location + obj.Velocity * dt
--obj:PrintLocation()
end

function OnHit(OtherActor)
end

-- Hot Reload
function OnHotReload()
end
