#include "EnginePCH.h"
#include "PrimitiveComponent.h"
#include "../Render/Renderer.h"
#include "Asset/AssetManager.h"
#include "Render/RenderCommand.h"


#include "Engine/PrimitiveSceneProxy.h"
#include "Engine/Scene.h"
#include "Engine/World.h"

#include "Collision/OverlapResult.h"

namespace
{
	FString PrimitiveTypeToString(EPrimitiveType Type)
	{
		switch (Type)
		{
		case EPrimitiveType::Sphere:
			return "Sphere";
			break;
		case EPrimitiveType::Cube:
			return "Cube";
			break;
		case EPrimitiveType::Cone:
			return "Cone";
			break;
		case EPrimitiveType::Plane:
			return "Plane";
			break;
		default:
			return "";
			break;
		}
	}
}

UPrimitiveComponent::UPrimitiveComponent()
{
	//SetMaterial(UAssetManager::GetAssetByPath<UMaterial>("DefaultMaterial"));
}

UPrimitiveComponent::~UPrimitiveComponent()
{
	for(UPrimitiveComponent* OtherComp : OverlappingComponents)
	{
		if(OtherComp)
		{
			OtherComp->OnComponentEndOverlap.Broadcast(OtherComp, GetOwner(), this);
			OtherComp->OverlappingComponents.Remove(this);
		}
	}
	OverlappingComponents.Empty();
}

void UPrimitiveComponent::BeginPlay()
{
	Super::BeginPlay();
	UpdateOverlaps();
}


void UPrimitiveComponent::SubmitToRenderQueue(FRenderQueue& RenderQueue, const FViewContext& ViewContext)
{
}

bool UPrimitiveComponent::LineTraceComponent(const FRay& WorldRay, FHitResult& OutHit)
{
	const FStaticMeshData* Mesh = GetMeshData();
	return Mesh && TraceMesh(WorldRay, *Mesh, GetWorldMatrix(), OutHit);
}

void UPrimitiveComponent::MarkRenderStateDirty()
{
	if (SceneProxy)
		SceneProxy->GetScene()->MarkRenderStateDirty(SceneProxy);
}

bool UPrimitiveComponent::LineTraceComponentLocal(const FRay& LocalRay, float& OutT)
{
	const FStaticMeshData* Mesh = GetMeshData();
	return Mesh && TraceMeshLocal(LocalRay, *Mesh, OutT);
}

void UPrimitiveComponent::OnTransformDirty()
{
	if (SceneProxy)
		SceneProxy->GetScene()->MarkDirty(SceneProxy);
}

bool UPrimitiveComponent::MoveComponent(const FVector& Delta, const FRotator& NewRotation, bool bSweep, FHitResult* Hit)
{
	return MoveComponent(Delta, NewRotation.Quaternion(), bSweep, Hit);
}

