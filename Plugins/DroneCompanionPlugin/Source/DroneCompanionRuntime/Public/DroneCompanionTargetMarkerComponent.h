#pragma once

#include "Components/ActorComponent.h"
#include "DroneCompanionTargetTypes.h"
#include "DroneCompanionTargetMarkerComponent.generated.h"

// Editor marker that makes an actor visible to the drone sensor.
UCLASS(ClassGroup = (DroneCompanion), meta = (BlueprintSpawnableComponent))
class DRONECOMPANIONRUNTIME_API UDroneCompanionTargetMarkerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDroneCompanionTargetMarkerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Target")
	EDroneCompanionTargetType TargetType = EDroneCompanionTargetType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Target")
	bool bIsDetectable = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Target")
	float PriorityBonus = 0.0f;
};
