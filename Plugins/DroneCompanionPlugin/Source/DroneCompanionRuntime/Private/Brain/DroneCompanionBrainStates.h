#pragma once

#include "Components/DroneCompanionBrainComponent.h"
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

protected:
	static void TransitionToFollow(UDroneCompanionBrainComponent& Brain);
	static void TransitionToInspectCollectible(UDroneCompanionBrainComponent& Brain, AActor* CollectibleTarget);
	static void TransitionToAttackEnemy(UDroneCompanionBrainComponent& Brain, AActor* EnemyTarget);
	static bool GetCachedBestTargetInfo(const UDroneCompanionBrainComponent& Brain, FDroneCompanionTargetInfo& OutTargetInfo);
	static bool ShouldInspectCollectible(const UDroneCompanionBrainComponent& Brain, const FDroneCompanionTargetInfo& TargetInfo);
	static bool ShouldAttackEnemy(const UDroneCompanionBrainComponent& Brain, const FDroneCompanionTargetInfo& TargetInfo);
	static void MarkCollectibleInspectionCompleted(UDroneCompanionBrainComponent& Brain, AActor* CollectibleTarget);
	static AActor* GetDroneActor(const UDroneCompanionBrainComponent& Brain);
	static UDroneCompanionConfigDataAsset* GetConfig(const UDroneCompanionBrainComponent& Brain);
	static UDroneCompanionFollowComponent* GetFollowComponent(const UDroneCompanionBrainComponent& Brain);
	static UDroneCompanionFeedbackComponent* GetFeedbackComponent(const UDroneCompanionBrainComponent& Brain);
	static UDroneCompanionCombatComponent* GetCombatComponent(const UDroneCompanionBrainComponent& Brain);
	static bool ShouldDrawInspectionDebug(const UDroneCompanionBrainComponent& Brain);
	static void LogInspectionStarted(const UDroneCompanionBrainComponent& Brain, AActor* CollectibleTarget);
	static void LogInspectionCompleted(const UDroneCompanionBrainComponent& Brain, AActor* CollectibleTarget);
	static void LogInspectionAborted(const UDroneCompanionBrainComponent& Brain, const TCHAR* Reason);
	static void LogAttackStarted(const UDroneCompanionBrainComponent& Brain, AActor* EnemyTarget);
	static void LogAttackExited(const UDroneCompanionBrainComponent& Brain, AActor* EnemyTarget);
	static void LogAttackAborted(const UDroneCompanionBrainComponent& Brain, const TCHAR* Reason);
};

class FDroneCompanionAttackEnemyState final : public IDroneCompanionBrainState
{
public:
	explicit FDroneCompanionAttackEnemyState(AActor* InEnemyTarget);

	virtual void Enter(UDroneCompanionBrainComponent& Brain) override;
	virtual void Exit(UDroneCompanionBrainComponent& Brain) override;
	virtual void Tick(UDroneCompanionBrainComponent& Brain, float DeltaTime) override;
	virtual FName GetName() const override;

private:
	TWeakObjectPtr<AActor> EnemyTarget;
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
