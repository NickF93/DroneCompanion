#include "Brain/DroneCompanionBrainStates.h"

#include "DrawDebugHelpers.h"
#include "Components/DroneCompanionBrainComponent.h"
#include "Components/DroneCompanionCombatComponent.h"
#include "Components/DroneCompanionFeedbackComponent.h"
#include "Components/DroneCompanionFollowComponent.h"
#include "Core/DroneCompanionConfigDataAsset.h"
#include "Targets/DroneCompanionTargetTypes.h"
#include "GameFramework/Actor.h"
#include "Math/Rotator.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Vector.h"
#include "UObject/UObjectGlobals.h"

namespace DroneCompanionBrainStateNames
{
	const FName AttackEnemy(TEXT("AttackEnemy"));
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

namespace DroneCompanionAttackDefaults
{
	constexpr float AimInterpSpeed = 8.0f;
	constexpr float MaxFireAngleDegrees = 15.0f;
}

namespace DroneCompanionBrainValue
{
	float PositiveOrDefault(float Value, float DefaultValue)
	{
		return Value > 0.0f ? Value : DefaultValue;
	}
}

void FDroneCompanionBrainStateDeleter::operator()(IDroneCompanionBrainState* State) const
{
	delete State;
}

void IDroneCompanionBrainState::TransitionToFollow(UDroneCompanionBrainComponent& Brain)
{
	Brain.TransitionToFollow();
}

void IDroneCompanionBrainState::TransitionToInspectCollectible(UDroneCompanionBrainComponent& Brain, AActor* CollectibleTarget)
{
	Brain.TransitionToInspectCollectible(CollectibleTarget);
}

void IDroneCompanionBrainState::TransitionToAttackEnemy(UDroneCompanionBrainComponent& Brain, AActor* EnemyTarget)
{
	Brain.TransitionToAttackEnemy(EnemyTarget);
}

bool IDroneCompanionBrainState::GetCachedBestTargetInfo(const UDroneCompanionBrainComponent& Brain, FDroneCompanionTargetInfo& OutTargetInfo)
{
	return Brain.GetCachedBestTargetInfo(OutTargetInfo);
}

bool IDroneCompanionBrainState::ShouldInspectCollectible(const UDroneCompanionBrainComponent& Brain, const FDroneCompanionTargetInfo& TargetInfo)
{
	return Brain.ShouldInspectCollectible(TargetInfo);
}

bool IDroneCompanionBrainState::ShouldAttackEnemy(const UDroneCompanionBrainComponent& Brain, const FDroneCompanionTargetInfo& TargetInfo)
{
	return Brain.ShouldAttackEnemy(TargetInfo);
}

void IDroneCompanionBrainState::MarkCollectibleInspectionCompleted(UDroneCompanionBrainComponent& Brain, AActor* CollectibleTarget)
{
	Brain.MarkCollectibleInspectionCompleted(CollectibleTarget);
}

AActor* IDroneCompanionBrainState::GetDroneActor(const UDroneCompanionBrainComponent& Brain)
{
	return Brain.GetDroneActor();
}

UDroneCompanionConfigDataAsset* IDroneCompanionBrainState::GetConfig(const UDroneCompanionBrainComponent& Brain)
{
	return Brain.GetConfig();
}

UDroneCompanionFollowComponent* IDroneCompanionBrainState::GetFollowComponent(const UDroneCompanionBrainComponent& Brain)
{
	return Brain.GetFollowComponent();
}

UDroneCompanionFeedbackComponent* IDroneCompanionBrainState::GetFeedbackComponent(const UDroneCompanionBrainComponent& Brain)
{
	return Brain.GetFeedbackComponent();
}

UDroneCompanionCombatComponent* IDroneCompanionBrainState::GetCombatComponent(const UDroneCompanionBrainComponent& Brain)
{
	return Brain.GetCombatComponent();
}

bool IDroneCompanionBrainState::ShouldDrawInspectionDebug(const UDroneCompanionBrainComponent& Brain)
{
	return Brain.ShouldDrawInspectionDebug();
}

void IDroneCompanionBrainState::LogInspectionStarted(const UDroneCompanionBrainComponent& Brain, AActor* CollectibleTarget)
{
	Brain.LogInspectionStarted(CollectibleTarget);
}

void IDroneCompanionBrainState::LogInspectionCompleted(const UDroneCompanionBrainComponent& Brain, AActor* CollectibleTarget)
{
	Brain.LogInspectionCompleted(CollectibleTarget);
}

void IDroneCompanionBrainState::LogInspectionAborted(const UDroneCompanionBrainComponent& Brain, const TCHAR* Reason)
{
	Brain.LogInspectionAborted(Reason);
}

void IDroneCompanionBrainState::LogAttackStarted(const UDroneCompanionBrainComponent& Brain, AActor* EnemyTarget)
{
	Brain.LogAttackStarted(EnemyTarget);
}

void IDroneCompanionBrainState::LogAttackExited(const UDroneCompanionBrainComponent& Brain, AActor* EnemyTarget)
{
	Brain.LogAttackExited(EnemyTarget);
}

void IDroneCompanionBrainState::LogAttackAborted(const UDroneCompanionBrainComponent& Brain, const TCHAR* Reason)
{
	Brain.LogAttackAborted(Reason);
}

void FDroneCompanionFollowState::Enter(UDroneCompanionBrainComponent& Brain)
{
	if (UDroneCompanionFollowComponent* Follow = GetFollowComponent(Brain))
	{
		Follow->SetFollowEnabled(true);
	}

	if (UDroneCompanionFeedbackComponent* Feedback = GetFeedbackComponent(Brain))
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
	if (!GetCachedBestTargetInfo(Brain, BestTargetInfo))
	{
		return;
	}

	if (ShouldAttackEnemy(Brain, BestTargetInfo))
	{
		TransitionToAttackEnemy(Brain, BestTargetInfo.TargetActor.Get());
		return;
	}

	if (ShouldInspectCollectible(Brain, BestTargetInfo))
	{
		TransitionToInspectCollectible(Brain, BestTargetInfo.TargetActor.Get());
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

	if (UDroneCompanionFollowComponent* Follow = GetFollowComponent(Brain))
	{
		Follow->SetFollowEnabled(false);
	}

	if (UDroneCompanionFeedbackComponent* Feedback = GetFeedbackComponent(Brain))
	{
		Feedback->PlayCollectibleFeedback();
	}

	LogInspectionStarted(Brain, CollectibleTarget.Get());
}

void FDroneCompanionInspectCollectibleState::Exit(UDroneCompanionBrainComponent& Brain)
{
	if (UDroneCompanionFeedbackComponent* Feedback = GetFeedbackComponent(Brain))
	{
		Feedback->StopCollectibleFeedback();
	}
}

void FDroneCompanionInspectCollectibleState::Tick(UDroneCompanionBrainComponent& Brain, float DeltaTime)
{
	AActor* DroneActor = GetDroneActor(Brain);
	AActor* TargetActor = CollectibleTarget.Get();
	if (!IsValid(DroneActor) || !IsValid(TargetActor))
	{
		LogInspectionAborted(Brain, TEXT("collectible target is invalid"));
		TransitionToFollow(Brain);
		return;
	}

	FDroneCompanionTargetInfo BestTargetInfo;
	if (!GetCachedBestTargetInfo(Brain, BestTargetInfo))
	{
		LogInspectionAborted(Brain, TEXT("sensor lost all targets"));
		TransitionToFollow(Brain);
		return;
	}

	if (BestTargetInfo.TargetType == EDroneCompanionTargetType::Enemy)
	{
		LogInspectionAborted(Brain, TEXT("enemy became the best target"));
		TransitionToAttackEnemy(Brain, BestTargetInfo.TargetActor.Get());
		return;
	}

	const UDroneCompanionConfigDataAsset* Config = GetConfig(Brain);
	const float MoveSpeed = Config ? DroneCompanionBrainValue::PositiveOrDefault(Config->MoveSpeed, DroneCompanionInspectionDefaults::MoveSpeed) : DroneCompanionInspectionDefaults::MoveSpeed;
	const float HoverHeight = Config ? FMath::Max(Config->CollectibleHoverHeight, 0.0f) : DroneCompanionInspectionDefaults::CollectibleHoverHeight;
	const float InspectDuration = FMath::Max(Config ? Config->InspectDuration : DroneCompanionInspectionDefaults::InspectDuration, 0.0f);
	const float AcceptanceRadius = Config ? DroneCompanionBrainValue::PositiveOrDefault(Config->InspectAcceptanceRadius, DroneCompanionInspectionDefaults::InspectAcceptanceRadius) : DroneCompanionInspectionDefaults::InspectAcceptanceRadius;

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
			MarkCollectibleInspectionCompleted(Brain, TargetActor);
			LogInspectionCompleted(Brain, TargetActor);
			TransitionToFollow(Brain);
			return;
		}
	}

