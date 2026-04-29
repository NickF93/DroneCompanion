#include "DroneCompanionBrainComponent.h"

#include "DroneCompanionBrainStates.h"
#include "DroneCompanionCombatComponent.h"
#include "DroneCompanionConfigDataAsset.h"
#include "DroneCompanionFeedbackComponent.h"
#include "DroneCompanionFollowComponent.h"
#include "DroneCompanionPawn.h"
#include "DroneCompanionRuntimeModule.h"
#include "DroneCompanionSensorComponent.h"
#include "GameFramework/Actor.h"

namespace
{
	const TCHAR* GetTargetTypeDebugName(EDroneCompanionTargetType TargetType)
	{
		switch (TargetType)
		{
		case EDroneCompanionTargetType::Collectible:
			return TEXT("Collectible");
		case EDroneCompanionTargetType::Enemy:
			return TEXT("Enemy");
		case EDroneCompanionTargetType::None:
		default:
			return TEXT("None");
		}
	}
}

UDroneCompanionBrainComponent::UDroneCompanionBrainComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

UDroneCompanionBrainComponent::~UDroneCompanionBrainComponent() = default;

void UDroneCompanionBrainComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsRunning && CurrentState)
	{
		bIsTickingState = true;
		CurrentState->Tick(*this, DeltaTime);
		bIsTickingState = false;

		if (PendingState)
		{
			ApplyState(MoveTemp(PendingState));
		}
	}
}

void UDroneCompanionBrainComponent::InitializeBrain(
	ADroneCompanionPawn* InDronePawn,
	UDroneCompanionConfigDataAsset* InConfig,
	UDroneCompanionFollowComponent* InFollowComponent,
	UDroneCompanionSensorComponent* InSensorComponent,
	UDroneCompanionFeedbackComponent* InFeedbackComponent,
	UDroneCompanionCombatComponent* InCombatComponent)
{
	DronePawn = InDronePawn;
	Config = InConfig;
	FollowComponent = InFollowComponent;
	SensorComponent = InSensorComponent;
	FeedbackComponent = InFeedbackComponent;
	CombatComponent = InCombatComponent;
}

void UDroneCompanionBrainComponent::StartBrain()
{
	if (bIsRunning)
	{
		return;
	}

	if (UDroneCompanionSensorComponent* Sensor = SensorComponent.Get())
	{
		Sensor->OnBestTargetChanged.AddUObject(this, &UDroneCompanionBrainComponent::HandleBestTargetChanged);
		Sensor->OnBestTargetLost.AddUObject(this, &UDroneCompanionBrainComponent::HandleBestTargetLost);
		bIsSubscribedToSensor = true;
		bHasCachedBestTarget = Sensor->GetBestTargetInfo(CachedBestTargetInfo);
	}

	bIsRunning = true;
	SetComponentTickEnabled(true);
	TransitionToFollow();

	UE_LOG(LogDroneCompanion, Log, TEXT("%s brain started."), *GetName());
}

void UDroneCompanionBrainComponent::StopBrain()
{
	if (UDroneCompanionSensorComponent* Sensor = SensorComponent.Get())
	{
		if (bIsSubscribedToSensor)
		{
			Sensor->OnBestTargetChanged.RemoveAll(this);
			Sensor->OnBestTargetLost.RemoveAll(this);
		}
	}

	bIsSubscribedToSensor = false;
	bIsRunning = false;
	bIsTickingState = false;
	SetComponentTickEnabled(false);
	PendingState.Reset();
	LastCompletedInspectionTarget.Reset();

	if (CurrentState)
	{
		CurrentState->Exit(*this);
		CurrentState.Reset();
	}

	if (UDroneCompanionFollowComponent* Follow = FollowComponent.Get())
	{
		Follow->SetFollowEnabled(true);
	}
}

FName UDroneCompanionBrainComponent::GetCurrentStateName() const
{
	return CurrentState ? CurrentState->GetName() : NAME_None;
}

