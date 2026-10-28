#pragma once
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Core/Delegate.h"

DECLARE_DELEGATE(FOnButtonPressed, bool)

class AButtonActorBase : public AActor
{
	DECLARE_CLASS(AButtonActorBase, AActor)

	REFLECT_START(AButtonActorBase)
		PROPERTY(bIsPressed)
		PROPERTY(PressDepth)
		PROPERTY(InterpSpeed)
	REFLECT_END()

public:
	AButtonActorBase();
	virtual ~AButtonActorBase() override = default;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	virtual void PressButton();
	virtual void ReleaseButton();

	void PlayButtonPressedSound();

	bool IsPressed() const { return bIsPressed; }

	FOnButtonPressed OnButtonPressed;

protected:
	bool bIsPressed = false;
	float PressDepth = 0.1f;       // 눌리는 깊이 (기본 0.1f)
	float InterpSpeed = 10.0f;     // 눌리고 복구되는 보간 속도
	FVector InitialButtonLocation = FVector::ZeroVector; // 초기 버튼 높이

	UStaticMeshComponent* BaseMesh = nullptr;
	UBoxComponent* BaseCollisionBox = nullptr;
	UStaticMeshComponent* ButtonMesh = nullptr;
	UBoxComponent* TriggerBox = nullptr;
};