bool UPrimitiveComponent::MoveComponent(const FVector& Delta, const FQuat& NewRotation, bool bSweep, FHitResult* OutHit)
{
	// Set up
	const FVector TraceStart = GetWorldLocation();
	const FVector TraceEnd = TraceStart + Delta;
	float DeltaSizeSq = (TraceEnd - TraceStart).SizeSquared();
	const FQuat InitialRotationQuat = GetWorldRotation().Quaternion();

	const float MinMovementDistSq = (bSweep ? FMath::Square(4.0f * KINDA_SMALL_NUMBER) : 0.0f);
	if(DeltaSizeSq <= MinMovementDistSq)
	{
		// Skip if no vector or rotation
		if(NewRotation.Equals(InitialRotationQuat, SCENECOMPONENT_QUAT_TOLERANCE))
		{
			if(OutHit)
			{
				OutHit->Init(TraceStart, TraceEnd);
			}
			return true;
		}
		DeltaSizeSq = 0.0f;
	}

	FHitResult BlockingHit;
	BlockingHit.bBlockingHit = false;
	BlockingHit.Time = 1.0f;
	bool bFilledHitResult = false;
	bool bMoved = false;
	bool bIncludesOverlapsAtEnd = false;
	bool bRotationOnly = false;
	AActor* Actor = GetOwner();

	if(!bSweep)
	{
		SetWorldLocationAndRotation(TraceEnd, NewRotation.ToFRotator(), bSweep);
		bRotationOnly = (DeltaSizeSq == 0);
		// TODO : If Rotation Only Check Overlaps at end
	}
	else
	{
		TArray<FHitResult> Hits;
		FVector NewLocation = TraceStart;

		UWorld* const MyWorld = GetWorld();
		if(MyWorld && Actor && Actor->GetActorEnableCollision() && DeltaSizeSq > 0.0f && CurrentCollisionResponse == ECollisionResponse::Block)
		{
			MyWorld->SweepMulti(Hits, TraceStart, TraceEnd, InitialRotationQuat, GetCollisionShape(), Actor);

			float BlockingHitNormalDotDelta = 0.0f;
			for(const FHitResult& TestHit : Hits)
			{
				if(TestHit.HitComponent == this || !TestHit.bBlockingHit)
				{
					continue;
				}

				if(TestHit.bStartPentrating)
				{
					const float NormalDotDelta = FVector::Dot(TestHit.ImpactNormal, Delta);
					if(NormalDotDelta < BlockingHitNormalDotDelta)
					{
						BlockingHit = TestHit;
						bFilledHitResult = true;
						BlockingHitNormalDotDelta = NormalDotDelta;
					}
				}
				else if(!bFilledHitResult)
				{
					BlockingHit = TestHit;
					bFilledHitResult = true;
					break;
				}
			}

			// Update Newlocation
			if(!BlockingHit.bBlockingHit)
			{
				NewLocation = TraceEnd;
			}
			else
			{
				NewLocation = TraceStart + (Delta * BlockingHit.Time) + BlockingHit.ImpactNormal * KINDA_SMALL_NUMBER; // 부동소수점 계산 오류 방지 
			}
		}
		else
		{
			NewLocation = TraceEnd;
		}

		SetWorldLocationAndRotation(NewLocation, NewRotation.ToFRotator(), false);
	}

	UpdateOverlaps();

	if(BlockingHit.bBlockingHit && BlockingHit.HitComponent)
	{
		OnComponentHit.Broadcast(this, BlockingHit.HitActor, BlockingHit.HitComponent, FVector::ZeroVector, BlockingHit);

		FHitResult ReverseHit = BlockingHit;
		ReverseHit.Normal = -BlockingHit.Normal;
		ReverseHit.ImpactNormal = -BlockingHit.ImpactNormal;
		ReverseHit.HitComponent = this;
		ReverseHit.HitActor = this->GetOwner();

		BlockingHit.HitComponent->OnComponentHit.Broadcast(BlockingHit.HitComponent, this->GetOwner(), this, FVector::ZeroVector, ReverseHit);
	}

	if(OutHit)
	{
		if(bFilledHitResult)
		{
			*OutHit = BlockingHit;
		}
		else
		{
			OutHit->Init(TraceStart, TraceEnd);
		}
	}

	return true;
}

void UPrimitiveComponent::PostEditChangeProperty(const FProperty& Property)
{
	Super::PostEditChangeProperty(Property);
	if (Property.Name == "bCastShadow" ||
		Property.Name == "bReceiveShadow")
	{
		MarkRenderStateDirty();
	}
}

void UPrimitiveComponent::SetCastShadow(bool NewValue)
{
	if (bCastShadow != NewValue)
	{
		bCastShadow = NewValue;
	}
}

void UPrimitiveComponent::SetReceiveShadow(bool NewValue)
{
	if (bReceiveShadow != NewValue)
	{
		bReceiveShadow = NewValue;
	}
}

