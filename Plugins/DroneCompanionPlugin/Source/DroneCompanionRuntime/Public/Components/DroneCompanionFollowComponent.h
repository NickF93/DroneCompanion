#pragma once

#include "Components/ActorComponent.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "DroneCompanionFollowComponent.generated.h"

class AActor;
class UDroneCompanionConfigDataAsset;
class UDroneCompanionMovementComponent;

// Computes follow intent and delegates physical movement to the drone movement component.
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
	void SetMovementComponent(UDroneCompanionMovementComponent* NewMovementComponent);
	void SetFollowEnabled(bool bEnabled);
	bool IsFollowEnabled() const;

private:
	TWeakObjectPtr<AActor> FollowTarget;
	TWeakObjectPtr<UDroneCompanionConfigDataAsset> Config;
	TWeakObjectPtr<UDroneCompanionMovementComponent> MovementComponent;
	bool bFollowEnabled = true;
};
