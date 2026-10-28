#pragma once

#include "GameFramework/Actor.h"
#include "Input/InputSystem.h"

class APawn;
class APlayerCameraManager;

class APlayerController : public AActor
{
	DECLARE_CLASS(APlayerController, AActor)
	REFLECT_START(ClassName)
	REFLECT_END()

public:
	APlayerController();
	void Tick(float DeltaTime) override;
	virtual void ProcessPlayerInput(float DeltaTime);
	void GetPlayerViewCamera(FVector& OutLocation, FRotator& OutRotation) const;
	virtual void Possess(APawn* InPawn);
	void UnPossess();
	APawn* GetPawn() const { return PossessedPawn; }
	virtual void InputKey(int32 Key, bool bDown);
	virtual void InputAxis(EGameInputAxis Axis, float Delta);
	virtual void ClearInputState();
	float GetLookSensitivity() const { return LookSensitivity; }
	void SetLookSensitivity(float InSens) { LookSensitivity = InSens; }
	
	APlayerCameraManager* GetPlayerCameraManager() const { return PlayerCameraManager; }
	void SpawnPlayerCameraManager();

protected:
	APawn* PossessedPawn = nullptr;
	APlayerCameraManager* PlayerCameraManager = nullptr;

	bool bKeyDown[256]{};
	float MouseAxisX = 0.0f;
	float MouseAxisY = 0.0f;
	float LookSensitivity = 0.1f;
};
