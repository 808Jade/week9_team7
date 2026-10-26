#pragma once

#include "GameFramework/Actor.h"

class UStaticMeshComponent;

class APortalActor : public AActor
{
	DECLARE_CLASS(APortalActor, AActor)
public:
	APortalActor();

	// 게임 시작 시 1번 호출: 2개 스폰 + 서로 연결 + 숨김 상태
	static void SpawnPair(UWorld* World, APortalActor*& OutBlue, APortalActor*& OutOrange);

	// 포탈건이 맞힌 지점/법선으로 이동
	void PlaceAt(const FVector& Location, const FVector& Normal);

	// 나와 상대가 모두 설치됐을 때만 건너편을 그린다
	bool IsActive() const { return bPlaced && LinkedPortal && LinkedPortal->bPlaced; }

	APortalActor* GetLinkedPortal() const { return LinkedPortal; }
	UStaticMeshComponent* GetPortalMesh() const { return PortalMesh; }
	FVector GetPortalNormal() const { return GetActorQuat().GetForwardVector(); }

private:
	UStaticMeshComponent* PortalMesh = nullptr;
	APortalActor* LinkedPortal = nullptr;
	bool bPlaced = false;

	float PortalWidth = 1.0f;
	float PortalHeight = 2.0f;
};