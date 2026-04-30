#include "Components/DroneCompanionMovementComponent.h"

#include "Components/SceneComponent.h"
#include "Core/DroneCompanionConfigDataAsset.h"
#include "DrawDebugHelpers.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "Module/DroneCompanionRuntimeModule.h"
#include "Math/UnrealMathUtility.h"

namespace DroneCompanionMovementDefaults
{
	constexpr float DebugTargetRadius = 18.0f;
	constexpr float DebugHitNormalLength = 90.0f;
}

UDroneCompanionMovementComponent::UDroneCompanionMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDroneCompanionMovementComponent::InitializeMovement(USceneComponent* InUpdatedComponent, UDroneCompanionConfigDataAsset* InConfig)
{
	SetUpdatedComponent(InUpdatedComponent);
	Config = InConfig;
	bLoggedMissingUpdatedComponent = false;

	if (!InUpdatedComponent)
	{
		UE_LOG(LogDroneCompanion, Warning, TEXT("%s could not initialize drone movement because UpdatedComponent is missing."), *GetName());
		bLoggedMissingUpdatedComponent = true;
	}
}

bool UDroneCompanionMovementComponent::MoveTowardLocation(const FVector& DesiredLocation, float MaxSpeed, float AcceptanceRadius, float DeltaTime)
{
	if (!UpdatedComponent)
	{
		if (!bLoggedMissingUpdatedComponent)
		{
			UE_LOG(LogDroneCompanion, Warning, TEXT("%s cannot move because UpdatedComponent is missing."), *GetName());
			bLoggedMissingUpdatedComponent = true;
		}

		return false;
	}

	const float SafeAcceptanceRadius = FMath::Max(AcceptanceRadius, 0.0f);
	const FVector StartLocation = UpdatedComponent->GetComponentLocation();
	const FVector ToTarget = DesiredLocation - StartLocation;
	const float DistanceToTarget = ToTarget.Size();
	if (DistanceToTarget <= SafeAcceptanceRadius)
	{
		return true;
	}

	const float SafeMaxSpeed = FMath::Max(MaxSpeed, 0.0f);
	if (DeltaTime <= 0.0f || SafeMaxSpeed <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const float MoveDistance = FMath::Min(DistanceToTarget, SafeMaxSpeed * DeltaTime);
	const FVector MoveDelta = ToTarget.GetSafeNormal() * MoveDistance;

	FHitResult Hit;
	SafeMoveUpdatedComponent(MoveDelta, UpdatedComponent->GetComponentQuat(), true, Hit);

	if (Hit.IsValidBlockingHit())
	{
		SlideAlongSurface(MoveDelta, 1.0f - Hit.Time, Hit.Normal, Hit, true);
	}

	if (ShouldDrawMovementDebug())
	{
		if (UWorld* World = GetWorld())
		{
			const FVector EndLocation = UpdatedComponent->GetComponentLocation();
			DrawDebugSphere(World, DesiredLocation, DroneCompanionMovementDefaults::DebugTargetRadius, 12, FColor::Blue, false, 0.0f);
			DrawDebugLine(World, StartLocation, EndLocation, FColor::Blue, false, 0.0f);

			if (Hit.IsValidBlockingHit())
			{
				DrawDebugLine(
					World,
					Hit.ImpactPoint,
					Hit.ImpactPoint + Hit.Normal * DroneCompanionMovementDefaults::DebugHitNormalLength,
					FColor::Purple,
					false,
					0.0f);
			}
		}
	}

	return FVector::DistSquared(UpdatedComponent->GetComponentLocation(), DesiredLocation) <= FMath::Square(SafeAcceptanceRadius);
}

bool UDroneCompanionMovementComponent::ShouldDrawMovementDebug() const
{
	const UDroneCompanionConfigDataAsset* ConfigAsset = Config.Get();
	return ConfigAsset && ConfigAsset->bEnableMovementDebug;
}
