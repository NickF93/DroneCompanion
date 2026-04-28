#pragma once

#include "GameFramework/Pawn.h"
#include "DroneCompanionPawn.generated.h"

class UAudioComponent;
class UDroneCompanionBrainComponent;
class UDroneCompanionConfigDataAsset;
class UDroneCompanionCombatComponent;
class UDroneCompanionFeedbackComponent;
class UDroneCompanionFollowComponent;
class UDroneCompanionSensorComponent;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;
class AActor;

// Base pawn that owns the drone presentation and runtime components.
UCLASS(Blueprintable)
class DRONECOMPANIONRUNTIME_API ADroneCompanionPawn : public APawn
{
	GENERATED_BODY()

public:
	ADroneCompanionPawn();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone Companion|Components")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone Companion|Components")
	UStaticMeshComponent* DroneMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone Companion|Components")
	USceneComponent* MuzzlePoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone Companion|Components")
	UPointLightComponent* StatusLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone Companion|Components")
	UAudioComponent* AudioComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone Companion|Components")
	UDroneCompanionFollowComponent* FollowComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone Companion|Components")
	UDroneCompanionSensorComponent* SensorComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone Companion|Components")
	UDroneCompanionFeedbackComponent* FeedbackComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone Companion|Components")
	UDroneCompanionCombatComponent* CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone Companion|Components")
	UDroneCompanionBrainComponent* BrainComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Config")
	UDroneCompanionConfigDataAsset* Config;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Drone Companion|Follow")
	AActor* InitialFollowTarget;
};
