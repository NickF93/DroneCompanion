#pragma once

#include "Components/ActorComponent.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "DroneCompanionFollowComponent.generated.h"

class AActor;
class UDroneCompanionConfigDataAsset;

// Moves the owning actor toward a simple offset behind a target actor.
UCLASS(ClassGroup = (DroneCompanion))
class DRONECOMPANIONRUNTIME_API UDroneCompanionFollowComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDroneCompanionFollowComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetFollowTarget(AActor* NewTarget);
	AActor* GetFollowTarget() const;
	void ClearFollowTarget();
	bool HasValidFollowTarget() const;

	void SetConfig(UDroneCompanionConfigDataAsset* NewConfig);
	void SetFollowEnabled(bool bEnabled);
	bool IsFollowEnabled() const;

private:
	TWeakObjectPtr<AActor> FollowTarget;
	TWeakObjectPtr<UDroneCompanionConfigDataAsset> Config;
	bool bFollowEnabled = true;
};
