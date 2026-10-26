function BeginPlay()
    local button = obj:FindActorByName("AWeightedButtonActor_5")
    local door = obj:FindActorByName("ADoorActor_5")
    if button and door then
        door:SetDoorAutoOpen(false)
        button:BindOnPressed(function(bIsPressed)
            if bIsPressed then
                print("[Lua] Button 5 Pressed! Opening Door 5...")
                door:OpenDoor()
            else
                print("[Lua] Button 5 Released! Closing Door 5...")
                door:CloseDoor()
            end
        end)
    else
        print("[Lua Error] AWeightedButtonActor_5 or ADoorActor_5 not found!")
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