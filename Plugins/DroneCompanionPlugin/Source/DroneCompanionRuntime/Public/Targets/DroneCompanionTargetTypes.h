#pragma once

#include "CoreTypes.h"
#include "Math/Vector.h"
#include "UObject/ObjectMacros.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "DroneCompanionTargetTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EDroneCompanionTargetType : uint8
{
	None,
	Collectible,
	Enemy
};

struct FDroneCompanionTargetInfo
{
	TWeakObjectPtr<AActor> TargetActor;
	EDroneCompanionTargetType TargetType = EDroneCompanionTargetType::None;
	FVector LastKnownLocation = FVector::ZeroVector;
	float Distance = 0.0f;
	float Score = 0.0f;
	bool bLineOfSightConfirmed = false;
};
