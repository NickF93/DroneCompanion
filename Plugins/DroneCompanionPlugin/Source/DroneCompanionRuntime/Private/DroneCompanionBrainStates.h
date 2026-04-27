#pragma once

#include "DroneCompanionBrainComponent.h"
#include "UObject/NameTypes.h"
#include "UObject/WeakObjectPtrTemplates.h"

class AActor;

class IDroneCompanionBrainState
{
public:
	virtual ~IDroneCompanionBrainState() = default;

	virtual void Enter(UDroneCompanionBrainComponent& Brain) = 0;
	virtual void Exit(UDroneCompanionBrainComponent& Brain) = 0;
	virtual void Tick(UDroneCompanionBrainComponent& Brain, float DeltaTime) = 0;
	virtual FName GetName() const = 0;
};

class FDroneCompanionFollowState final : public IDroneCompanionBrainState
{
public:
	virtual void Enter(UDroneCompanionBrainComponent& Brain) override;
	virtual void Exit(UDroneCompanionBrainComponent& Brain) override;
	virtual void Tick(UDroneCompanionBrainComponent& Brain, float DeltaTime) override;
	virtual FName GetName() const override;
};

class FDroneCompanionInspectCollectibleState final : public IDroneCompanionBrainState
{
public:
	explicit FDroneCompanionInspectCollectibleState(AActor* InCollectibleTarget);

	virtual void Enter(UDroneCompanionBrainComponent& Brain) override;
	virtual void Exit(UDroneCompanionBrainComponent& Brain) override;
	virtual void Tick(UDroneCompanionBrainComponent& Brain, float DeltaTime) override;
	virtual FName GetName() const override;

private:
	TWeakObjectPtr<AActor> CollectibleTarget;
	float InspectElapsedTime = 0.0f;
};
