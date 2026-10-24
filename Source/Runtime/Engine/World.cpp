#include "EnginePCH.h"
#include "World.h"
#include "Level.h"

#include "Core/EngineStatics.h"
#include "GameFramework/Actor/StaticMeshActor.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/DefaultPawn.h"
#include "GameFramework/PlayerController.h"
#include "Input/InputSystem.h"

#include "UObject/UObjectIterator.h"

#include "Collision/Ray.h"
#include "Components/BillboardComponent.h"
#include "Components/ParticleSubUVComponent.h"
#include "Components/ExponentialHeightFogComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Asset/LOD/StaticMeshLODSelector.h"

#include "Math/Frustum.h"

#include "Core/Stats/LightweightStats.h"
#include "Core/Stats/EditorStats.h"
#include "Core/Async/TaskPool.h"
#include "Render/ObjectConstants.h"

#include "UObject/UObjectGlobals.h"

namespace
{
	inline void WritePerObjectConstants(uint8* Dest, const FMatrix& LocalToWorld, const FMatrix& WorldToLocal)
	{
		FPerObjectConstants Constants{};
		Constants.World = LocalToWorld;
		Constants.NormalMatrix = WorldToLocal.GetTransposed();
		std::memcpy(Dest, &Constants, sizeof(Constants));
	}
}

DECLARE_CYCLE_STAT("Actor Tick", STAT_ActorTick); // Actor ??痢≪젙
DECLARE_CYCLE_STAT("Update All Transforms", STAT_UpdateAllTransforms); // 媛?Transform??Update ?쒓컙 痢≪젙
DECLARE_CYCLE_STAT("Gather Render Packets", STAT_GatherRenderPackets);
DECLARE_CYCLE_STAT("Frustum Cull", STAT_FrustumCull);
DECLARE_CYCLE_STAT("Gather Elements", STAT_GatherElements);
DECLARE_CYCLE_STAT("Gather - LOD", STAT_GatherLOD);
DECLARE_CYCLE_STAT("Gather - Submit", STAT_GatherSubmit);

UWorld::~UWorld()
{

}

bool UWorld::Init()
{
	// Spawn Actor濡?移대찓???앹꽦?섍퀬 ?명똿?섍린
	PersistentLevel = NewObject<ULevel>(this);

	if (!PersistentLevel)
	{
		HTR_LOG(Error, "Failed to create PersistentLevel");
		return false;
	}

	//?덈꺼 ?곌껐
	PersistentLevel->SetWorld(this);
	Levels.Add(PersistentLevel);
	CurrentLevel = PersistentLevel;

	//移대찓???앹꽦
	CreateMainCamera();

	return true;
}

AActor* UWorld::SpawnActor(UClass* Class, FName InName, const FTransform* Transform)
{
	if (!Class) return nullptr;
	if (!Class->IsChildOf(AActor::StaticClass())) return nullptr;

	// 1. ObjectFactory濡?Actor ?앹꽦
	AActor* NewActor = NewObject<AActor>(PersistentLevel, Class, InName);

	if (!NewActor)
	{
		HTR_LOG(Error, "SpawnActor : Failed to create Actor");
		return nullptr;
	}

	// 2. Actor??World/Level ?곌껐
	NewActor->World = this;
	NewActor->Level = PersistentLevel;

	// 3. Transform ?곸슜
	const FTransform SpawnTransform = Transform ? *Transform : FTransform::Identity;

	if (NewActor->GetRootComponent())
	{
		NewActor->GetRootComponent()->SetTransform(SpawnTransform);
	}
	
	// 4. Level->Actors???깅줉????而댄룷?뚰듃瑜??붾뱶???곌껐?쒕떎.
	PersistentLevel->AddActor(NewActor);
	NewActor->RegisterAllComponents();

	// ?붾뱶媛 ?ъ깮 以묒씪 ???앹꽦???≫꽣留?蹂꾨룄 BeginPlay ?湲곗뿴???ｋ뒗??
	if (bBegunPlay) BeginPlayList.Enqueue(NewActor);

	return NewActor;
}

