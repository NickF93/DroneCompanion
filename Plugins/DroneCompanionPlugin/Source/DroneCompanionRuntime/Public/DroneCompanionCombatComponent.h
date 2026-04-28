#pragma once

#include "Components/ActorComponent.h"
#include "Engine/HitResult.h"
#include "Math/Vector.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "DroneCompanionCombatComponent.generated.h"

class AActor;
class UDroneCompanionConfigDataAsset;
class USceneComponent;

// Encapsulates the drone's simple hitscan weapon.
UCLASS(ClassGroup = (DroneCompanion))
class DRONECOMPANIONRUNTIME_API UDroneCompanionCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDroneCompanionCombatComponent();

	void InitializeCombat(USceneComponent* InMuzzlePoint, UDroneCompanionConfigDataAsset* InConfig);

	bool CanFire() const;
	bool IsTargetInRange(AActor* TargetActor) const;
	bool HasClearShot(AActor* TargetActor) const;
	bool TryFireAtTarget(AActor* TargetActor);

private:
	FVector GetFireStartLocation() const;
	float GetAttackRange() const;
	float GetFireCooldown() const;
	float GetCombatDamage() const;
	bool ShouldApplyDamageOnHit() const;
	bool ShouldDrawCombatDebug() const;
	bool TraceTowardTarget(AActor* TargetActor, FHitResult& OutHitResult, FVector& OutStartLocation, FVector& OutEndLocation) const;

	TWeakObjectPtr<USceneComponent> MuzzlePoint;
	TWeakObjectPtr<UDroneCompanionConfigDataAsset> Config;
	float NextAllowedFireTime = 0.0f;
};
