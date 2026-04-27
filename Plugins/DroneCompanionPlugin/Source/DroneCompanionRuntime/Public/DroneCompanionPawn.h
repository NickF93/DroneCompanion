#pragma once

#include "GameFramework/Pawn.h"
#include "DroneCompanionPawn.generated.h"

class UAudioComponent;
class UDroneCompanionConfigDataAsset;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

// Base pawn for the companion drone. Behavior will be added in later phases.
UCLASS(Blueprintable)
class DRONECOMPANIONRUNTIME_API ADroneCompanionPawn : public APawn
{
	GENERATED_BODY()

public:
	ADroneCompanionPawn();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Companion|Config")
	UDroneCompanionConfigDataAsset* Config;
};