void UWorld::Tick(float DeltaTime)
{
	{
		SCOPE_CYCLE_COUNTER(STAT_UpdateAllTransforms);
		Scene.UpdateAllTransforms();
	}

	// ?먮뵒???붾뱶媛 ?꾨땲硫??≫꽣 ?깆쓣 ?뚮┛??
	if (WorldType != EWorldType::Editor)
	{
		while (!BeginPlayList.IsEmpty())
		{
			AActor* Actor = BeginPlayList.Peek();
			BeginPlayList.Dequeue();
			if (Actor) Actor->BeginPlay();
		}

		if (!bIsPaused)
		{
			SCOPE_CYCLE_COUNTER(STAT_ActorTick);
			// 紐⑤뱺 Actor瑜??꾨뒗 ????깅줉??Tick ?⑥닔留??ㅽ뻾?쒕떎
			TickTaskManager.RunAllTickGroups(DeltaTime);

			for (ULevel* Level : Levels)
			{
				PathTracker.Tick(Level->GetActors(), DeltaTime);
			}
		}
	}
	else
	{
		// ?먮뵒???붾뱶?먯꽌???뚰떚???쒕??덉씠?섏? ?깆씠 ?뚯븘???쒕떎
		for (TObjectIterator<UParticleSubUVComponent> Comp; Comp; ++Comp)
		{
			if (!Comp || !Comp->IsRegistered() || Comp->GetWorld() != this)
			{
				continue;
			}
			if (Comp->GetParticlesForView().IsEmpty())
			{
				Comp->SetParticles(10);
			}
			Comp->TickComponent(DeltaTime);
		}
	}


}

void UWorld::ClearWorld()
{
	EndPlay();
	// BeginPlay ?湲?以묒씤 Actor ?쒓굅
	while (!BeginPlayList.IsEmpty())
	{
		BeginPlayList.Dequeue();
	}

	PathTracker.SetPlaybackEnabled(false);
	PathTracker.SetPathRenderingEnabled(false);
	PathTracker.ClearPath();

	
	for (ULevel* Level : Levels)
	{
		for (AActor* Actor : Level->Actors)
		{
			if (!Actor) continue;
			Actor->UnregisterAllComponents();
			Actor->RegisterAllActorTickFunctions(false);
		}
		Level->ClearActors();
	}
	// ?≫꽣瑜?吏?곌린 ?꾩뿉 ?뚮뜑 ?꾨줉?쒖? ???깅줉遺???쇰떎. ClearActors???≫꽣瑜?delete留??섎?濡?
	// 洹몃?濡??먮㈃ 吏?뚯쭊 而댄룷?뚰듃瑜?媛由ы궎???꾨줉?쒓? FScene???⑥븘 ?ㅼ쓬 ?꾨젅?꾩뿉 ?곗쭊??
	Scene.RemoveAllLights();
	Scene.RemoveAllPrimitives();
	HTR_LOG(Info, "{} : ", PersistentLevel->GetActorNum());
}

