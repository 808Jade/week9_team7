#pragma once
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Components/SceneComponent.h"


class ACanvasActor : public AActor
{
	DECLARE_CLASS(ACanvasActor, AActor)
	REFLECT_START(ClassName)
		PROPERTY(bCanvasVisible)
		PROPERTY(bFollowCamera)
		PROPERTY(CameraDistance)
		REFLECT_END()
public:
	ACanvasActor();
	virtual ~ACanvasActor() = default;

	virtual void Tick(float DeltaTime) override;

	bool IsFollowCamera() const { return bFollowCamera; }
	float GetCameraDistance() const { return CameraDistance; }
	bool IsCanvasVisible() const { return bCanvasVisible; }
	virtual void SetCanvasVisibility(bool bVisible);
private:
	bool bCanvasVisible = true;
	bool bFollowCamera = false;
	float CameraDistance = 2.0f;
};