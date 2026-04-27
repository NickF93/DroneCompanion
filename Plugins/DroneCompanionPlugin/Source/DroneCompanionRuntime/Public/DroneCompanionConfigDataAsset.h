#pragma once

#include "Engine/DataAsset.h"
#include "Math/Color.h"
#include "UObject/ObjectPtr.h"
#include "DroneCompanionConfigDataAsset.generated.h"

class USoundBase;

// Editor-authored base values for the companion drone.
UCLASS(BlueprintType)
class DRONECOMPANIONRUNTIME_API UDroneCompanionConfigDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Follow", meta = (ClampMin = "0.0"))
	float FollowDistance = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Follow", meta = (ClampMin = "0.0"))
	float FollowHeight = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Follow")
	bool bAutoAcquirePlayerOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Follow")
	bool bEnableFollowDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Follow", meta = (ClampMin = "0.0"))
	float AcceptanceRadius = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Movement", meta = (ClampMin = "0.0"))
	float MoveSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Scanning", meta = (ClampMin = "0.0"))
	float ScanRadius = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Scanning", meta = (ClampMin = "0.0"))
	float ScanInterval = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Scanning")
	bool bEnableSensorDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Scanning")
	bool bRequireLineOfSightForDetection = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Scanning", meta = (ClampMin = "0.0"))
	float EnemyBaseScore = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Scanning", meta = (ClampMin = "0.0"))
	float CollectibleBaseScore = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Collectibles", meta = (ClampMin = "0.0"))
	float CollectibleHoverHeight = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Collectibles", meta = (ClampMin = "0.0"))
	float InspectDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Collectibles")
	bool bEnableBrainDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Collectibles")
	bool bEnableInspectionDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Collectibles", meta = (ClampMin = "0.0"))
	float InspectAcceptanceRadius = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Combat", meta = (ClampMin = "0.0"))
	float AttackRange = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Combat", meta = (ClampMin = "0.0"))
	float FireCooldown = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Feedback")
	FLinearColor IdleLightColor = FLinearColor(0.0f, 0.55f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Feedback", meta = (ClampMin = "0.0"))
	float IdleLightIntensity = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Feedback")
	FLinearColor CollectibleLightColor = FLinearColor(0.0f, 1.0f, 0.25f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Feedback", meta = (ClampMin = "0.0"))
	float CollectibleLightIntensity = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Feedback")
	TObjectPtr<USoundBase> CollectibleFeedbackSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Feedback")
	FLinearColor CombatLightColor = FLinearColor(1.0f, 0.05f, 0.0f, 1.0f);
};