void UWorld::GatherRenderPackets(FRenderQueue& RenderQueue, const FViewContext* LODView, const FFrustumPlanes* Frustum, FRenderer* Renderer)
{
	// 硫ㅻ쾭濡??먯뼱 留??꾨젅???⑸웾???ъ궗?⑺븳??
	RenderStats.Reset();
	VisibleProxies.Reset();

	{
		SCOPE_CYCLE_COUNTER(STAT_FrustumCull);
		// 而щ쭅 ?④퀎?먯꽌???꾨줉???ъ씤?곕쭔 紐⑥쑝怨? 而댄룷?뚰듃 ??갭議?媛?쒖꽦 ?뺤씤)???댁감??而댄룷?뚰듃瑜??쎈뒗 Gather濡?誘몃，??
		const auto Visit = [&](FPrimitiveSceneProxy* Proxy) { VisibleProxies.Add(Proxy); };

		if (Frustum)
		{
			Scene.BVH.QueryCull(
				FrustumAllPlanesMask,
				[Frustum](const FBox& Bounds, uint32& Mask)
				{
					return static_cast<EBVHCullResult>(ClassifyBoxInFrustum(Bounds, *Frustum, Mask));
				},
				Visit);
		}
		else
		{
			Scene.BVH.QueryCull(0, [](const FBox&, uint32&) { return EBVHCullResult::Inside; }, Visit);
		}
	}

	RenderStats.TotalPrimitives = Scene.PrimitiveSceneProxies.Num();

	// GPU ?ㅽ겢猷⑥쟾: ?꾨윭?ㅽ????듦낵??臾쇱껜瑜?GPU?먯꽌 媛由??먯젙?섍퀬 寃곌낵瑜??대쾲 ?꾨젅?꾩뿉 諛쏆븘 ?⑤떎.
	// Cull??耳쒖졇 ?덉쑝硫?媛?ㅼ쭊 臾쇱껜瑜?紐⑸줉?먯꽌 鍮쇱꽌 ?댄썑 Gather쨌?뺣젹쨌?쒕줈?곕? 紐⑤몢 嫄대꼫?대떎.
	// 爰쇱졇 ?덉쑝硫?寃利?紐⑤뱶) 紐⑸줉? 洹몃?濡??먭퀬 ?⑦궥???먯젙留??쒖떆?쒕떎.
	const uint8* OccludedMask = nullptr;
	if (Renderer && LODView && Renderer->GetGPUOcclusion().GetSettings().bEnabled)
	{
		FGPUOcclusion& Occlusion = Renderer->GetGPUOcclusion();
		if (Occlusion.Run(VisibleProxies.GetData(), VisibleProxies.Num(), *LODView))
		{
			const std::vector<uint8>& Occluded = Occlusion.GetOccluded();
			if (Occlusion.GetSettings().bCull)
			{
				uint32 Kept = 0;
				for (uint32 i = 0; i < VisibleProxies.Num(); ++i)
					if (!Occluded[i])
						VisibleProxies[Kept++] = VisibleProxies[i];
				VisibleProxies.SetNum(Kept);
			}
			else
			{
				OccludedMask = Occluded.data();
			}
		}
	}

	RenderStats.VisiblePrimitives = VisibleProxies.Num();

	constexpr uint32 ExtraSlots = 4096;
	const uint32 VisibleCount = VisibleProxies.Num();
	const uint32 MaxSlots = VisibleCount + ExtraSlots;
	uint8* SlotDest = Renderer ? Renderer->BeginObjectConstants(MaxSlots) : nullptr;
	uint32 NextExtraSlot = VisibleCount;

	// 議곌컖 ??= ?ㅻ젅????횞 4. ?섍쾶 ?섎닠??癒쇱? ?앸궃 ?ㅻ젅?쒓? ?⑥? 議곌컖??媛?멸???遺?섍? 怨좊Ⅴ寃??쒕떎.
	FTaskPool& Pool = FTaskPool::Get();
	const uint32 ChunkCount = FMath::Clamp(VisibleCount, 1u, Pool.GetNumThreads() * 4);

	if (GatherChunks.Num() < ChunkCount)
		GatherChunks.SetNum(ChunkCount);     // ?섎┫ ?뚮쭔. 以꾩씠吏 ?딆븘??諛곗뿴 ?⑸웾??怨꾩냽 ?ъ궗?⑸맂??

	for (uint32 c = 0; c < GatherChunks.Num(); ++c)   // ?대쾲?????곕뒗 議곌컖??臾띠쓬??鍮꾩썙 ?붾떎 (Renderer???섍린吏 ?딅룄濡?
	{
		FGatherChunk& Chunk = GatherChunks[c];
		Chunk.Packets.Reset();               // ?⑸웾? ?좎?, 媛쒖닔留?0
		Chunk.SlowPathIndices.Reset();
		std::fill(std::begin(Chunk.LODCounts), std::end(Chunk.LODCounts), 0u);
		std::fill(std::begin(Chunk.LODTriangles), std::end(Chunk.LODTriangles), 0ull);
		for (FStaticDrawGroup& Group : Chunk.Groups)
			Group.Items.clear();             // ?⑸웾? ?좎?
		Chunk.StaticDrawCount = 0;
	}

	const bool bStaticGroups = false;

	{
		SCOPE_CYCLE_COUNTER(STAT_GatherElements);

		LODInputs.Reset();
		if (LODView)
		{
			LODInputs.Reserve(VisibleProxies.Num());
			for (const FPrimitiveSceneProxy* Proxy : VisibleProxies)
				LODInputs.Add({ Proxy->GetLODSphere(), Proxy->GetRenderState() });
			SelectLODs(LODInputs, *LODView, SelectedLODs);
		}
		Pool.ParallelFor(VisibleCount, ChunkCount, [&](uint32 Begin, uint32 End, uint32 ChunkIndex)
			{
				FGatherChunk& Out = GatherChunks[ChunkIndex];      // ??議곌컖 ?꾩슜. ?ㅻⅨ ?ㅻ젅?쒕뒗 ?덈? ??嫄대뱶由?
				// (癒명떚由ъ뼹, 硫붿떆, LOD) 臾띠쓬 李얘린. 議고빀??紐?媛쒕퓧?대씪 ?좏삎 ?먯깋?대㈃ 異⑸텇?섍퀬, 諛붾줈 ??臾띠쓬??癒쇱? 蹂몃떎.
				const auto FindGroup = [&Out](UMaterial* Material, UStaticMesh* Mesh, uint8 LOD) -> FStaticDrawGroup&
					{
						if (Out.LastGroup < Out.Groups.size())
						{
							FStaticDrawGroup& Last = Out.Groups[Out.LastGroup];
							if (Last.Material == Material && Last.Mesh == Mesh && Last.LODIndex == LOD)
								return Last;
						}
						for (uint32 g = 0; g < Out.Groups.size(); ++g)
						{
							FStaticDrawGroup& Group = Out.Groups[g];
							if (Group.Material == Material && Group.Mesh == Mesh && Group.LODIndex == LOD)
							{
								Out.LastGroup = g;
								return Group;
							}
						}
						Out.LastGroup = static_cast<uint32>(Out.Groups.size());
						FStaticDrawGroup& Group = Out.Groups.emplace_back();
						Group.Material = Material;
						Group.Mesh = Mesh;
						Group.LODIndex = LOD;
						return Group;
					};

				for (uint32 VisibleIndex = Begin; VisibleIndex < End; ++VisibleIndex)
				{
					FPrimitiveSceneProxy* Proxy = VisibleProxies[VisibleIndex];   // ?쎄린留?
					if (!Proxy->IsVisible()) continue;

					UStaticMesh* Mesh = Proxy->GetMesh();
					if (!Mesh)
					{
						Out.SlowPathIndices.Add(VisibleIndex);     // 而댄룷?뚰듃瑜?嫄대뱶由щ뒗 寃쎈줈??硫붿씤???섏쨷??
						continue;
					}

					const uint32 LOD = LODView ? SelectedLODs[VisibleIndex] : 0;
					const FCachedMeshLOD& CachedLOD = Proxy->GetLOD(LOD);
					++Out.LODCounts[LOD];                          // RenderStats ???議곌컖 ?꾩슜 ?듦퀎

					uint32 Slot = InvalidObjectSlot;
					if (SlotDest)
					{
						// 移?VisibleIndex????諛섎났留??대떎 ???ㅻ젅?쒕겮由?寃뱀튂吏 ?딆쓬
						WritePerObjectConstants(SlotDest + size_t(VisibleIndex) * ObjectSlotBytes,
							Proxy->GetLocalToWorld(), Proxy->GetWorldToLocal());
						Slot = VisibleIndex;
					}

					const bool bOccludedByGpu = OccludedMask && OccludedMask[VisibleIndex];
					for (uint32 i = 0; i < CachedLOD.NumSections; ++i)
					{
						const FCachedMeshSection& Section = Proxy->GetSection(CachedLOD.FirstSection + i);
						Out.LODTriangles[LOD] += Section.IndexCount / 3;

						if (bStaticGroups && Section.Material && Section.Material->BlendState == EBlendState::Opaque)
						{
							FStaticDrawGroup& Group = FindGroup(Section.Material, Mesh, static_cast<uint8>(LOD));
							Group.Items.push_back({ Proxy, Slot, Section.StartIndex, Section.IndexCount, bOccludedByGpu ? 1u : 0u });
							++Out.StaticDrawCount;
							continue;
						}

						FRenderPacket& Packet = Out.Packets.AddDefaulted_GetRef();   // 議곌컖 ?꾩슜 諛곗뿴??異붽?
						Packet.Proxy = Proxy;
						Packet.Mesh = Mesh;
						Packet.Material = Section.Material;
						Packet.StartIndex = Section.StartIndex;
						Packet.IndexCount = Section.IndexCount;
						Packet.LODIndex = static_cast<uint8>(LOD);
						Packet.Slot = Slot;
						Packet.bOccludedByGpu = bOccludedByGpu;
					}
				}
			});

		uint32 StaticDraws = 0;

		uint32 TotalPackets = 0;
		for (uint32 c = 0; c < ChunkCount; ++c)
			TotalPackets += GatherChunks[c].Packets.Num();

		RenderQueue.Reserve(TotalPackets + 256);             // ?먮┛ 寃쎈줈 紐??쎄컙 ?ъ쑀
		for (uint32 c = 0; c < ChunkCount; ++c)
			if (GatherChunks[c].Packets.Num() > 0)
				RenderQueue.Append(GatherChunks[c].Packets);  // 議곌컖 ?쒖꽌?濡??댁뼱 遺숈씠湲?

		for (uint32 c = 0; c < ChunkCount; ++c)
		{
			for (uint32 VisibleIndex : GatherChunks[c].SlowPathIndices)
			{
				FPrimitiveSceneProxy* Proxy = VisibleProxies[VisibleIndex];
				UPrimitiveComponent* Primitive = Proxy->GetComponent();
				if (!Primitive || !Primitive->IsVisible())
					continue;

				const uint32 FirstNew = RenderQueue.Num();

				// ?꾨줉??罹먯떆媛 ?녿뒗 ?ㅽ깭??硫붿떆??湲곗〈泥섎읆 LOD瑜?怨⑤씪 ?쒖텧?섍퀬, 洹??몃뒗 而댄룷?뚰듃??留↔릿??
				UStaticMeshComponent* StaticMeshComponent = LODView ? Cast<UStaticMeshComponent>(Primitive) : nullptr;
				if (StaticMeshComponent && StaticMeshComponent->GetStaticMesh())
				{
					const uint32 LOD = SelectedLODs[VisibleIndex];
					StaticMeshComponent->SubmitToRenderQueue(RenderQueue, LOD);
				}
				else
				{
					Primitive->SubmitToRenderQueue(RenderQueue, LODView ? *LODView : FViewContext{});
				}

				// continue ?놁씠 ??긽 ?ш린源뚯? ??????⑦궥???ъ쑀 移몄쓣 諛곗젙?쒕떎.
				for (uint32 p = FirstNew; p < RenderQueue.Num(); ++p)
				{
					if (!SlotDest || NextExtraSlot >= MaxSlots) break;
					FRenderPacket& Packet = RenderQueue[p];
					const FMatrix& Model = Packet.Proxy ? Packet.Proxy->GetLocalToWorld() : Packet.Model ? *Packet.Model : FMatrix::Identity;
					const FMatrix WorldToLocal = Packet.Proxy ? Packet.Proxy->GetWorldToLocal() : Model.Inverse();
					WritePerObjectConstants(SlotDest + size_t(NextExtraSlot) * ObjectSlotBytes, Model, WorldToLocal);
					Packet.Slot = NextExtraSlot++;
				}
			}
		}

		for (uint32 c = 0; c < ChunkCount; ++c)
			for (uint32 L = 0; L < 4; ++L)
			{
				RenderStats.LODCounts[L] += GatherChunks[c].LODCounts[L];
				RenderStats.LODTriangles[L] += GatherChunks[c].LODTriangles[L];
			}

		if (SlotDest) Renderer->EndObjectConstants();
		RenderStats.DrawCalls = RenderQueue.Num() + StaticDraws;
		for (uint64 T : RenderStats.LODTriangles) RenderStats.Triangles += T;
	}
}

