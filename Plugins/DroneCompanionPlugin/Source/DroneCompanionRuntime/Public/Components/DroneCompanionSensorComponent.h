#pragma once

#include "Containers/Array.h"
#include "Components/ActorComponent.h"
#include "Targets/DroneCompanionTargetTypes.h"
#include "TimerManager.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "DroneCompanionSensorComponent.generated.h"

class AActor;
class UDroneCompanionConfigDataAsset;

DECLARE_MULTICAST_DELEGATE_OneParam(FDroneCompanionBestTargetChangedSignature, const FDroneCompanionTargetInfo&);
DECLARE_MULTICAST_DELEGATE(FDroneCompanionBestTargetLostSignature);

// Facade for target overlap, filtering, line-of-sight checks, and scoring.
UCLASS(ClassGroup = (DroneCompanion))
class DRONECOMPANIONRUNTIME_API UDroneCompanionSensorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDroneCompanionSensorComponent();

	void StartSensing();
	void StopSensing();
	void ScanForTargets();

	bool HasBestTarget() const;
	AActor* GetBestTargetActor() const;
	EDroneCompanionTargetType GetBestTargetType() const;
	bool GetBestTargetInfo(FDroneCompanionTargetInfo& OutTargetInfo) const;

	void SetConfig(UDroneCompanionConfigDataAsset* NewConfig);

	FDroneCompanionBestTargetChangedSignature OnBestTargetChanged;
	FDroneCompanionBestTargetLostSignature OnBestTargetLost;

private:
	float GetScanRadius() const;
	float GetScanInterval() const;
	bool ShouldRequireLineOfSight() const;
	bool ShouldDrawDebug() const;
	bool ShouldLogSensorDebug() const;
	float GetBaseScore(EDroneCompanionTargetType TargetType) const;
	bool HasLineOfSightToTarget(AActor* Owner, AActor* Candidate, FVector CandidateLocation) const;
	float CalculateScore(EDroneCompanionTargetType TargetType, float Distance, float PriorityBonus, float ScanRadius) const;
	void UpdateBestTarget(const FDroneCompanionTargetInfo& NewBestTarget, bool bFoundTarget);
	void DrawSensorDebug(FVector Origin, float ScanRadius, const TArray<FDroneCompanionTargetInfo>& Candidates) const;

	TWeakObjectPtr<UDroneCompanionConfigDataAsset> Config;
	FDroneCompanionTargetInfo BestTargetInfo;
	FTimerHandle ScanTimerHandle;
	bool bHasBestTarget = false;
};
