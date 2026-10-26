function BeginPlay()
    local button = obj:FindActorByName("AWeightedButtonActor_1")
    local door = obj:FindActorByName("ADoorActor_1")
    if button and door then
        door:SetDoorAutoOpen(false)
        button:BindOnPressed(function(bIsPressed)
            if bIsPressed then
                print("[Lua] Button 1 Pressed! Opening Door 1...")
                door:OpenDoor()
            else
                print("[Lua] Button 1 Released! Closing Door 1...")
                door:CloseDoor()
            end
        end)
    else
        print("[Lua Error] AWeightedButtonActor_1 or ADoorActor_1 not found!")
    end
end

function EndPlay()
end

function OnOverlap(OtherActor)
end

function Tick(dt)
end

function OnHit(OtherActor)
end

function OnHotReload()
end