#pragma once

#include "Components/ActorComponent.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "DroneCompanionFeedbackComponent.generated.h"

class UAudioComponent;
class UDroneCompanionConfigDataAsset;
class UPointLightComponent;

// Applies simple light and audio feedback for the drone.
UCLASS(ClassGroup = (DroneCompanion))
class DRONECOMPANIONRUNTIME_API UDroneCompanionFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDroneCompanionFeedbackComponent();

	void InitializeFeedback(UPointLightComponent* InStatusLight, UAudioComponent* InAudioComponent, UDroneCompanionConfigDataAsset* InConfig);
	void SetIdleFeedback();
	void PlayCollectibleFeedback();
	void StopCollectibleFeedback();

private:
	TWeakObjectPtr<UPointLightComponent> StatusLight;
	TWeakObjectPtr<UAudioComponent> AudioComponent;
	TWeakObjectPtr<UDroneCompanionConfigDataAsset> Config;
};
