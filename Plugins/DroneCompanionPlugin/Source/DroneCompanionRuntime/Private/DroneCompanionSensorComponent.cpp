#include "DroneCompanionSensorComponent.h"

#include "CollisionQueryParams.h"
#include "Containers/Set.h"
#include "DrawDebugHelpers.h"
#include "DroneCompanionConfigDataAsset.h"
#include "DroneCompanionRuntimeModule.h"
#include "DroneCompanionTargetMarkerComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Vector.h"

namespace DroneCompanionSensorDefaults
{
	constexpr float ScanRadius = 1000.0f;
	constexpr float ScanInterval = 0.2f;
	constexpr float MinimumScanInterval = 0.01f;
	constexpr float EnemyBaseScore = 100.0f;
	constexpr float CollectibleBaseScore = 50.0f;
	constexpr float MaximumDistanceScore = 10.0f;
	constexpr bool bRequireLineOfSight = true;
}

UDroneCompanionSensorComponent::UDroneCompanionSensorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDroneCompanionSensorComponent::StartSensing()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!Config.IsValid())
	{
		UE_LOG(LogDroneCompanion, Warning, TEXT("%s has no Drone Companion config asset assigned. Sensor defaults will be used."), *GetName());
	}

	const float ScanInterval = GetScanInterval();
	World->GetTimerManager().ClearTimer(ScanTimerHandle);
	ScanForTargets();
	World->GetTimerManager().SetTimer(ScanTimerHandle, this, &UDroneCompanionSensorComponent::ScanForTargets, ScanInterval, true);

	UE_LOG(LogDroneCompanion, Log, TEXT("%s sensing started."), *GetName());
}

void UDroneCompanionSensorComponent::StopSensing()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ScanTimerHandle);
	}

	ScanTimerHandle.Invalidate();
	UE_LOG(LogDroneCompanion, Log, TEXT("%s sensing stopped."), *GetName());
}

void UDroneCompanionSensorComponent::ScanForTargets()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		UpdateBestTarget(FDroneCompanionTargetInfo(), false);
		return;
	}

	const FVector Origin = Owner->GetActorLocation();
	const float ScanRadius = GetScanRadius();
	const bool bRequireLineOfSight = ShouldRequireLineOfSight();

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DroneCompanionSensorOverlap), false, Owner);
	QueryParams.AddIgnoredActor(Owner);

	World->OverlapMultiByObjectType(
		Overlaps,
		Origin,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(ScanRadius),
		QueryParams);

	TSet<AActor*> ProcessedActors;
	TArray<FDroneCompanionTargetInfo> CandidateInfos;
	FDroneCompanionTargetInfo BestCandidate;
	bool bFoundBestCandidate = false;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!Candidate || Candidate == Owner || ProcessedActors.Contains(Candidate))
		{
			continue;
		}

		ProcessedActors.Add(Candidate);

		const UDroneCompanionTargetMarkerComponent* Marker = Candidate->FindComponentByClass<UDroneCompanionTargetMarkerComponent>();
		if (!Marker || !Marker->bIsDetectable || Marker->TargetType == EDroneCompanionTargetType::None)
		{
			continue;
		}

		const FVector CandidateLocation = Candidate->GetActorLocation();
		const float Distance = FVector::Dist(Origin, CandidateLocation);
		const bool bLineOfSightConfirmed = bRequireLineOfSight
			? HasLineOfSightToTarget(Owner, Candidate, CandidateLocation)
			: false;

		if (bRequireLineOfSight && !bLineOfSightConfirmed)
		{
			continue;
		}

		FDroneCompanionTargetInfo CandidateInfo;
		CandidateInfo.TargetActor = Candidate;
		CandidateInfo.TargetType = Marker->TargetType;
		CandidateInfo.LastKnownLocation = CandidateLocation;
		CandidateInfo.Distance = Distance;
		CandidateInfo.Score = CalculateScore(Marker->TargetType, Distance, Marker->PriorityBonus, ScanRadius);
		CandidateInfo.bLineOfSightConfirmed = bLineOfSightConfirmed;

		CandidateInfos.Add(CandidateInfo);

		if (!bFoundBestCandidate || CandidateInfo.Score > BestCandidate.Score)
		{
			BestCandidate = CandidateInfo;
			bFoundBestCandidate = true;
		}
	}

	UpdateBestTarget(BestCandidate, bFoundBestCandidate);

	if (ShouldDrawDebug())
	{
		DrawSensorDebug(Origin, ScanRadius, CandidateInfos);
	}
}

bool UDroneCompanionSensorComponent::HasBestTarget() const
{
	return bHasBestTarget && BestTargetInfo.TargetActor.IsValid();
}

AActor* UDroneCompanionSensorComponent::GetBestTargetActor() const
{
	return HasBestTarget() ? BestTargetInfo.TargetActor.Get() : nullptr;
}

EDroneCompanionTargetType UDroneCompanionSensorComponent::GetBestTargetType() const
{
	return HasBestTarget() ? BestTargetInfo.TargetType : EDroneCompanionTargetType::None;
}

bool UDroneCompanionSensorComponent::GetBestTargetInfo(FDroneCompanionTargetInfo& OutTargetInfo) const
{
	if (!HasBestTarget())
	{
		return false;
	}

	OutTargetInfo = BestTargetInfo;
	return true;
}

void UDroneCompanionSensorComponent::SetConfig(UDroneCompanionConfigDataAsset* NewConfig)
{
	Config = NewConfig;
}

