#include "DroneCompanionBrainStates.h"

#include "DrawDebugHelpers.h"
#include "DroneCompanionBrainComponent.h"
#include "DroneCompanionConfigDataAsset.h"
#include "DroneCompanionFeedbackComponent.h"
#include "DroneCompanionFollowComponent.h"
#include "DroneCompanionTargetTypes.h"
#include "GameFramework/Actor.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Vector.h"

namespace DroneCompanionBrainStateNames
{
	const FName Follow(TEXT("Follow"));
	const FName InspectCollectible(TEXT("InspectCollectible"));
}

namespace DroneCompanionInspectionDefaults
{
	constexpr float MoveSpeed = 600.0f;
	constexpr float CollectibleHoverHeight = 120.0f;
	constexpr float InspectDuration = 2.0f;
	constexpr float InspectAcceptanceRadius = 40.0f;
}

void FDroneCompanionBrainStateDeleter::operator()(IDroneCompanionBrainState* State) const
{
	delete State;
}

void FDroneCompanionFollowState::Enter(UDroneCompanionBrainComponent& Brain)
{
	if (UDroneCompanionFollowComponent* Follow = Brain.GetFollowComponent())
	{
		Follow->SetFollowEnabled(true);
	}

	if (UDroneCompanionFeedbackComponent* Feedback = Brain.GetFeedbackComponent())
	{
		Feedback->SetIdleFeedback();
	}
}

void FDroneCompanionFollowState::Exit(UDroneCompanionBrainComponent& Brain)
{
}

void FDroneCompanionFollowState::Tick(UDroneCompanionBrainComponent& Brain, float DeltaTime)
{
	FDroneCompanionTargetInfo BestTargetInfo;
	if (!Brain.GetCachedBestTargetInfo(BestTargetInfo))
	{
		return;
	}

	if (Brain.ShouldInspectCollectible(BestTargetInfo))
	{
		Brain.TransitionToInspectCollectible(BestTargetInfo.TargetActor.Get());
	}
}

FName FDroneCompanionFollowState::GetName() const
{
	return DroneCompanionBrainStateNames::Follow;
}

FDroneCompanionInspectCollectibleState::FDroneCompanionInspectCollectibleState(AActor* InCollectibleTarget)
	: CollectibleTarget(InCollectibleTarget)
{
}

void FDroneCompanionInspectCollectibleState::Enter(UDroneCompanionBrainComponent& Brain)
{
	InspectElapsedTime = 0.0f;

	if (UDroneCompanionFollowComponent* Follow = Brain.GetFollowComponent())
	{
		Follow->SetFollowEnabled(false);
	}

	if (UDroneCompanionFeedbackComponent* Feedback = Brain.GetFeedbackComponent())
	{
		Feedback->PlayCollectibleFeedback();
	}

	Brain.LogInspectionStarted(CollectibleTarget.Get());
}

void FDroneCompanionInspectCollectibleState::Exit(UDroneCompanionBrainComponent& Brain)
{
	if (UDroneCompanionFeedbackComponent* Feedback = Brain.GetFeedbackComponent())
	{
		Feedback->StopCollectibleFeedback();
	}
}

void FDroneCompanionInspectCollectibleState::Tick(UDroneCompanionBrainComponent& Brain, float DeltaTime)
{
	AActor* DroneActor = Brain.GetDroneActor();
	AActor* TargetActor = CollectibleTarget.Get();
	if (!DroneActor || !TargetActor)
	{
		Brain.LogInspectionAborted(TEXT("collectible target is invalid"));
		Brain.TransitionToFollow();
		return;
	}

	FDroneCompanionTargetInfo BestTargetInfo;
	if (!Brain.GetCachedBestTargetInfo(BestTargetInfo))
	{
		Brain.LogInspectionAborted(TEXT("sensor lost all targets"));
		Brain.TransitionToFollow();
		return;
	}

	if (BestTargetInfo.TargetType == EDroneCompanionTargetType::Enemy)
	{
		Brain.LogInspectionAborted(TEXT("enemy became the best target"));
		Brain.TransitionToFollow();
		return;
	}

	const UDroneCompanionConfigDataAsset* Config = Brain.GetConfig();
	const float MoveSpeed = FMath::Max(Config ? Config->MoveSpeed : DroneCompanionInspectionDefaults::MoveSpeed, 0.0f);
	const float HoverHeight = Config ? Config->CollectibleHoverHeight : DroneCompanionInspectionDefaults::CollectibleHoverHeight;
	const float InspectDuration = FMath::Max(Config ? Config->InspectDuration : DroneCompanionInspectionDefaults::InspectDuration, 0.0f);
	const float AcceptanceRadius = FMath::Max(Config ? Config->InspectAcceptanceRadius : DroneCompanionInspectionDefaults::InspectAcceptanceRadius, 0.0f);

	const FVector DesiredLocation = TargetActor->GetActorLocation() + FVector(0.0f, 0.0f, HoverHeight);
	const FVector CurrentLocation = DroneActor->GetActorLocation();

	if (FVector::DistSquared(CurrentLocation, DesiredLocation) > FMath::Square(AcceptanceRadius))
	{
		const FVector NewLocation = FMath::VInterpConstantTo(CurrentLocation, DesiredLocation, DeltaTime, MoveSpeed);
		DroneActor->SetActorLocation(NewLocation, false, nullptr, ETeleportType::None);
	}
	else
	{
		InspectElapsedTime += DeltaTime;
		if (InspectElapsedTime >= InspectDuration)
		{
			Brain.MarkCollectibleInspectionCompleted(TargetActor);
			Brain.LogInspectionCompleted(TargetActor);
			Brain.TransitionToFollow();
			return;
		}
	}

	if (Brain.ShouldDrawInspectionDebug())
	{
		if (UWorld* World = DroneActor->GetWorld())
		{
			DrawDebugSphere(World, DesiredLocation, 28.0f, 12, FColor::Yellow, false, 0.0f);
			DrawDebugLine(World, DroneActor->GetActorLocation(), DesiredLocation, FColor::Yellow, false, 0.0f);
		}
	}
}

FName FDroneCompanionInspectCollectibleState::GetName() const
{
	return DroneCompanionBrainStateNames::InspectCollectible;
}