//void UWorld::GatherRenderPackets(TArray<FRenderPacket>& RenderArray, const FLODViewContext* LODView, const FFrustumPlanes* Frustum)
//{
//	for (TObjectIterator<UPrimitiveComponent> Itr; Itr; ++Itr)
//	{
//		if (!*Itr || !Itr->IsVisible())
//			continue;
//
//		if (Frustum)
//		{
//			const FBox Box = Itr->CalcBounds();
//			const FAABB Bounds{
//				(Box.Min + Box.Max) * 0.5f,
//				(Box.Max - Box.Min) * 0.5f
//			};
//			if (!IsAABBInFrustum(Bounds, *Frustum))
//				continue;
//		}
//
//		if (LODView)
//		{
//			if (auto* Component = Cast<UStaticMeshComponent>(*Itr))
//			{
//				if (UStaticMesh* Mesh =
//					Component->GetStaticMesh())
//				{
//					const uint32 LOD = SelectStaticMeshLOD(
//						*Mesh,
//						Component->GetWorldMatrix(),
//						*LODView);
//
//					Component->SubmitToRenderQueue(RenderQueue, LOD);
//					continue;
//				}
//			}
//		}
//
//		Itr->SubmitToRenderQueue(RenderQueue);
//	}
//}

// 硫붿씤 移대찓???앹꽦
void UWorld::CreateMainCamera()
{
	if (MainCamera)
		return;

	MainCamera = NewObject<ACameraActor>();

	if (!MainCamera)
	{
		HTR_LOG(Error, "Failed to create MainCamera");
		return;
	}

	MainCamera->World = this;
	MainCamera->Level = nullptr;
	MainCamera->GetCameraComponent()->SetRelativeLocation(FVector(-5.0f, -5.0f, 5.0f));

	// 硫붿씤 移대찓?쇰뒗 Level???랁븯吏 ?딆븘 BeginPlay瑜?嫄곗튂吏 ?딆쑝誘濡??ш린???깅줉?쒕떎.
	MainCamera->RegisterAllActorTickFunctions(true);
}

