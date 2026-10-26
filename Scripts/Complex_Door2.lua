function BeginPlay()
    local button = obj:FindActorByName("AWeightedButtonActor_2")
    local door = obj:FindActorByName("ADoorActor_2")
    if button and door then
        door:SetDoorAutoOpen(false)
        button:BindOnPressed(function(bIsPressed)
            if bIsPressed then
                print("[Lua] Button 2 Pressed! Opening Door 2...")
                door:OpenDoor()
            else
                print("[Lua] Button 2 Released! Closing Door 2...")
                door:CloseDoor()
            end
        end)
    else
        print("[Lua Error] AWeightedButtonActor_2 or ADoorActor_2 not found!")
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