void UPrimitiveComponent::CreateRenderState()
{
	Super::CreateRenderState();
	GetWorld()->GetScene().AddPrimitive(this);
}

void UPrimitiveComponent::DestroyRenderState()
{
	GetWorld()->GetScene().RemovePrimitive(this);
	Super::DestroyRenderState();
}

bool UPrimitiveComponent::TraceMesh(const FRay& WorldRay, const FStaticMeshData& Mesh, const FMatrix& WorldMatrix, FHitResult& OutResult)
{
	FBox MeshBox = Mesh.AABB.GetWorldAABB(WorldMatrix);
	float BoxT;
	if (!RayIntersectsAABB(WorldRay, MeshBox.Min, MeshBox.Max, BoxT))
	{
		return false;
	}

	FRay LocalRay = ToLocalRay(WorldRay, WorldMatrix);
	float T = FLT_MAX;

	if (!RayIntersectsMesh(LocalRay, Mesh, T)) return false;

	OutResult.HitComponent = this;
	OutResult.Distance = T;
	OutResult.ImpactPoint = WorldRay.Origin + WorldRay.Direction * T;

	return true;
}



bool UPrimitiveComponent::TraceMeshLocal(const FRay& LocalRay, const FStaticMeshData& Mesh, float& OutT)
{
	return RayIntersectsMesh(LocalRay, Mesh, OutT);
}

bool UPrimitiveComponent::UpdateOverlaps()
{
	const AActor* MyActor = GetOwner();
	if(MyActor && (!MyActor->HasBegunPlay() || !MyActor->GetActorEnableCollision()))
	{
		return false;
	}

	UWorld* MyWorld = GetWorld();
	if(!MyWorld)
	{
		return false;
	}

	TArray<FOverlapResult> Overlaps;
	MyWorld->OverlapMulti(Overlaps, GetWorldLocation(), GetWorldRotation().Quaternion(), GetCollisionShape(), MyActor);

	TArray<UPrimitiveComponent*> NewOverlappingComponents;
	for(FOverlapResult& Overlap : Overlaps)
	{
		if(Overlap.Component == this)
		{
			continue;
		}
		NewOverlappingComponents.AddUnique(Overlap.Component);
	}

	// On End Overlap
	for(UPrimitiveComponent* OldComp : OverlappingComponents)
	{
		if(OldComp == nullptr)
		{
			continue;
		}

		if(NewOverlappingComponents.Find(OldComp) == -1)
		{
			OnComponentEndOverlap.Broadcast(this, OldComp->GetOwner(), OldComp);
			OldComp->OnComponentEndOverlap.Broadcast(OldComp, GetOwner(), this);
			OldComp->OverlappingComponents.Remove(this);
		}
	}

	// On Begin overlap
	FHitResult EmptyHit;
	for(UPrimitiveComponent* NewComp : NewOverlappingComponents)
	{
		if(NewComp == nullptr)
		{
			continue;
		}

		if(OverlappingComponents.Find(NewComp) == -1)
		{
			OnComponentBeginOverlap.Broadcast(this, NewComp->GetOwner(), NewComp, false, EmptyHit);
			NewComp->OnComponentBeginOverlap.Broadcast(NewComp, GetOwner(), this, false, EmptyHit);
			NewComp->OverlappingComponents.AddUnique(this);
		}
	}

	// Overlapupdate for children
	TArray<USceneComponent*> AttachedChildren;
	AttachedChildren.Append(GetAttachChildren());
	for(USceneComponent* const ChildComp : AttachedChildren)
	{
		if(ChildComp == nullptr)
		{
			continue;
		}

		UPrimitiveComponent* ChildPrim = Cast<UPrimitiveComponent>(ChildComp);
		if(ChildPrim)
		{
			ChildPrim->UpdateOverlaps();
		}
	}

	OverlappingComponents = NewOverlappingComponents;

	return true;
}
