#pragma once

#include "GameFramework/PawnMovementComponent.h"
#include "Math/Vector.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "DroneCompanionMovementComponent.generated.h"

class UDroneCompanionConfigDataAsset;
class USceneComponent;

// Owns collision-aware pawn movement execution for the drone.
UCLASS(ClassGroup = (DroneCompanion))
class DRONECOMPANIONRUNTIME_API UDroneCompanionMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()

public:
	UDroneCompanionMovementComponent();

	void InitializeMovement(USceneComponent* InUpdatedComponent, UDroneCompanionConfigDataAsset* InConfig);
	bool MoveTowardLocation(const FVector& DesiredLocation, float MaxSpeed, float AcceptanceRadius, float DeltaTime);

private:
	bool ShouldDrawMovementDebug() const;

	TWeakObjectPtr<UDroneCompanionConfigDataAsset> Config;
	bool bLoggedMissingUpdatedComponent = false;
};
