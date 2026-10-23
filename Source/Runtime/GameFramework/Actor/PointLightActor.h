#pragma once

#include "GameFramework/Actor/LightActorBase.h"
#include "Components/PointLightComponent.h"

class AStaticMeshActor;

class APointLightActor : public ALightActorBase
{
	DECLARE_CLASS(APointLightActor, ALightActorBase)

	REFLECT_START(ClassName)
	REFLECT_END()

public:
	APointLightActor();
	virtual ~APointLightActor() override;

	virtual void BeginPlay() override;

    virtual ULightComponent* GetLightComponent() const { return PointLightComponent; }
    UPointLightComponent* GetPointLightComponent() const { return PointLightComponent; }

	// 남의 델리게이트(StaticMeshActor->OnPicked)를 구독/해제하는 함수
	void WatchMesh(AStaticMeshActor* TargetMesh);
	void UnwatchMesh();
	void OnTargetMeshPicked(AStaticMeshActor* Actor, int Count);

	AStaticMeshActor* GetObservedMesh() const { return ObservedMesh; }

private:
    UPointLightComponent* PointLightComponent = nullptr;
	AStaticMeshActor* ObservedMesh = nullptr;
};
