#include "DroneCompanionFollowComponent.h"

#include "DrawDebugHelpers.h"
#include "DroneCompanionConfigDataAsset.h"
#include "GameFramework/Actor.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Vector.h"

namespace DroneCompanionFollowDefaults
{
	constexpr float FollowDistance = 300.0f;
	constexpr float FollowHeight = 150.0f;
	constexpr float MoveSpeed = 600.0f;
	constexpr float AcceptanceRadius = 25.0f;
}

UDroneCompanionFollowComponent::UDroneCompanionFollowComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UDroneCompanionFollowComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	AActor* Target = GetFollowTarget();
	if (!bFollowEnabled || !Owner || !Target)
	{
		return;
	}

	const UDroneCompanionConfigDataAsset* ConfigAsset = Config.Get();
	const float FollowDistance = ConfigAsset ? ConfigAsset->FollowDistance : DroneCompanionFollowDefaults::FollowDistance;
	const float FollowHeight = ConfigAsset ? ConfigAsset->FollowHeight : DroneCompanionFollowDefaults::FollowHeight;
	const float MoveSpeed = FMath::Max(ConfigAsset ? ConfigAsset->MoveSpeed : DroneCompanionFollowDefaults::MoveSpeed, 0.0f);
	const float AcceptanceRadius = FMath::Max(ConfigAsset ? ConfigAsset->AcceptanceRadius : DroneCompanionFollowDefaults::AcceptanceRadius, 0.0f);

	const FVector DesiredLocation = Target->GetActorLocation()
		- Target->GetActorForwardVector() * FollowDistance
		+ FVector(0.0f, 0.0f, FollowHeight);

	const FVector CurrentLocation = Owner->GetActorLocation();
	if (FVector::DistSquared(CurrentLocation, DesiredLocation) > FMath::Square(AcceptanceRadius))
	{
		const FVector NewLocation = FMath::VInterpConstantTo(CurrentLocation, DesiredLocation, DeltaTime, MoveSpeed);
		Owner->SetActorLocation(NewLocation, false, nullptr, ETeleportType::None);
	}

	if (ConfigAsset && ConfigAsset->bEnableFollowDebug)
	{
		if (UWorld* World = GetWorld())
		{
			DrawDebugSphere(World, DesiredLocation, 20.0f, 12, FColor::Cyan, false, 0.0f);
			DrawDebugLine(World, Owner->GetActorLocation(), DesiredLocation, FColor::Cyan, false, 0.0f);
		}
	}
}

void UDroneCompanionFollowComponent::SetFollowTarget(AActor* NewTarget)
{
	FollowTarget = NewTarget;
}

AActor* UDroneCompanionFollowComponent::GetFollowTarget() const
{
	return FollowTarget.Get();
}

void UDroneCompanionFollowComponent::ClearFollowTarget()
{
	FollowTarget.Reset();
}

bool UDroneCompanionFollowComponent::HasValidFollowTarget() const
{
	return FollowTarget.IsValid();
}

void UDroneCompanionFollowComponent::SetConfig(UDroneCompanionConfigDataAsset* NewConfig)
{
	Config = NewConfig;
}

void UDroneCompanionFollowComponent::SetFollowEnabled(bool bEnabled)
{
	bFollowEnabled = bEnabled;
}

bool UDroneCompanionFollowComponent::IsFollowEnabled() const
{
	return bFollowEnabled;
}
