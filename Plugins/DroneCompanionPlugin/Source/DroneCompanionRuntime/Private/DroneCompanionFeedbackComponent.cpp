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
	const FLinearColor CombatLightColor = FLinearColor::Red;
	constexpr float IdleLightIntensity = 2000.0f;
	constexpr float CollectibleLightIntensity = 5000.0f;
	constexpr float CombatLightIntensity = 6000.0f;
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

void UDroneCompanionFeedbackComponent::SetCombatFeedback()
{
	const UDroneCompanionConfigDataAsset* ConfigAsset = Config.Get();
	const FLinearColor LightColor = ConfigAsset ? ConfigAsset->CombatLightColor : DroneCompanionFeedbackDefaults::CombatLightColor;
	const float LightIntensity = FMath::Max(ConfigAsset ? ConfigAsset->CombatLightIntensity : DroneCompanionFeedbackDefaults::CombatLightIntensity, 0.0f);

	if (UPointLightComponent* Light = StatusLight.Get())
	{
		Light->SetLightColor(LightColor);
		Light->SetIntensity(LightIntensity);
	}
}

void UDroneCompanionFeedbackComponent::PlayFireFeedback()
{
	const UDroneCompanionConfigDataAsset* ConfigAsset = Config.Get();
	if (ConfigAsset && ConfigAsset->FireSound)
	{
		if (UAudioComponent* Audio = AudioComponent.Get())
		{
			Audio->SetSound(ConfigAsset->FireSound);
			Audio->Play();
		}
	}
}

void UDroneCompanionFeedbackComponent::StopCombatFeedback()
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
