#pragma once

#include "GameFramework/Actor.h"

class UBoxComponent;
class UStaticMeshComponent;

class AMapCubeActor : public AActor
{
	DECLARE_CLASS(AMapCubeActor, AActor)

public:
	AMapCubeActor();
	virtual ~AMapCubeActor() override = default;

	virtual void BeginPlay() override;

	UBoxComponent* GetBoxComponent() const { return BoxComponent; }
	UStaticMeshComponent* GetStaticMeshComponent() const { return StaticMeshComponent; }

	// 박스 크기 설정
	void SetBoxExtent(const FVector& InExtent);

private:
	// 루트 박스 충돌체 (플레이어/큐브/투사체 충돌 블록)
	UBoxComponent* BoxComponent = nullptr;

	// 큐브 3D 렌더링 메시
	UStaticMeshComponent* StaticMeshComponent = nullptr;
};