	if (ShouldDrawInspectionDebug(Brain))
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

FDroneCompanionAttackEnemyState::FDroneCompanionAttackEnemyState(AActor* InEnemyTarget)
	: EnemyTarget(InEnemyTarget)
{
}

void FDroneCompanionAttackEnemyState::Enter(UDroneCompanionBrainComponent& Brain)
{
	if (UDroneCompanionFollowComponent* Follow = GetFollowComponent(Brain))
	{
		Follow->SetFollowEnabled(false);
	}

	if (UDroneCompanionFeedbackComponent* Feedback = GetFeedbackComponent(Brain))
	{
		Feedback->SetCombatFeedback();
	}

	LogAttackStarted(Brain, EnemyTarget.Get());
}

void FDroneCompanionAttackEnemyState::Exit(UDroneCompanionBrainComponent& Brain)
{
	if (UDroneCompanionFeedbackComponent* Feedback = GetFeedbackComponent(Brain))
	{
		Feedback->StopCombatFeedback();
	}

	LogAttackExited(Brain, EnemyTarget.Get());
}

void FDroneCompanionAttackEnemyState::Tick(UDroneCompanionBrainComponent& Brain, float DeltaTime)
{
	AActor* DroneActor = GetDroneActor(Brain);
	AActor* TargetActor = EnemyTarget.Get();
	if (!IsValid(DroneActor) || !IsValid(TargetActor))
	{
		LogAttackAborted(Brain, TEXT("enemy target is invalid"));
		TransitionToFollow(Brain);
		return;
	}

	FDroneCompanionTargetInfo BestTargetInfo;
	if (!GetCachedBestTargetInfo(Brain, BestTargetInfo) || !ShouldAttackEnemy(Brain, BestTargetInfo))
	{
		LogAttackAborted(Brain, TEXT("sensor no longer reports an enemy as best target"));
		TransitionToFollow(Brain);
		return;
	}

	if (BestTargetInfo.TargetActor.Get() != TargetActor)
	{
		LogAttackAborted(Brain, TEXT("enemy is no longer the best target"));
		TransitionToFollow(Brain);
		return;
	}

	UDroneCompanionCombatComponent* Combat = GetCombatComponent(Brain);
	if (!Combat)
	{
		LogAttackAborted(Brain, TEXT("combat component is missing"));
		TransitionToFollow(Brain);
		return;
	}

	if (!Combat->IsTargetInRange(TargetActor))
	{
		LogAttackAborted(Brain, TEXT("enemy is outside attack range"));
		TransitionToFollow(Brain);
		return;
	}

	if (!Combat->HasClearShot(TargetActor))
	{
		LogAttackAborted(Brain, TEXT("enemy line of sight is blocked"));
		TransitionToFollow(Brain);
		return;
	}

	const UDroneCompanionConfigDataAsset* Config = GetConfig(Brain);
	const float AimInterpSpeed = Config ? DroneCompanionBrainValue::PositiveOrDefault(Config->AimInterpSpeed, DroneCompanionAttackDefaults::AimInterpSpeed) : DroneCompanionAttackDefaults::AimInterpSpeed;
	const float MaxFireAngleDegrees = FMath::Clamp(Config ? Config->MaxFireAngleDegrees : DroneCompanionAttackDefaults::MaxFireAngleDegrees, 0.0f, 180.0f);

	const FVector DirectionToTarget = (TargetActor->GetActorLocation() - DroneActor->GetActorLocation()).GetSafeNormal();
	if (DirectionToTarget.IsNearlyZero())
	{
		return;
	}

	const FRotator DesiredRotation = DirectionToTarget.Rotation();
	const FRotator NewRotation = FMath::RInterpTo(DroneActor->GetActorRotation(), DesiredRotation, DeltaTime, AimInterpSpeed);
	DroneActor->SetActorRotation(NewRotation, ETeleportType::None);

	const float FacingDot = FVector::DotProduct(DroneActor->GetActorForwardVector(), DirectionToTarget);
	const float FacingAngleDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FacingDot, -1.0f, 1.0f)));
	if (FacingAngleDegrees <= MaxFireAngleDegrees && Combat->CanFire())
	{
		if (UDroneCompanionFeedbackComponent* Feedback = GetFeedbackComponent(Brain))
		{
			Feedback->PlayFireFeedback();
		}

		Combat->TryFireAtTarget(TargetActor);
	}
}

FName FDroneCompanionAttackEnemyState::GetName() const
{
	return DroneCompanionBrainStateNames::AttackEnemy;
}