FString UDroneCompanionBrainComponent::GetDebugStatusString() const
{
	if (!bHasCachedBestTarget || !CachedBestTargetInfo.TargetActor.IsValid())
	{
		return FString::Printf(
			TEXT("State=%s Running=%s Target=None"),
			*GetCurrentStateName().ToString(),
			bIsRunning ? TEXT("true") : TEXT("false"));
	}

	return FString::Printf(
		TEXT("State=%s Running=%s Target=%s Type=%s Distance=%.1f Score=%.1f"),
		*GetCurrentStateName().ToString(),
		bIsRunning ? TEXT("true") : TEXT("false"),
		*GetNameSafe(CachedBestTargetInfo.TargetActor.Get()),
		GetTargetTypeDebugName(CachedBestTargetInfo.TargetType),
		CachedBestTargetInfo.Distance,
		CachedBestTargetInfo.Score);
}

void UDroneCompanionBrainComponent::HandleBestTargetChanged(const FDroneCompanionTargetInfo& TargetInfo)
{
	CachedBestTargetInfo = TargetInfo;
	bHasCachedBestTarget = TargetInfo.TargetActor.IsValid();

	if (!TargetInfo.TargetActor.IsValid() || TargetInfo.TargetActor.Get() != LastCompletedInspectionTarget.Get())
	{
		LastCompletedInspectionTarget.Reset();
	}
}

void UDroneCompanionBrainComponent::HandleBestTargetLost()
{
	CachedBestTargetInfo = FDroneCompanionTargetInfo();
	bHasCachedBestTarget = false;
	LastCompletedInspectionTarget.Reset();
}

void UDroneCompanionBrainComponent::TransitionToFollow()
{
	SetState(FDroneCompanionBrainStatePtr(new FDroneCompanionFollowState()));
}

void UDroneCompanionBrainComponent::TransitionToInspectCollectible(AActor* CollectibleTarget)
{
	if (!CollectibleTarget)
	{
		TransitionToFollow();
		return;
	}

	SetState(FDroneCompanionBrainStatePtr(new FDroneCompanionInspectCollectibleState(CollectibleTarget)));
}

void UDroneCompanionBrainComponent::TransitionToAttackEnemy(AActor* EnemyTarget)
{
	if (!EnemyTarget)
	{
		TransitionToFollow();
		return;
	}

	SetState(FDroneCompanionBrainStatePtr(new FDroneCompanionAttackEnemyState(EnemyTarget)));
}

void UDroneCompanionBrainComponent::SetState(FDroneCompanionBrainStatePtr NewState)
{
	if (bIsTickingState)
	{
		PendingState = MoveTemp(NewState);
		return;
	}

	ApplyState(MoveTemp(NewState));
}

void UDroneCompanionBrainComponent::ApplyState(FDroneCompanionBrainStatePtr NewState)
{
	const FName PreviousStateName = GetCurrentStateName();

	if (CurrentState)
	{
		CurrentState->Exit(*this);
	}

	CurrentState = MoveTemp(NewState);

	if (ShouldLogBrainDebug())
	{
		UE_LOG(LogDroneCompanion, Log, TEXT("%s state changed from %s to %s."), *GetName(), *PreviousStateName.ToString(), *GetCurrentStateName().ToString());
	}

	if (CurrentState)
	{
		CurrentState->Enter(*this);
	}
}

bool UDroneCompanionBrainComponent::GetCachedBestTargetInfo(FDroneCompanionTargetInfo& OutTargetInfo) const
{
	if (!bHasCachedBestTarget || !CachedBestTargetInfo.TargetActor.IsValid())
	{
		return false;
	}

	OutTargetInfo = CachedBestTargetInfo;
	return true;
}

bool UDroneCompanionBrainComponent::ShouldInspectCollectible(const FDroneCompanionTargetInfo& TargetInfo) const
{
	if (TargetInfo.TargetType != EDroneCompanionTargetType::Collectible || !TargetInfo.TargetActor.IsValid())
	{
		return false;
	}

	return TargetInfo.TargetActor.Get() != LastCompletedInspectionTarget.Get();
}

