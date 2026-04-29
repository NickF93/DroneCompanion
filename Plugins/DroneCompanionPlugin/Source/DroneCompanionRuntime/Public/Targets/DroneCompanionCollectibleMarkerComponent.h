#pragma once

#include "Targets/DroneCompanionTargetMarkerComponent.h"
#include "DroneCompanionCollectibleMarkerComponent.generated.h"

UCLASS(ClassGroup = (DroneCompanion), meta = (BlueprintSpawnableComponent))
class DRONECOMPANIONRUNTIME_API UDroneCompanionCollectibleMarkerComponent : public UDroneCompanionTargetMarkerComponent
{
	GENERATED_BODY()

public:
	UDroneCompanionCollectibleMarkerComponent();
};