void UWorld::SetMainCamera(ACameraActor* Camera)
{
	if (MainCamera == Camera)
		return;

	if (MainCamera)
		MainCamera->RegisterAllActorTickFunctions(false);

	MainCamera = Camera;

	if (MainCamera)
	{
		MainCamera->World = this;
		MainCamera->RegisterAllActorTickFunctions(true);
	}
}

int32 UWorld::GetActorNum()
{
	return PersistentLevel->GetActorNum();
}

bool UWorld::DestroyActor(AActor* Actor)
{
	if (!Actor)
		return false;

	ULevel* Level = Actor->GetLevel();

	if (!Level)
		return false;

	// 1. BeginPlay ?湲곗뿴?먯꽌 ?쒓굅
	TQueue<AActor*> NewBeginPlayList;

	while (!BeginPlayList.IsEmpty())
	{
		AActor* PendingActor = BeginPlayList.Peek();
		BeginPlayList.Dequeue();

		if (PendingActor != Actor)
		{
			NewBeginPlayList.Enqueue(PendingActor);
		}
	}

	BeginPlayList = std::move(NewBeginPlayList);

	// 2. PathTracker?먯꽌 ?쒓굅
	PathTracker.OnObjectDestroyed(Actor);

	//// 3. PrimitiveComponents?먯꽌 ?쒓굅
	//for (UActorComponent* Component : Actor->Components)
	//{
	//	UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component);

	//	if (!Primitive)
	//		continue;

	//	for (int32 i = PrimitiveComponents.Num() - 1; i >= 0; --i)
	//	{
	//		if (PrimitiveComponents[i] == Primitive)
	//		{
	//			PrimitiveComponents.RemoveAt(i, 1);
	//			break;
	//		}
	//	}
	//}

	// 4. Level??Actors?먯꽌 ?쒓굅
	for (int32 i = Level->Actors.Num() - 1; i >= 0; --i)
	{
		if (Level->Actors[i] == Actor)
		{
			Level->Actors.RemoveAt(i, 1);
			break;
		}
	}

	FString ActorName = Actor->GetName();
	uint32 ActorUUID = Actor->GetUUID();

	// 5. ?꾨줉???쒓굅
	Actor->UnregisterAllComponents();

	Actor->RegisterAllActorTickFunctions(false);

	// 6. Actor ??젣
	delete Actor;

	HTR_LOG(Info, "Destroy Actor : {} UUID {}", ActorName, ActorUUID);

	return true;
}