bool UDroneCompanionBrainComponent::ShouldAttackEnemy(const FDroneCompanionTargetInfo& TargetInfo) const
{
	return TargetInfo.TargetType == EDroneCompanionTargetType::Enemy && TargetInfo.TargetActor.IsValid();
}

void UDroneCompanionBrainComponent::MarkCollectibleInspectionCompleted(AActor* CollectibleTarget)
{
	LastCompletedInspectionTarget = CollectibleTarget;
}

AActor* UDroneCompanionBrainComponent::GetDroneActor() const
{
	return DronePawn.Get();
}

UDroneCompanionConfigDataAsset* UDroneCompanionBrainComponent::GetConfig() const
{
	return Config.Get();
}

UDroneCompanionFollowComponent* UDroneCompanionBrainComponent::GetFollowComponent() const
{
	return FollowComponent.Get();
}

UDroneCompanionFeedbackComponent* UDroneCompanionBrainComponent::GetFeedbackComponent() const
{
	return FeedbackComponent.Get();
}

UDroneCompanionCombatComponent* UDroneCompanionBrainComponent::GetCombatComponent() const
{
	return CombatComponent.Get();
}

bool UDroneCompanionBrainComponent::ShouldLogBrainDebug() const
{
	const UDroneCompanionConfigDataAsset* ConfigAsset = Config.Get();
	return ConfigAsset && ConfigAsset->bEnableBrainDebug;
}

bool UDroneCompanionBrainComponent::ShouldDrawInspectionDebug() const
{
	const UDroneCompanionConfigDataAsset* ConfigAsset = Config.Get();
	return ConfigAsset && ConfigAsset->bEnableInspectionDebug;
}

bool UDroneCompanionBrainComponent::ShouldLogCombatDebug() const
{
	const UDroneCompanionConfigDataAsset* ConfigAsset = Config.Get();
	return ConfigAsset && ConfigAsset->bEnableCombatDebug;
}

void UDroneCompanionBrainComponent::LogInspectionStarted(AActor* CollectibleTarget) const
{
	UE_LOG(LogDroneCompanion, Log, TEXT("%s started inspecting collectible %s."), *GetName(), *GetNameSafe(CollectibleTarget));
}

void UDroneCompanionBrainComponent::LogInspectionCompleted(AActor* CollectibleTarget) const
{
	UE_LOG(LogDroneCompanion, Log, TEXT("%s completed collectible inspection for %s."), *GetName(), *GetNameSafe(CollectibleTarget));
}

void UDroneCompanionBrainComponent::LogInspectionAborted(const TCHAR* Reason) const
{
	UE_LOG(LogDroneCompanion, Log, TEXT("%s aborted collectible inspection: %s."), *GetName(), Reason ? Reason : TEXT("unspecified"));
}

void UDroneCompanionBrainComponent::LogAttackStarted(AActor* EnemyTarget) const
{
	if (ShouldLogCombatDebug())
	{
		UE_LOG(LogDroneCompanion, Log, TEXT("%s started attacking enemy %s."), *GetName(), *GetNameSafe(EnemyTarget));
	}
}

void UDroneCompanionBrainComponent::LogAttackExited(AActor* EnemyTarget) const
{
	if (ShouldLogCombatDebug())
	{
		UE_LOG(LogDroneCompanion, Log, TEXT("%s stopped attacking enemy %s."), *GetName(), *GetNameSafe(EnemyTarget));
	}
}

void UDroneCompanionBrainComponent::LogAttackAborted(const TCHAR* Reason) const
{
	if (ShouldLogCombatDebug())
	{
		UE_LOG(LogDroneCompanion, Log, TEXT("%s aborted enemy attack: %s."), *GetName(), Reason ? Reason : TEXT("unspecified"));
	}
}
