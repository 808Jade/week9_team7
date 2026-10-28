#pragma once
#include "GameFramework/Actor.h"

class UStaticMeshComponent;
class UBoxComponent;

class ATurretActor : public AActor
{
	DECLARE_CLASS(ATurretActor, AActor)
	REFLECT_START(ClassName)
		REFLECT_END()
public:
	ATurretActor();
	virtual ~ATurretActor() = default;

	virtual void Tick(float DeltaTime) override;
	// 터렛이 맞았을 때 호출되는 함수. 충격 방향과 충격 지점을 전달받는다.
	void OnHit(const FVector& Direction, const FVector& ImpactPoint);
	// 터렛이 맞았을 때 메시를 눌러서 찌그러뜨리는 크기를 적용하는 함수
	void ApplyHitSquash(const FVector& HitDirection);
	// 터렛이 맞았을 때 메시를 눌러서 찌그러뜨리는 크기를 구하는 함수
	FVector GetDirectionalSquashScale(const FVector& HitDirection) const;

	void SetIsHit(bool bInIsHit) { bIsHit = bInIsHit; }
	bool GetIsHit() const { return bIsHit; }

private:
	// 터렛 충돌용 박스 컴포넌트 (루트)
	UBoxComponent* CollisionBox = nullptr;
	// 터렛 메시 컴포넌트
	UStaticMeshComponent* TurretMeshComponent = nullptr;
	// 터렛이 맞았는지 여부를 나타내는 플래그
	bool bIsHit = false;
	// 바닥 착지 여부
	bool bIsGrounded = false;
	// 터렛의 넉백을 위한 속도 벡터
	FVector Velocity = FVector::ZeroVector;
	// 중력 가속도
	float Gravity = 9.8f;
};