// ?ㅻⅨ World??媛앹껜瑜??쒖쇅?섍퀬 Component 援먯감 以?理쒓렐??寃곌낵瑜??좏깮?쒕떎.
bool UWorld::SweepSingle(const FRay& WorldRay, float MaxDistance, float Radius, FHitResult& OutHit, AActor* IgnoreActor)
{
	OutHit = FHitResult(1.0f);
	if (MaxDistance <= 0.0f || !std::isfinite(MaxDistance) || Radius < 0.0f || !std::isfinite(Radius)) return false;
	Scene.UpdateAllTransforms();
	float NearestT = MaxDistance;
	const FVector Extent(Radius, Radius, Radius);
	const FPreparedRay PreparedRay(WorldRay);
	Scene.BVH.TraceClosest(
		[&](const FBox& Bounds, float& OutEnterT)
		{
			return RayIntersectsAABB(PreparedRay, Bounds.Min - Extent, Bounds.Max + Extent, OutEnterT);
		},
		[&](FPrimitiveSceneProxy* Proxy, float& InOutNearestT)
		{
			UPrimitiveComponent* Component = Proxy ? Proxy->GetComponent() : nullptr;
			if (!Component || !Component->IsShown(true) ||
				!Component->GetOwner() || !Component->GetOwner()->GetActorEnableCollision() ||
				Component->GetOwner() == IgnoreActor || Component->GetOwner()->GetWorld() != this)
				return false;
			if (!Proxy->GetMesh())
			{
				const FMatrix& WorldToLocal = Proxy->GetWorldToLocal();
				const FRay LocalRay{
					.Origin = WorldToLocal.TransformPosition(WorldRay.Origin),
					.Direction = WorldToLocal.TransformVector(WorldRay.Direction)
				};
				float T = InOutNearestT;
				if (!Component->LineTraceComponentLocal(LocalRay, T) || T > InOutNearestT) return false;
				OutHit.bBlockingHit = true;
				OutHit.HitComponent = Component;
				OutHit.Distance = T;
				OutHit.Time = T / MaxDistance;
				OutHit.ImpactPoint = WorldRay.Origin + WorldRay.Direction * T;
				OutHit.Location = OutHit.ImpactPoint;
				OutHit.ImpactNormal = -WorldRay.Direction;
				OutHit.Normal = OutHit.ImpactNormal;
				InOutNearestT = T;
				return true;
			}

			FAABB Bounds = Proxy->GetBounds();
			Bounds.Extent += Extent;
			float T = InOutNearestT;
			if (!RayIntersectsAABB(WorldRay, Bounds.Center - Bounds.Extent, Bounds.Center + Bounds.Extent, T) ||
				T > InOutNearestT) return false;

			const FVector Point = WorldRay.Origin + WorldRay.Direction * T;
			const FVector Offset = Point - Bounds.Center;
			const float X = Bounds.Extent.X > 0.0f ? Offset.X / Bounds.Extent.X : 0.0f;
			const float Y = Bounds.Extent.Y > 0.0f ? Offset.Y / Bounds.Extent.Y : 0.0f;
			const float Z = Bounds.Extent.Z > 0.0f ? Offset.Z / Bounds.Extent.Z : 0.0f;
			const float AX = std::abs(X), AY = std::abs(Y), AZ = std::abs(Z);
			FVector Normal = AX >= AY && AX >= AZ ? FVector(X >= 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f)
				: AY >= AZ ? FVector(0.0f, Y >= 0.0f ? 1.0f : -1.0f, 0.0f)
				: FVector(0.0f, 0.0f, Z >= 0.0f ? 1.0f : -1.0f);
			if (Normal.Dot(WorldRay.Direction) > 0.0f) return false;

			OutHit.bBlockingHit = true;
			OutHit.HitComponent = Component;
			OutHit.Distance = T;
			OutHit.Time = T / MaxDistance;
			OutHit.ImpactPoint = Point;
			OutHit.Location = Point;
			OutHit.ImpactNormal = Normal;
			OutHit.Normal = Normal;
			InOutNearestT = T;
			return true;
		}, NearestT);
	return OutHit.bBlockingHit;
}

