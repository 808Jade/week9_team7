function BeginPlay()
    local button = obj:FindActorByName("AWeightedButtonActor_3")
    local door = obj:FindActorByName("ADoorActor_3")
    if button and door then
        door:SetDoorAutoOpen(false)
        button:BindOnPressed(function(bIsPressed)
            if bIsPressed then
                print("[Lua] Button 3 Pressed! Opening Door 3...")
                door:OpenDoor()
            else
                print("[Lua] Button 3 Released! Closing Door 3...")
                door:CloseDoor()
            end
        end)
    else
        print("[Lua Error] AWeightedButtonActor_3 or ADoorActor_3 not found!")
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