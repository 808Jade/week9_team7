#pragma once
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Components/SceneComponent.h"


class ACanvasActor : public AActor
{
	DECLARE_CLASS(ACanvasActor, AActor)
	REFLECT_START(ClassName)
		PROPERTY(bFollowCamera)
		PROPERTY(CameraDistance)
		REFLECT_END()
public:
	ACanvasActor();
	virtual ~ACanvasActor() = default;

	virtual void Tick(float DeltaTime) override;

	bool IsFollowCamera() const { return bFollowCamera; }
	float GetCameraDistance() const { return CameraDistance; }
private:
	bool bFollowCamera = true;
	float CameraDistance = 2.0f;
};