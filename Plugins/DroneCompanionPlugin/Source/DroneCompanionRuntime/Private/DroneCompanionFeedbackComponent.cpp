#include "DroneCompanionFeedbackComponent.h"

#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "DroneCompanionConfigDataAsset.h"
#include "Math/Color.h"
#include "Math/UnrealMathUtility.h"
#include "Sound/SoundBase.h"

namespace DroneCompanionFeedbackDefaults
{
	const FLinearColor IdleLightColor = FLinearColor::White;
	const FLinearColor CollectibleLightColor = FLinearColor::Yellow;
	constexpr float IdleLightIntensity = 2000.0f;
	constexpr float CollectibleLightIntensity = 5000.0f;
}

UDroneCompanionFeedbackComponent::UDroneCompanionFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDroneCompanionFeedbackComponent::InitializeFeedback(UPointLightComponent* InStatusLight, UAudioComponent* InAudioComponent, UDroneCompanionConfigDataAsset* InConfig)
{
	StatusLight = InStatusLight;
	AudioComponent = InAudioComponent;
	Config = InConfig;
}

void UDroneCompanionFeedbackComponent::SetIdleFeedback()
{
	const UDroneCompanionConfigDataAsset* ConfigAsset = Config.Get();
	const FLinearColor LightColor = ConfigAsset ? ConfigAsset->IdleLightColor : DroneCompanionFeedbackDefaults::IdleLightColor;
	const float LightIntensity = FMath::Max(ConfigAsset ? ConfigAsset->IdleLightIntensity : DroneCompanionFeedbackDefaults::IdleLightIntensity, 0.0f);

	if (UPointLightComponent* Light = StatusLight.Get())
	{
		Light->SetLightColor(LightColor);
		Light->SetIntensity(LightIntensity);
	}
}

void UDroneCompanionFeedbackComponent::PlayCollectibleFeedback()
{
	const UDroneCompanionConfigDataAsset* ConfigAsset = Config.Get();
	const FLinearColor LightColor = ConfigAsset ? ConfigAsset->CollectibleLightColor : DroneCompanionFeedbackDefaults::CollectibleLightColor;
	const float LightIntensity = FMath::Max(ConfigAsset ? ConfigAsset->CollectibleLightIntensity : DroneCompanionFeedbackDefaults::CollectibleLightIntensity, 0.0f);

	if (UPointLightComponent* Light = StatusLight.Get())
	{
		Light->SetLightColor(LightColor);
		Light->SetIntensity(LightIntensity);
	}

	if (ConfigAsset && ConfigAsset->CollectibleFeedbackSound)
	{
		if (UAudioComponent* Audio = AudioComponent.Get())
		{
			Audio->SetSound(ConfigAsset->CollectibleFeedbackSound);
			Audio->Play();
		}
	}
}

void UDroneCompanionFeedbackComponent::StopCollectibleFeedback()
{
	if (UAudioComponent* Audio = AudioComponent.Get())
	{
		if (Audio->IsPlaying())
		{
			Audio->Stop();
		}
	}

	SetIdleFeedback();
}
