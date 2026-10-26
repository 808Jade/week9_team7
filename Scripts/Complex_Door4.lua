function BeginPlay()
    local button = obj:FindActorByName("AWeightedButtonActor_4")
    local door = obj:FindActorByName("ADoorActor_4")
    if button and door then
        door:SetDoorAutoOpen(false)
        button:BindOnPressed(function(bIsPressed)
            if bIsPressed then
                print("[Lua] Button 4 Pressed! Opening Door 4...")
                door:OpenDoor()
            else
                print("[Lua] Button 4 Released! Closing Door 4...")
                door:CloseDoor()
            end
        end)
    else
        print("[Lua Error] AWeightedButtonActor_4 or ADoorActor_4 not found!")
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