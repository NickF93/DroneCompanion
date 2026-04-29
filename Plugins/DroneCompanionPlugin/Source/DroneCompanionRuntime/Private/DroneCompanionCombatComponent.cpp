#include "DroneCompanionCombatComponent.h"

#include "CollisionQueryParams.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "DroneCompanionConfigDataAsset.h"
#include "DroneCompanionRuntimeModule.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Vector.h"
#include "UObject/UObjectGlobals.h"

namespace DroneCompanionCombatDefaults
{
	constexpr float AttackRange = 1200.0f;
	constexpr float FireCooldown = 0.5f;
	constexpr float CombatDamage = 10.0f;
	constexpr bool bApplyDamageOnHit = true;

	float PositiveOrDefault(float Value, float DefaultValue)
	{
		return Value > 0.0f ? Value : DefaultValue;
	}
}

UDroneCompanionCombatComponent::UDroneCompanionCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDroneCompanionCombatComponent::InitializeCombat(USceneComponent* InMuzzlePoint, UDroneCompanionConfigDataAsset* InConfig)
{
	MuzzlePoint = InMuzzlePoint;
	Config = InConfig;
}

bool UDroneCompanionCombatComponent::CanFire() const
{
	const UWorld* World = GetWorld();
	return World && World->GetTimeSeconds() >= NextAllowedFireTime;
}

bool UDroneCompanionCombatComponent::IsTargetInRange(AActor* TargetActor) const
{
	if (!IsValid(GetOwner()) || !IsValid(TargetActor))
	{
		return false;
	}

	const float AttackRange = GetAttackRange();
	return FVector::DistSquared(GetFireStartLocation(), TargetActor->GetActorLocation()) <= FMath::Square(AttackRange);
}

bool UDroneCompanionCombatComponent::HasClearShot(AActor* TargetActor) const
{
	FHitResult HitResult;
	FVector StartLocation;
	FVector EndLocation;
	if (!TraceTowardTarget(TargetActor, HitResult, StartLocation, EndLocation))
	{
		return false;
	}

	return !HitResult.bBlockingHit || HitResult.GetActor() == TargetActor;
}

bool UDroneCompanionCombatComponent::TryFireAtTarget(AActor* TargetActor)
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !IsValid(Owner) || !IsValid(TargetActor) || !CanFire() || !IsTargetInRange(TargetActor))
	{
		return false;
	}

	FHitResult HitResult;
	FVector StartLocation;
	FVector EndLocation;
	if (!TraceTowardTarget(TargetActor, HitResult, StartLocation, EndLocation))
	{
		return false;
	}

	NextAllowedFireTime = World->GetTimeSeconds() + GetFireCooldown();

	const bool bHitTarget = HitResult.bBlockingHit && HitResult.GetActor() == TargetActor;
	const bool bBlocked = HitResult.bBlockingHit && HitResult.GetActor() != TargetActor;
	const bool bDrawDebug = ShouldDrawCombatDebug();

	if (bHitTarget)
	{
		if (ShouldApplyDamageOnHit() && IsValid(TargetActor))
		{
			UGameplayStatics::ApplyDamage(TargetActor, GetCombatDamage(), Owner->GetInstigatorController(), Owner, nullptr);
		}

		if (bDrawDebug)
		{
			DrawDebugLine(World, StartLocation, HitResult.ImpactPoint, FColor::Red, false, GetFireCooldown());
			DrawDebugSphere(World, HitResult.ImpactPoint, 16.0f, 12, FColor::Red, false, GetFireCooldown());
			UE_LOG(LogDroneCompanion, Log, TEXT("%s hit enemy target %s."), *GetName(), *GetNameSafe(TargetActor));
		}

		return true;
	}

	if (bDrawDebug)
	{
		const FVector DebugEnd = HitResult.bBlockingHit ? HitResult.ImpactPoint : EndLocation;
		const FColor DebugColor = bBlocked ? FColor::Orange : FColor::Silver;
		DrawDebugLine(World, StartLocation, DebugEnd, DebugColor, false, GetFireCooldown());

		if (bBlocked)
		{
			UE_LOG(LogDroneCompanion, Log, TEXT("%s shot at %s was blocked by %s."), *GetName(), *GetNameSafe(TargetActor), *GetNameSafe(HitResult.GetActor()));
		}
	}

	return false;
}

FVector UDroneCompanionCombatComponent::GetFireStartLocation() const
{
	if (const USceneComponent* Muzzle = MuzzlePoint.Get())
	{
		return Muzzle->GetComponentLocation();
	}

	if (const AActor* Owner = GetOwner())
	{
		return Owner->GetActorLocation();
	}

	return FVector::ZeroVector;
}

float UDroneCompanionCombatComponent::GetAttackRange() const
{
	const UDroneCompanionConfigDataAsset* ConfigAsset = Config.Get();
	return ConfigAsset
		? DroneCompanionCombatDefaults::PositiveOrDefault(ConfigAsset->AttackRange, DroneCompanionCombatDefaults::AttackRange)
		: DroneCompanionCombatDefaults::AttackRange;
}

float UDroneCompanionCombatComponent::GetFireCooldown() const
{
	const UDroneCompanionConfigDataAsset* ConfigAsset = Config.Get();
	return FMath::Max(ConfigAsset ? ConfigAsset->FireCooldown : DroneCompanionCombatDefaults::FireCooldown, 0.0f);
}

float UDroneCompanionCombatComponent::GetCombatDamage() const
{
	const UDroneCompanionConfigDataAsset* ConfigAsset = Config.Get();
	return FMath::Max(ConfigAsset ? ConfigAsset->CombatDamage : DroneCompanionCombatDefaults::CombatDamage, 0.0f);
}

bool UDroneCompanionCombatComponent::ShouldApplyDamageOnHit() const
{
	const UDroneCompanionConfigDataAsset* ConfigAsset = Config.Get();
	return ConfigAsset ? ConfigAsset->bApplyDamageOnHit : DroneCompanionCombatDefaults::bApplyDamageOnHit;
}

bool UDroneCompanionCombatComponent::ShouldDrawCombatDebug() const
{
	const UDroneCompanionConfigDataAsset* ConfigAsset = Config.Get();
	return ConfigAsset && ConfigAsset->bEnableCombatDebug;
}

bool UDroneCompanionCombatComponent::TraceTowardTarget(AActor* TargetActor, FHitResult& OutHitResult, FVector& OutStartLocation, FVector& OutEndLocation) const
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!IsValid(Owner) || !World || !IsValid(TargetActor))
	{
		return false;
	}

	OutStartLocation = GetFireStartLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();
	const FVector FireDirection = (TargetLocation - OutStartLocation).GetSafeNormal();
	if (FireDirection.IsNearlyZero())
	{
		return false;
	}

	OutEndLocation = OutStartLocation + FireDirection * GetAttackRange();

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DroneCompanionCombatTrace), false, Owner);
	QueryParams.AddIgnoredActor(Owner);

	World->LineTraceSingleByChannel(
		OutHitResult,
		OutStartLocation,
		OutEndLocation,
		ECC_Visibility,
		QueryParams);

	return true;
}
