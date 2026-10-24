#pragma once

#include <sol/sol.hpp>
#include "Core/Types.h"

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

private:
    FLuaManager() = default;
    ~FLuaManager() = default;
    void BindEngine();

    sol::state Lua;
    bool bInitialized = false;
};
