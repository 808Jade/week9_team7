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

}

void UPrimitiveComponent::BeginPlay()
{
	Super::BeginPlay();
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
	return Super::MoveComponent(Delta, NewRotation, bSweep, Hit);
}

void UPrimitiveComponent::PostEditChangeProperty(const FProperty& Property)
{
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
		NewOverlappingComponents.Add(Overlap.Component);
	}

	FHitResult EmptyHit;
	for(UPrimitiveComponent* NewComp : NewOverlappingComponents)
	{
		if(OverlappingComponents.Find(NewComp) == -1)
		{
			OnComponentBeginOverlap.Broadcast(this, NewComp->GetOwner(), NewComp, false, EmptyHit);
			if(NewComp)
			{
				NewComp->OnComponentBeginOverlap.Broadcast(NewComp, GetOwner(), this, false, EmptyHit);
				NewComp->OverlappingComponents.AddUnique(this);
			}
		}
	}

	for(UPrimitiveComponent* OldComp : OverlappingComponents)
	{
		if(NewOverlappingComponents.Find(OldComp) == -1)
		{
			OnComponentEndOverlap.Broadcast(this, OldComp->GetOwner(), OldComp);
			if(OldComp)
			{
				OldComp->OnComponentEndOverlap.Broadcast(OldComp, GetOwner(), this);
				OldComp->OverlappingComponents.Remove(this);
			}
		}
	}


	OverlappingComponents = NewOverlappingComponents;

	return true;
}