bool UWorld::LineTraceSingle(const FRay& WorldRay, FHitResult& OutHit,
	FBillboardTraceTransform ResolveBillboard, const void* ViewContext)
{
	SCOPE_CYCLE_COUNTER_ALWAYS(EditorStats::STAT_PickingTime_Name);
	OutHit = FHitResult();
	float NearestT = std::numeric_limits<float>::max();

	const auto TraceComponent = [&](FPrimitiveSceneProxy* Proxy, float& InOutNearestT)
		{
			if (UStaticMesh* Mesh = Proxy ? Proxy->GetMesh() : nullptr)
			{
				if (!Proxy->IsVisible())
					return false;

				const FMatrix& WorldToLocal = Proxy->GetWorldToLocal();
				const FRay LocalRay{
					.Origin = WorldToLocal.TransformPosition(WorldRay.Origin),
					.Direction = WorldToLocal.TransformVector(WorldRay.Direction)
				};

				float T = InOutNearestT;
				if (!RayIntersectsMesh(LocalRay, Mesh->GetMeshData(), T))
					return false;

				OutHit.HitComponent = Proxy->GetComponent();
				OutHit.Distance = T;
				OutHit.ImpactPoint = WorldRay.Origin + WorldRay.Direction * T;
				InOutNearestT = T;
				return true;
			}

			UPrimitiveComponent* Component = Proxy ? Proxy->GetComponent() : nullptr;

			if (!Component || !Component->IsVisible())
				return false;

			if (UBillboardComponent* Billboard = Cast<UBillboardComponent>(Component))
			{
				if (!ResolveBillboard)
				{
					FHitResult Hit;
					if (!Billboard->LineTraceComponent(WorldRay, Hit) ||
						Hit.Distance >= InOutNearestT)
					{
						return false;
					}

					OutHit = Hit;
					InOutNearestT = Hit.Distance;
					return true;
				}

				const FMatrix BillboardToWorld = ResolveBillboard(*Billboard, ViewContext);

				const FRay LocalRay = ToLocalRay(WorldRay, BillboardToWorld);

				float T = InOutNearestT;
				if (!Billboard->LineTraceComponentLocal(LocalRay, T))
				{
					return false;
				}

				OutHit.HitComponent = Billboard;
				OutHit.Distance = T;
				OutHit.ImpactPoint = WorldRay.Origin + WorldRay.Direction * T;
				InOutNearestT = T;
				return true;
			}

			const FMatrix& WorldToLocal = Proxy->GetWorldToLocal();
			const FRay LocalRay{
				.Origin = WorldToLocal.TransformPosition(WorldRay.Origin),
				.Direction = WorldToLocal.TransformVector(WorldRay.Direction)
			};

			float T = InOutNearestT;
			if (!Component->LineTraceComponentLocal(LocalRay, T))
			{
				return false;
			}

			OutHit.HitComponent = Component;
			OutHit.Distance = T;
			OutHit.ImpactPoint = WorldRay.Origin + WorldRay.Direction * T;
			InOutNearestT = T;
			return true;
		};

	const FPreparedRay PreparedRay(WorldRay);

	Scene.BVH.TraceClosest(
		[&](const FBox& Bounds, float& OutEnterT) { return RayIntersectsAABB(PreparedRay, Bounds.Min, Bounds.Max, OutEnterT); },
		[&](FPrimitiveSceneProxy* Proxy, float& OutNearestT) { return TraceComponent(Proxy, OutNearestT); },
		NearestT);

	return OutHit.HitComponent != nullptr;
}

