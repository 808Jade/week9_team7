#pragma once

#include <sol/sol.hpp>
#include "Core/Types.h"

class UWorld;

class FLuaManager
{
public:
    static FLuaManager& Get()
    {
        static FLuaManager Instance;
        return Instance;
    }

    bool Init();

    // 엔진 종료 시 호출
    void Shutdown();

    // 루아 가상 머신 반환
    sol::state& GetState()
    {
        if (!bInitialized)
        {
            Init();
        }
        return Lua;
    }

    void SetActiveWorld(UWorld* InWorld) { ActiveWorld = InWorld; }
    UWorld* GetActiveWorld() const { return ActiveWorld; }

private:
    FLuaManager() = default;
    ~FLuaManager() = default;
    void BindEngine();

    sol::state Lua;
    bool bInitialized = false;
    UWorld* ActiveWorld = nullptr;
};
