#pragma once

#include "DroneCompanionTargetMarkerComponent.h"
#include "DroneCompanionEnemyMarkerComponent.generated.h"

UCLASS(ClassGroup = (DroneCompanion), meta = (BlueprintSpawnableComponent))
class DRONECOMPANIONRUNTIME_API UDroneCompanionEnemyMarkerComponent : public UDroneCompanionTargetMarkerComponent
{
	GENERATED_BODY()

public:
	UDroneCompanionEnemyMarkerComponent();
};
