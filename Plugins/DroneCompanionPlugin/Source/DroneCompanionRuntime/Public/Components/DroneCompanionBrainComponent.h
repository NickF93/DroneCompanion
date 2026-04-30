#pragma once

#include "Containers/UnrealString.h"
#include "Components/ActorComponent.h"
#include "Templates/UniquePtr.h"
#include "Targets/DroneCompanionTargetTypes.h"
#include "UObject/NameTypes.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "DroneCompanionBrainComponent.generated.h"

class AActor;
class ADroneCompanionPawn;
class IDroneCompanionBrainState;
class UDroneCompanionCombatComponent;
class UDroneCompanionConfigDataAsset;
class UDroneCompanionFeedbackComponent;
class UDroneCompanionFollowComponent;
class UDroneCompanionMovementComponent;
class UDroneCompanionSensorComponent;

// Deletion is defined privately because UHT sees this header with only the state interface forward-declared.
struct DRONECOMPANIONRUNTIME_API FDroneCompanionBrainStateDeleter
{
	void operator()(IDroneCompanionBrainState* State) const;
};

using FDroneCompanionBrainStatePtr = TUniquePtr<IDroneCompanionBrainState, FDroneCompanionBrainStateDeleter>;

// Coordinates high-level drone behavior without implementing sensing or weapon logic.
UCLASS(ClassGroup = (DroneCompanion))
class DRONECOMPANIONRUNTIME_API UDroneCompanionBrainComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDroneCompanionBrainComponent();
	virtual ~UDroneCompanionBrainComponent() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void InitializeBrain(
		ADroneCompanionPawn* InDronePawn,
		UDroneCompanionConfigDataAsset* InConfig,
		UDroneCompanionFollowComponent* InFollowComponent,
		UDroneCompanionMovementComponent* InMovementComponent,
		UDroneCompanionSensorComponent* InSensorComponent,
		UDroneCompanionFeedbackComponent* InFeedbackComponent,
		UDroneCompanionCombatComponent* InCombatComponent);

	void StartBrain();
	void StopBrain();
	FName GetCurrentStateName() const;
	FString GetDebugStatusString() const;

private:
	friend class IDroneCompanionBrainState;

	void HandleBestTargetChanged(const FDroneCompanionTargetInfo& TargetInfo);
	void HandleBestTargetLost();

	void TransitionToFollow();
	void TransitionToInspectCollectible(AActor* CollectibleTarget);
	void TransitionToAttackEnemy(AActor* EnemyTarget);
	void SetState(FDroneCompanionBrainStatePtr NewState);
	void ApplyState(FDroneCompanionBrainStatePtr NewState);

	bool GetCachedBestTargetInfo(FDroneCompanionTargetInfo& OutTargetInfo) const;
	bool ShouldInspectCollectible(const FDroneCompanionTargetInfo& TargetInfo) const;
	bool ShouldAttackEnemy(const FDroneCompanionTargetInfo& TargetInfo) const;
	void MarkCollectibleInspectionCompleted(AActor* CollectibleTarget);
	AActor* GetDroneActor() const;
	UDroneCompanionConfigDataAsset* GetConfig() const;
	UDroneCompanionFollowComponent* GetFollowComponent() const;
	UDroneCompanionMovementComponent* GetMovementComponent() const;
	UDroneCompanionFeedbackComponent* GetFeedbackComponent() const;
	UDroneCompanionCombatComponent* GetCombatComponent() const;

	bool ShouldLogBrainDebug() const;
	bool ShouldDrawInspectionDebug() const;
	bool ShouldLogCombatDebug() const;
	void LogInspectionStarted(AActor* CollectibleTarget) const;
	void LogInspectionCompleted(AActor* CollectibleTarget) const;
	void LogInspectionAborted(const TCHAR* Reason) const;
	void LogAttackStarted(AActor* EnemyTarget) const;
	void LogAttackExited(AActor* EnemyTarget) const;
	void LogAttackAborted(const TCHAR* Reason) const;

	TWeakObjectPtr<ADroneCompanionPawn> DronePawn;
	TWeakObjectPtr<UDroneCompanionConfigDataAsset> Config;
	TWeakObjectPtr<UDroneCompanionFollowComponent> FollowComponent;
	TWeakObjectPtr<UDroneCompanionMovementComponent> MovementComponent;
	TWeakObjectPtr<UDroneCompanionSensorComponent> SensorComponent;
	TWeakObjectPtr<UDroneCompanionFeedbackComponent> FeedbackComponent;
	TWeakObjectPtr<UDroneCompanionCombatComponent> CombatComponent;
	TWeakObjectPtr<AActor> LastCompletedInspectionTarget;

	FDroneCompanionBrainStatePtr CurrentState;
	FDroneCompanionBrainStatePtr PendingState;
	FDroneCompanionTargetInfo CachedBestTargetInfo;
	bool bHasCachedBestTarget = false;
	bool bIsRunning = false;
	bool bIsSubscribedToSensor = false;
	bool bIsTickingState = false;
};
