function BeginPlay()
    local button = obj:FindActorByName("AWeightedButtonActor_6")
    local door = obj:FindActorByName("ADoorActor_6")
    if button and door then
        door:SetDoorAutoOpen(false)
        button:BindOnPressed(function(bIsPressed)
            if bIsPressed then
                print("[Lua] Button 6 Pressed! Opening Door 6...")
                door:OpenDoor()
            else
                print("[Lua] Button 6 Released! Closing Door 6...")
                door:CloseDoor()
            end
        end)
    else
        print("[Lua Error] AWeightedButtonActor_6 or ADoorActor_6 not found!")
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