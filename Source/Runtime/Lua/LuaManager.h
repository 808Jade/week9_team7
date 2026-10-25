#pragma once

#include <sol/sol.hpp>
#include "Core/Types.h"

class UWorld;

class FLuaManager
{
public:
    FLuaManager() = default;
    ~FLuaManager();

    FLuaManager(const FLuaManager&) = delete;
    FLuaManager& operator=(const FLuaManager&) = delete;

    bool Init();

    // GameInstance::Shutdown 또는 소멸자에서 호출
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

    bool IsInitialized() const { return bInitialized; }

    void SetActiveWorld(UWorld* InWorld) { ActiveWorld = InWorld; }
    UWorld* GetActiveWorld() const { return ActiveWorld; }

    // 활성 인스턴스 관리 (GameInstance 소유)
    static FLuaManager* GetActive() { return ActiveManager; }
    static void SetActiveManager(FLuaManager* Manager) { ActiveManager = Manager; }

    // 기존 FLuaManager::Get() 호환:
    // 활성 GameInstance의 LuaManager가 있으면 반환, 없을 경우 폴백 인스턴스 반환
    static FLuaManager& Get()
    {
        if (ActiveManager)
        {
            return *ActiveManager;
        }
        static FLuaManager FallbackInstance;
        return FallbackInstance;
    }

private:
    void BindEngine();

    static FLuaManager* ActiveManager;

    sol::state Lua;
    bool bInitialized = false;
    UWorld* ActiveWorld = nullptr;
};
