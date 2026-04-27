#pragma once

#include "Components/ActorComponent.h"
#include "DroneCompanionTargetTypes.h"
#include "Templates/UniquePtr.h"
#include "UObject/NameTypes.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "DroneCompanionBrainComponent.generated.h"

class AActor;
class ADroneCompanionPawn;
class IDroneCompanionBrainState;
class UDroneCompanionBrainComponent;
class UDroneCompanionConfigDataAsset;
class UDroneCompanionFeedbackComponent;
class UDroneCompanionFollowComponent;
class UDroneCompanionSensorComponent;

class FDroneCompanionFollowState;
class FDroneCompanionInspectCollectibleState;

struct DRONECOMPANIONRUNTIME_API FDroneCompanionBrainStateDeleter
{
	void operator()(IDroneCompanionBrainState* State) const;
};

using FDroneCompanionBrainStatePtr = TUniquePtr<IDroneCompanionBrainState, FDroneCompanionBrainStateDeleter>;

// Coordinates high-level drone behavior without implementing sensing or combat.
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
		UDroneCompanionSensorComponent* InSensorComponent,
		UDroneCompanionFeedbackComponent* InFeedbackComponent);

	void StartBrain();
	void StopBrain();
	FName GetCurrentStateName() const;

private:
	friend class FDroneCompanionFollowState;
	friend class FDroneCompanionInspectCollectibleState;

	void HandleBestTargetChanged(const FDroneCompanionTargetInfo& TargetInfo);
	void HandleBestTargetLost();

	void TransitionToFollow();
	void TransitionToInspectCollectible(AActor* CollectibleTarget);
	void SetState(FDroneCompanionBrainStatePtr NewState);
	void ApplyState(FDroneCompanionBrainStatePtr NewState);

	bool GetCachedBestTargetInfo(FDroneCompanionTargetInfo& OutTargetInfo) const;
	bool ShouldInspectCollectible(const FDroneCompanionTargetInfo& TargetInfo) const;
	void MarkCollectibleInspectionCompleted(AActor* CollectibleTarget);
	AActor* GetDroneActor() const;
	UDroneCompanionConfigDataAsset* GetConfig() const;
	UDroneCompanionFollowComponent* GetFollowComponent() const;
	UDroneCompanionFeedbackComponent* GetFeedbackComponent() const;

	bool ShouldLogBrainDebug() const;
	bool ShouldDrawInspectionDebug() const;
	void LogInspectionStarted(AActor* CollectibleTarget) const;
	void LogInspectionCompleted(AActor* CollectibleTarget) const;
	void LogInspectionAborted(const TCHAR* Reason) const;

	TWeakObjectPtr<ADroneCompanionPawn> DronePawn;
	TWeakObjectPtr<UDroneCompanionConfigDataAsset> Config;
	TWeakObjectPtr<UDroneCompanionFollowComponent> FollowComponent;
	TWeakObjectPtr<UDroneCompanionSensorComponent> SensorComponent;
	TWeakObjectPtr<UDroneCompanionFeedbackComponent> FeedbackComponent;
	TWeakObjectPtr<AActor> LastCompletedInspectionTarget;

	FDroneCompanionBrainStatePtr CurrentState;
	FDroneCompanionBrainStatePtr PendingState;
	FDroneCompanionTargetInfo CachedBestTargetInfo;
	bool bHasCachedBestTarget = false;
	bool bIsRunning = false;
	bool bIsSubscribedToSensor = false;
	bool bIsTickingState = false;
};