float UDroneCompanionSensorComponent::GetScanRadius() const
{
	const UDroneCompanionConfigDataAsset* ConfigAsset = Config.Get();
	return FMath::Max(ConfigAsset ? ConfigAsset->ScanRadius : DroneCompanionSensorDefaults::ScanRadius, 0.0f);
}

float UDroneCompanionSensorComponent::GetScanInterval() const
{
	const UDroneCompanionConfigDataAsset* ConfigAsset = Config.Get();
	return FMath::Max(ConfigAsset ? ConfigAsset->ScanInterval : DroneCompanionSensorDefaults::ScanInterval, DroneCompanionSensorDefaults::MinimumScanInterval);
}

bool UDroneCompanionSensorComponent::ShouldRequireLineOfSight() const
{
	const UDroneCompanionConfigDataAsset* ConfigAsset = Config.Get();
	return ConfigAsset ? ConfigAsset->bRequireLineOfSightForDetection : DroneCompanionSensorDefaults::bRequireLineOfSight;
}

bool UDroneCompanionSensorComponent::ShouldDrawDebug() const
{
	const UDroneCompanionConfigDataAsset* ConfigAsset = Config.Get();
	return ConfigAsset && ConfigAsset->bEnableSensorDebug;
}

float UDroneCompanionSensorComponent::GetBaseScore(EDroneCompanionTargetType TargetType) const
{
	const UDroneCompanionConfigDataAsset* ConfigAsset = Config.Get();

	if (TargetType == EDroneCompanionTargetType::Enemy)
	{
		return FMath::Max(ConfigAsset ? ConfigAsset->EnemyBaseScore : DroneCompanionSensorDefaults::EnemyBaseScore, 0.0f);
	}

	if (TargetType == EDroneCompanionTargetType::Collectible)
	{
		return FMath::Max(ConfigAsset ? ConfigAsset->CollectibleBaseScore : DroneCompanionSensorDefaults::CollectibleBaseScore, 0.0f);
	}

	return 0.0f;
}

bool UDroneCompanionSensorComponent::HasLineOfSightToTarget(AActor* Owner, AActor* Candidate, FVector CandidateLocation) const
{
	UWorld* World = GetWorld();
	if (!World || !Owner || !Candidate)
	{
		return false;
	}

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DroneCompanionSensorLineOfSight), false, Owner);
	QueryParams.AddIgnoredActor(Owner);

	const bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		Owner->GetActorLocation(),
		CandidateLocation,
		ECC_Visibility,
		QueryParams);

	return !bHit || HitResult.GetActor() == Candidate;
}

float UDroneCompanionSensorComponent::CalculateScore(EDroneCompanionTargetType TargetType, float Distance, float PriorityBonus, float ScanRadius) const
{
	const float DistanceAlpha = ScanRadius > 0.0f ? FMath::Clamp(1.0f - (Distance / ScanRadius), 0.0f, 1.0f) : 0.0f;
	const float DistanceScore = DistanceAlpha * DroneCompanionSensorDefaults::MaximumDistanceScore;
	return GetBaseScore(TargetType) + PriorityBonus + DistanceScore;
}

void UDroneCompanionSensorComponent::UpdateBestTarget(const FDroneCompanionTargetInfo& NewBestTarget, bool bFoundTarget)
{
	const bool bHadStoredTarget = bHasBestTarget;
	AActor* PreviousTarget = BestTargetInfo.TargetActor.Get();
	const EDroneCompanionTargetType PreviousType = BestTargetInfo.TargetType;

	if (!bFoundTarget)
	{
		BestTargetInfo = FDroneCompanionTargetInfo();
		bHasBestTarget = false;

		if (bHadStoredTarget)
		{
			OnBestTargetLost.Broadcast();
			UE_LOG(LogDroneCompanion, Log, TEXT("%s lost its best target."), *GetName());
		}

		return;
	}

	BestTargetInfo = NewBestTarget;
	bHasBestTarget = true;

	AActor* NewTarget = BestTargetInfo.TargetActor.Get();
	if (!bHadStoredTarget || PreviousTarget != NewTarget || PreviousType != BestTargetInfo.TargetType)
	{
		OnBestTargetChanged.Broadcast(BestTargetInfo);
		UE_LOG(LogDroneCompanion, Log, TEXT("%s best target changed to %s."), *GetName(), *GetNameSafe(NewTarget));
	}
}

void UDroneCompanionSensorComponent::DrawSensorDebug(FVector Origin, float ScanRadius, const TArray<FDroneCompanionTargetInfo>& Candidates) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	DrawDebugSphere(World, Origin, ScanRadius, 32, FColor::Silver, false, GetScanInterval());

	for (const FDroneCompanionTargetInfo& Candidate : Candidates)
	{
		const FColor CandidateColor = Candidate.TargetType == EDroneCompanionTargetType::Enemy ? FColor::Red : FColor::Green;
		DrawDebugLine(World, Origin, Candidate.LastKnownLocation, CandidateColor, false, GetScanInterval());
		DrawDebugSphere(World, Candidate.LastKnownLocation, 20.0f, 12, CandidateColor, false, GetScanInterval());
	}

	if (HasBestTarget())
	{
		DrawDebugSphere(World, BestTargetInfo.LastKnownLocation, 40.0f, 16, FColor::Yellow, false, GetScanInterval());
	}
}
