#pragma once
#include <vector>
#include <functional>
#include <utility>

template<typename... Args>
class TDelegate
{
public:
    using FFuncType = std::function<void(Args...)>;

private:
    std::vector<FFuncType> Handlers;

public:
    TDelegate() = default;
    ~TDelegate() = default;

    // 1. 일반 함수, 정적(static) 함수, 람다 표현식 등록용
    void Add(FFuncType InFunc)
    {
        if (InFunc)
        {
            Handlers.push_back(std::move(InFunc));
        }
    }

    // 2. 특정 객체(Instance)와 그 객체의 멤버 함수(Func)를 묶어서 등록
    template<typename UserClass>
    void AddDynamic(UserClass* Instance, void(UserClass::* Func)(Args...))
    {
        if (Instance && Func)
        {
            Handlers.push_back([Instance, Func](Args... args)
                {
                    (Instance->*Func)(std::forward<Args>(args)...);
                });
        }
    }

    // 3. const 멤버 함수 지원용 오버로딩
    template<typename UserClass>
    void AddDynamic(const UserClass* Instance, void(UserClass::* Func)(Args...) const)
    {
        if (Instance && Func)
        {
            Handlers.push_back([Instance, Func](Args... args)
                {
                    (Instance->*Func)(std::forward<Args>(args)...);
                });
        }
    }

    // 4. 등록된 모든 함수 일괄 호출
    void Broadcast(Args... args)
    {
        // 실행 도중 리스트가 수정(삭제/재시작 등)되어도 안전하도록 복사본으로 순회하거나 인덱스 순회
        for (size_t i = 0; i < Handlers.size(); ++i)
        {
            if (Handlers[i])
            {
                Handlers[i](args...);
            }
        }
    }

    // 5. 바인딩된 모든 함수 초기화 (게임 재시작이나 액터 파괴 시)
    void Clear()
    {
        Handlers.clear();
    }

    // 현재 연결된 함수가 있는지 확인
    bool IsBound() const
    {
        return !Handlers.empty();
    }
};


// 메크로
#define DECLARE_DELEGATE(DelegateName, ...) \
    using DelegateName = TDelegate<__VA_ARGS__>;