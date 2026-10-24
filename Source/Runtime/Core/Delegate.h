#pragma once
#include <vector>
#include <functional>
#include <utility>
#include <cstdint>
#include <atomic>

// 델리게이트 등록 식별 핸들 (언리얼의 FDelegateHandle 스타일)
class FDelegateHandle
{
public:
    uint64_t ID = 0;

    FDelegateHandle() : ID(0) {}
    explicit FDelegateHandle(uint64_t InID) : ID(InID) {}

    bool IsValid() const { return ID != 0; }
    void Reset() { ID = 0; }

    bool operator==(const FDelegateHandle& Other) const { return ID == Other.ID; }
    bool operator!=(const FDelegateHandle& Other) const { return ID != Other.ID; }

    static FDelegateHandle GenerateNew()
    {
        static std::atomic<uint64_t> GlobalIDCounter{ 1 };
        return FDelegateHandle(GlobalIDCounter.fetch_add(1));
    }
};

template<typename... Args>
class TDelegate
{
public:
    using FFuncType = std::function<void(Args...)>;

private:
    struct FDelegateEntry
    {
        FDelegateHandle Handle;
        const void* UserObject = nullptr; // AddDynamic에 바인딩된 인스턴스 (람다/일반함수는 nullptr)
        FFuncType Callback;
    };

    std::vector<FDelegateEntry> Handlers;

public:
    TDelegate() = default;
    ~TDelegate() = default;

    // 1. 일반 함수, 정적(static) 함수, 람다 표현식 등록용 (Handle 반환)
    FDelegateHandle Add(FFuncType InFunc)
    {
        if (!InFunc) return FDelegateHandle();

        FDelegateHandle Handle = FDelegateHandle::GenerateNew();
        Handlers.push_back(FDelegateEntry{
            Handle,
            nullptr,
            std::move(InFunc)
            });
        return Handle;
    }

    // 2. 특정 객체(Instance)와 그 객체의 멤버 함수(Func)를 묶어서 등록 (Handle 반환)
    template<typename UserClass>
    FDelegateHandle AddDynamic(UserClass* Instance, void(UserClass::* Func)(Args...))
    {
        if (!Instance || !Func) return FDelegateHandle();

        FDelegateHandle Handle = FDelegateHandle::GenerateNew();
        Handlers.push_back(FDelegateEntry{
            Handle,
            static_cast<const void*>(Instance),
            [Instance, Func](Args... args)
            {
                (Instance->*Func)(std::forward<Args>(args)...);
            }
            });
        return Handle;
    }

    // 3. const 멤버 함수 지원용 오버로딩
    template<typename UserClass>
    FDelegateHandle AddDynamic(const UserClass* Instance, void(UserClass::* Func)(Args...) const)
    {
        if (!Instance || !Func) return FDelegateHandle();

        FDelegateHandle Handle = FDelegateHandle::GenerateNew();
        Handlers.push_back(FDelegateEntry{
            Handle,
            static_cast<const void*>(Instance),
            [Instance, Func](Args... args)
            {
                (Instance->*Func)(std::forward<Args>(args)...);
            }
            });
        return Handle;
    }

    // 4. 특정 핸들(ID)을 가진 핸들러 1개만 제거
    bool Remove(FDelegateHandle Handle)
    {
        if (!Handle.IsValid()) return false;

        for (auto it = Handlers.begin(); it != Handlers.end(); ++it)
        {
            if (it->Handle == Handle)
            {
                Handlers.erase(it);
                return true;
            }
        }
        return false;
    }

    // 5. 특정 객체(Instance)에 등록된 모든 핸들러 제거
    int32_t RemoveAll(const void* Instance)
    {
        if (!Instance) return 0;

        int32_t RemovedCount = 0;
        for (auto it = Handlers.begin(); it != Handlers.end(); )
        {
            if (it->UserObject == Instance)
            {
                it = Handlers.erase(it);
                RemovedCount++;
            }
            else
            {
                ++it;
            }
        }
        return RemovedCount;
    }

    // 6. 등록된 모든 함수 일괄 호출
    void Broadcast(Args... args)
    {
        // 순회 도중 리스너 내부에서 Add/Remove가 호출되어도 안전하도록 복사본 순회 (Re-entrancy 보장)
        auto EntriesCopy = Handlers;
        for (const auto& Entry : EntriesCopy)
        {
            if (Entry.Callback)
            {
                Entry.Callback(args...);
            }
        }
    }

    // 7. 바인딩된 모든 함수 초기화 (게임 재시작이나 액터 파괴 시)
    void Clear()
    {
        Handlers.clear();
    }

    // 현재 연결된 함수가 있는지 확인
    bool IsBound() const
    {
        return !Handlers.empty();
    }

    // 특정 객체가 바인딩되어 있는지 확인
    bool IsBoundToObject(const void* Instance) const
    {
        if (!Instance) return false;
        for (const auto& Entry : Handlers)
        {
            if (Entry.UserObject == Instance) return true;
        }
        return false;
    }
};

// 매크로
#define DECLARE_DELEGATE(DelegateName, ...) \
    using DelegateName = TDelegate<__VA_ARGS__>;