void UWorld::BeginPlay()
{
	if (WorldType != EWorldType::PIE || bBegunPlay) return;
	/*
	if (!PlayerPawn)
	{
		for (AActor* Actor : PersistentLevel->GetActors())
		{
			if (APawn* Pawn = Cast<APawn>(Actor)) { PlayerPawn = Pawn; break; }
		}
	}
	if (!PlayerPawn)
	{
		FTransform Transform = MainCamera ? MainCamera->GetActorTransform() : FTransform::Identity;
		PlayerPawn = SpawnActor<ADefaultPawn>(NAME_None, &Transform);
	}
	if (!PlayerController) PlayerController = SpawnActor<APlayerController>();
	if (PlayerController && PlayerPawn) PlayerController->Possess(PlayerPawn);
	*/

	// 湲곗〈 ?≫꽣 紐⑸줉??怨좎젙?????쒖옉?쒕떎. BeginPlay 以??앹꽦???≫꽣???湲곗뿴濡?媛꾨떎.
	TArray<AActor*> InitialActors;
	for (ULevel* Level : Levels)
		if (Level)
			for (AActor* Actor : Level->GetActors())
				if (Actor) InitialActors.Add(Actor);
	bBegunPlay = true;
	for (AActor* Actor : InitialActors)
		Actor->BeginPlay();

	// PIE ?낅젰 ?꾩뿉 DefaultPawn???ъ궗泥??源뚯? ?쒖옉?쒕떎.
	while (!BeginPlayList.IsEmpty())
	{
		AActor* Actor = BeginPlayList.Peek();
		BeginPlayList.Dequeue();
		if (Actor) Actor->BeginPlay();
	}
}

void UWorld::EndPlay()
{
	bBegunPlay = false;
	if (PlayerController)
	{
		PlayerController->ClearInputState();
		PlayerController->UnPossess();
	}
	PlayerController = nullptr;
	PlayerPawn = nullptr;
	bIsPaused = false;
}

UWorld* UWorld::CreateWorld(const EWorldType InWorldType, bool bInformEngineOfWorld, FName WorldName)
{
	UWorld* NewWorld = NewObject<UWorld>(nullptr, WorldName);
	NewWorld->SetFlags(EObjectFlags::RF_Transactional);
	NewWorld->WorldType = InWorldType;
	NewWorld->Init();

	return NewWorld;
}

UWorld* UWorld::GetDuplicatedWorldForPIE(UWorld * InWorld)
{
	if (!InWorld)
	{
		return nullptr;
	}

	UWorld* PIEWorld = CreateWorld(EWorldType::PIE, false);

	FObjectDuplicationParameters Params(InWorld, nullptr);
	Params.DuplicationSeed.Add(InWorld, PIEWorld);
	Params.DuplicationSeed.Add(InWorld->GetPersistentLevel(), PIEWorld->GetPersistentLevel());
	Params.PortFlags = EPropertyPortFlags::PPF_DuplicateForPIE;

	StaticDuplicateObjectEx(Params);
	return PIEWorld;
}

void UWorld::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	// ?꾩쭅 ?꾩슂 ?녿뒗?좊뱾
	//TArray<UObject*> ObjectsToFixReferences;
	//TMap<UObject*, UObject*> ReplacementMap;

	if (!bDuplicateForPIE)
	{
		assert(PersistentLevel);

		// Update the persistent level's owning world. This is needed for some initialization
		if (!PersistentLevel->OwningWorld)
		{
			PersistentLevel->OwningWorld = this;
		}
	}

	for (AActor* Actor : PersistentLevel->GetActors())
	{
		if (!Actor)
			continue;

		for (UActorComponent* Component : Actor->GetComponents())
			if (USceneComponent* SceneComp = Cast<USceneComponent>(Component))
				SceneComp->MarkTransformDirty();

		Actor->RegisterAllComponents();

	}

	Scene.BuildBVH();
}

void UWorld::Serialize(FStructuredArchive::FRecord Record)
{
	Super::Serialize(Record);   // "Properties"

	FArchive& Ar = Record.GetUnderlyingArchive();
	PersistentLevel->Serialize(Record.EnterRecord("PersistentLevel"));

	// (?좏깮) 硫붿씤 移대찓?쇰뒗 ?덈꺼???랁븯吏 ?딆븘 ?곕줈 ?대떎. PIE ?쒖옉 ?쒖젏?대굹 ?먮뵒???쒖젏??蹂듭썝?????????덈떎.
	// MainCamera->GetCameraComponent()->Serialize(Record.EnterRecord("MainCamera"));
}
