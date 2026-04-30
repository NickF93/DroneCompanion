#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/DroneCompanionBrainComponent.h"
#include "Components/DroneCompanionCombatComponent.h"
#include "Components/DroneCompanionFeedbackComponent.h"
#include "Components/DroneCompanionFollowComponent.h"
#include "Components/DroneCompanionMovementComponent.h"
#include "Components/DroneCompanionSensorComponent.h"
#include "Core/DroneCompanionConfigDataAsset.h"
#include "Core/DroneCompanionPawn.h"
#include "Engine/Engine.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Targets/DroneCompanionCollectibleMarkerComponent.h"
#include "Targets/DroneCompanionEnemyMarkerComponent.h"
#include "Targets/DroneCompanionTargetMarkerComponent.h"
#include "Targets/DroneCompanionTargetTypes.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

namespace DroneCompanionAutomation
{
	constexpr float DefaultCollisionRadius = 25.0f;
	constexpr float PawnFallbackCollisionRadius = 40.0f;
	constexpr float TickDeltaSeconds = 0.1f;

	class FTestWorld
	{
	public:
		FTestWorld()
		{
			if (!GEngine)
			{
				return;
			}

			const FName WorldName = MakeUniqueObjectName(nullptr, UWorld::StaticClass(), TEXT("DroneCompanionAutomationWorld"), EUniqueObjectNameOptions::GloballyUnique);
			WorldContext = &GEngine->CreateNewWorldContext(EWorldType::Game);
			World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
			if (!World)
			{
				return;
			}

			World->AddToRoot();
			WorldContext->SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
		}

		~FTestWorld()
		{
			if (!World)
			{
				return;
			}

			if (World->AreActorsInitialized())
			{
				for (AActor* Actor : FActorRange(World))
				{
					if (Actor)
					{
						Actor->RouteEndPlay(EEndPlayReason::LevelTransition);
					}
				}
			}

			if (GEngine)
			{
				GEngine->ShutdownWorldNetDriver(World);
				World->DestroyWorld(false);
				GEngine->DestroyWorldContext(World);
			}
			else
			{
				World->DestroyWorld(false);
			}

			World->RemoveFromRoot();
			World = nullptr;
			WorldContext = nullptr;
		}

		UWorld* Get() const
		{
			return World;
		}

		bool IsValid() const
		{
			return World != nullptr;
		}

	private:
		UWorld* World = nullptr;
		FWorldContext* WorldContext = nullptr;
	};

	UDroneCompanionConfigDataAsset* NewConfig()
	{
		return NewObject<UDroneCompanionConfigDataAsset>(GetTransientPackage());
	}

	void TickWorld(UWorld* World, int32 NumTicks = 1, float DeltaSeconds = TickDeltaSeconds)
	{
		if (!World)
		{
			return;
		}

		for (int32 TickIndex = 0; TickIndex < NumTicks; ++TickIndex)
		{
			World->Tick(ELevelTick::LEVELTICK_All, DeltaSeconds);
		}
	}

	AActor* SpawnActor(UWorld* World, const FVector& Location)
	{
		if (!World)
		{
			return nullptr;
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Name = MakeUniqueObjectName(World, AActor::StaticClass(), TEXT("DroneCompanionTestActor"), EUniqueObjectNameOptions::GloballyUnique);
		return World->SpawnActor<AActor>(AActor::StaticClass(), Location, FRotator::ZeroRotator, SpawnParameters);
	}

	AActor* SpawnSphereActor(
		UWorld* World,
		const FVector& Location,
		float Radius = DefaultCollisionRadius,
		ECollisionChannel ObjectType = ECC_WorldDynamic,
		ECollisionResponse Response = ECR_Block)
	{
		AActor* Actor = SpawnActor(World, Location);
		if (!Actor)
		{
			return nullptr;
		}

		USphereComponent* Sphere = NewObject<USphereComponent>(Actor, TEXT("TestSphere"));
		Actor->SetRootComponent(Sphere);
		Actor->AddInstanceComponent(Sphere);
		Sphere->InitSphereRadius(Radius);
		Sphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Sphere->SetCollisionObjectType(ObjectType);
		Sphere->SetCollisionResponseToAllChannels(Response);
		Sphere->RegisterComponent();
		Actor->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
		return Actor;
	}

	AActor* SpawnWall(UWorld* World, const FVector& Location, const FVector& Extent)
	{
		AActor* Actor = SpawnActor(World, Location);
		if (!Actor)
		{
			return nullptr;
		}

		UBoxComponent* Box = NewObject<UBoxComponent>(Actor, TEXT("TestWall"));
		Actor->SetRootComponent(Box);
		Actor->AddInstanceComponent(Box);
		Box->SetBoxExtent(Extent);
		Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Box->SetCollisionObjectType(ECC_WorldStatic);
		Box->SetCollisionResponseToAllChannels(ECR_Block);
		Box->RegisterComponent();
		Actor->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
		return Actor;
	}

	template <typename MarkerType>
	MarkerType* AddMarker(AActor* Actor)
	{
		if (!Actor)
		{
			return nullptr;
		}

		MarkerType* Marker = NewObject<MarkerType>(Actor);
		Actor->AddInstanceComponent(Marker);
		Marker->RegisterComponent();
		return Marker;
	}

	template <typename MarkerType>
	AActor* SpawnMarkedActor(UWorld* World, const FVector& Location, MarkerType*& OutMarker)
	{
		AActor* Actor = SpawnSphereActor(World, Location, DefaultCollisionRadius, ECC_WorldDynamic, ECR_Block);
		OutMarker = AddMarker<MarkerType>(Actor);
		return Actor;
	}

	UDroneCompanionSensorComponent* AddSensor(AActor* Owner, UDroneCompanionConfigDataAsset* Config)
	{
		if (!Owner)
		{
			return nullptr;
		}

		UDroneCompanionSensorComponent* Sensor = NewObject<UDroneCompanionSensorComponent>(Owner);
		Owner->AddInstanceComponent(Sensor);
		Sensor->RegisterComponent();
		Sensor->SetConfig(Config);
		return Sensor;
	}

	APawn* SpawnMovementPawn(
		UWorld* World,
		const FVector& Location,
		UDroneCompanionMovementComponent*& OutMovement,
		USphereComponent*& OutCollision,
		UDroneCompanionConfigDataAsset* Config = nullptr)
	{
		OutMovement = nullptr;
		OutCollision = nullptr;

		if (!World)
		{
			return nullptr;
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Name = MakeUniqueObjectName(World, APawn::StaticClass(), TEXT("DroneCompanionMovementPawn"), EUniqueObjectNameOptions::GloballyUnique);
		APawn* Pawn = World->SpawnActor<APawn>(APawn::StaticClass(), Location, FRotator::ZeroRotator, SpawnParameters);
		if (!Pawn)
		{
			return nullptr;
		}

		OutCollision = NewObject<USphereComponent>(Pawn, TEXT("MovementCollision"));
		Pawn->SetRootComponent(OutCollision);
		Pawn->AddInstanceComponent(OutCollision);
		OutCollision->InitSphereRadius(DefaultCollisionRadius);
		OutCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		OutCollision->SetCollisionObjectType(ECC_Pawn);
		OutCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
		OutCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		OutCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		OutCollision->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
		OutCollision->RegisterComponent();
		Pawn->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);

		OutMovement = NewObject<UDroneCompanionMovementComponent>(Pawn);
		Pawn->AddInstanceComponent(OutMovement);
		OutMovement->RegisterComponent();
		OutMovement->InitializeMovement(OutCollision, Config);
		return Pawn;
	}

	UDroneCompanionFollowComponent* AddFollowComponent(
		APawn* Pawn,
		UDroneCompanionMovementComponent* Movement,
		UDroneCompanionConfigDataAsset* Config,
		AActor* Target)
	{
		if (!Pawn)
		{
			return nullptr;
		}

		UDroneCompanionFollowComponent* Follow = NewObject<UDroneCompanionFollowComponent>(Pawn);
		Pawn->AddInstanceComponent(Follow);
		Follow->RegisterComponent();
		Follow->SetConfig(Config);
		Follow->SetMovementComponent(Movement);
		Follow->SetFollowTarget(Target);
		return Follow;
	}

	bool SetPawnConfig(ADroneCompanionPawn* Pawn, UDroneCompanionConfigDataAsset* Config)
	{
		if (!Pawn)
		{
			return false;
		}

		FObjectProperty* ConfigProperty = FindFProperty<FObjectProperty>(ADroneCompanionPawn::StaticClass(), TEXT("Config"));
		if (!ConfigProperty)
		{
			return false;
		}

		ConfigProperty->SetObjectPropertyValue_InContainer(Pawn, Config);
		return true;
	}

	void TickBrain(UDroneCompanionBrainComponent* Brain, float DeltaSeconds = TickDeltaSeconds)
	{
		if (!Brain)
		{
			return;
		}

		FActorComponentTickFunction TickFunction;
		Brain->TickComponent(DeltaSeconds, LEVELTICK_All, &TickFunction);
	}

	struct FBrainTestRig
	{
		ADroneCompanionPawn* Pawn = nullptr;
		UDroneCompanionConfigDataAsset* Config = nullptr;
		UDroneCompanionMovementComponent* Movement = nullptr;
		UDroneCompanionFollowComponent* Follow = nullptr;
		UDroneCompanionSensorComponent* Sensor = nullptr;
		UDroneCompanionFeedbackComponent* Feedback = nullptr;
		UDroneCompanionCombatComponent* Combat = nullptr;
		UDroneCompanionBrainComponent* Brain = nullptr;
	};

	FBrainTestRig CreateBrainRig(UWorld* World)
	{
		FBrainTestRig Rig;
		if (!World)
		{
			return Rig;
		}

		Rig.Config = NewConfig();
		Rig.Config->bRequireLineOfSightForDetection = false;
		Rig.Config->ScanRadius = 1000.0f;
		Rig.Config->AttackRange = 1200.0f;
		Rig.Config->FireCooldown = 0.5f;
		Rig.Config->bApplyDamageOnHit = false;

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Name = MakeUniqueObjectName(World, ADroneCompanionPawn::StaticClass(), TEXT("DroneCompanionBrainPawn"), EUniqueObjectNameOptions::GloballyUnique);
		Rig.Pawn = World->SpawnActor<ADroneCompanionPawn>(ADroneCompanionPawn::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
		if (!Rig.Pawn)
		{
			return Rig;
		}

		Rig.Movement = Rig.Pawn->FindComponentByClass<UDroneCompanionMovementComponent>();
		Rig.Follow = Rig.Pawn->FindComponentByClass<UDroneCompanionFollowComponent>();
		Rig.Sensor = Rig.Pawn->FindComponentByClass<UDroneCompanionSensorComponent>();
		Rig.Feedback = Rig.Pawn->FindComponentByClass<UDroneCompanionFeedbackComponent>();
		Rig.Combat = Rig.Pawn->FindComponentByClass<UDroneCompanionCombatComponent>();
		Rig.Brain = Rig.Pawn->FindComponentByClass<UDroneCompanionBrainComponent>();

		if (Rig.Movement)
		{
			Rig.Movement->InitializeMovement(Rig.Pawn->GetRootComponent(), Rig.Config);
		}

		if (Rig.Follow)
		{
			Rig.Follow->SetConfig(Rig.Config);
			Rig.Follow->SetMovementComponent(Rig.Movement);
		}

		if (Rig.Sensor)
		{
			Rig.Sensor->SetConfig(Rig.Config);
		}

		if (Rig.Feedback)
		{
			Rig.Feedback->InitializeFeedback(nullptr, nullptr, Rig.Config);
		}

		if (Rig.Combat)
		{
			Rig.Combat->InitializeCombat(nullptr, Rig.Config);
		}

		if (Rig.Brain)
		{
			Rig.Brain->InitializeBrain(Rig.Pawn, Rig.Config, Rig.Follow, Rig.Movement, Rig.Sensor, Rig.Feedback, Rig.Combat);
			Rig.Brain->StartBrain();
		}

		return Rig;
	}
}

#define DRONE_COMPANION_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneCompanionMarkerDefaultsTest, "DroneCompanion.Targets.MarkerDefaults", DRONE_COMPANION_TEST_FLAGS)
bool FDroneCompanionMarkerDefaultsTest::RunTest(const FString& Parameters)
{
	using namespace DroneCompanionAutomation;

	const UDroneCompanionTargetMarkerComponent* BaseMarker = NewObject<UDroneCompanionTargetMarkerComponent>();
	const UDroneCompanionCollectibleMarkerComponent* CollectibleMarker = NewObject<UDroneCompanionCollectibleMarkerComponent>();
	const UDroneCompanionEnemyMarkerComponent* EnemyMarker = NewObject<UDroneCompanionEnemyMarkerComponent>();

	TestEqual(TEXT("Base marker defaults to no target type."), BaseMarker->TargetType, EDroneCompanionTargetType::None);
	TestTrue(TEXT("Base marker is detectable by default."), BaseMarker->bIsDetectable);
	TestEqual(TEXT("Base marker has no priority bonus by default."), BaseMarker->PriorityBonus, 0.0f);
	TestEqual(TEXT("Collectible marker defaults to collectible type."), CollectibleMarker->TargetType, EDroneCompanionTargetType::Collectible);
	TestTrue(TEXT("Collectible marker inherits detectable default."), CollectibleMarker->bIsDetectable);
	TestEqual(TEXT("Collectible marker inherits no priority bonus by default."), CollectibleMarker->PriorityBonus, 0.0f);
	TestEqual(TEXT("Enemy marker defaults to enemy type."), EnemyMarker->TargetType, EDroneCompanionTargetType::Enemy);
	TestTrue(TEXT("Enemy marker inherits detectable default."), EnemyMarker->bIsDetectable);
	TestEqual(TEXT("Enemy marker inherits no priority bonus by default."), EnemyMarker->PriorityBonus, 0.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneCompanionSensorEnemyOutranksCollectibleTest, "DroneCompanion.Sensor.EnemyOutranksCollectible", DRONE_COMPANION_TEST_FLAGS)
bool FDroneCompanionSensorEnemyOutranksCollectibleTest::RunTest(const FString& Parameters)
{
	using namespace DroneCompanionAutomation;

	FTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world was created."), TestWorld.IsValid()))
	{
		return false;
	}

	UDroneCompanionConfigDataAsset* Config = NewConfig();
	Config->bRequireLineOfSightForDetection = false;
	AActor* Owner = SpawnSphereActor(TestWorld.Get(), FVector::ZeroVector, DefaultCollisionRadius, ECC_Pawn, ECR_Ignore);
	UDroneCompanionSensorComponent* Sensor = AddSensor(Owner, Config);
	UDroneCompanionCollectibleMarkerComponent* CollectibleMarker = nullptr;
	UDroneCompanionEnemyMarkerComponent* EnemyMarker = nullptr;
	SpawnMarkedActor(TestWorld.Get(), FVector(100.0f, 0.0f, 0.0f), CollectibleMarker);
	AActor* Enemy = SpawnMarkedActor(TestWorld.Get(), FVector(900.0f, 0.0f, 0.0f), EnemyMarker);

	bool bChangedBroadcast = false;
	FDroneCompanionTargetInfo ChangedTargetInfo;
	Sensor->OnBestTargetChanged.AddLambda([&bChangedBroadcast, &ChangedTargetInfo](const FDroneCompanionTargetInfo& TargetInfo)
	{
		bChangedBroadcast = true;
		ChangedTargetInfo = TargetInfo;
	});

	Sensor->ScanForTargets();

	TestTrue(TEXT("Sensor found a best target."), Sensor->HasBestTarget());
	TestEqual(TEXT("Enemy target normally outranks collectible target."), Sensor->GetBestTargetType(), EDroneCompanionTargetType::Enemy);
	TestEqual(TEXT("The enemy actor is selected."), Sensor->GetBestTargetActor(), Enemy);
	TestTrue(TEXT("Best target changed delegate fires for the first valid best target."), bChangedBroadcast);
	TestEqual(TEXT("Changed delegate reports the selected enemy actor."), ChangedTargetInfo.TargetActor.Get(), Enemy);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneCompanionSensorCloserSameTypeWinsTest, "DroneCompanion.Sensor.CloserSameTypeWins", DRONE_COMPANION_TEST_FLAGS)
bool FDroneCompanionSensorCloserSameTypeWinsTest::RunTest(const FString& Parameters)
{
	using namespace DroneCompanionAutomation;

	FTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world was created."), TestWorld.IsValid()))
	{
		return false;
	}

	UDroneCompanionConfigDataAsset* Config = NewConfig();
	Config->bRequireLineOfSightForDetection = false;
	AActor* Owner = SpawnSphereActor(TestWorld.Get(), FVector::ZeroVector, DefaultCollisionRadius, ECC_Pawn, ECR_Ignore);
	UDroneCompanionSensorComponent* Sensor = AddSensor(Owner, Config);
	UDroneCompanionCollectibleMarkerComponent* NearMarker = nullptr;
	UDroneCompanionCollectibleMarkerComponent* FarMarker = nullptr;
	AActor* NearCollectible = SpawnMarkedActor(TestWorld.Get(), FVector(100.0f, 0.0f, 0.0f), NearMarker);
	SpawnMarkedActor(TestWorld.Get(), FVector(800.0f, 0.0f, 0.0f), FarMarker);

	Sensor->ScanForTargets();

	TestTrue(TEXT("Sensor found a best target."), Sensor->HasBestTarget());
	TestEqual(TEXT("Closer collectible wins among same target type."), Sensor->GetBestTargetActor(), NearCollectible);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneCompanionSensorPriorityBonusTest, "DroneCompanion.Sensor.PriorityBonusAffectsScore", DRONE_COMPANION_TEST_FLAGS)
bool FDroneCompanionSensorPriorityBonusTest::RunTest(const FString& Parameters)
{
	using namespace DroneCompanionAutomation;

	FTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world was created."), TestWorld.IsValid()))
	{
		return false;
	}

	UDroneCompanionConfigDataAsset* Config = NewConfig();
	Config->bRequireLineOfSightForDetection = false;
	AActor* Owner = SpawnSphereActor(TestWorld.Get(), FVector::ZeroVector, DefaultCollisionRadius, ECC_Pawn, ECR_Ignore);
	UDroneCompanionSensorComponent* Sensor = AddSensor(Owner, Config);
	UDroneCompanionCollectibleMarkerComponent* NearMarker = nullptr;
	UDroneCompanionCollectibleMarkerComponent* BoostedMarker = nullptr;
	SpawnMarkedActor(TestWorld.Get(), FVector(100.0f, 0.0f, 0.0f), NearMarker);
	AActor* BoostedCollectible = SpawnMarkedActor(TestWorld.Get(), FVector(800.0f, 0.0f, 0.0f), BoostedMarker);
	BoostedMarker->PriorityBonus = 100.0f;

	Sensor->ScanForTargets();

	TestTrue(TEXT("Sensor found a best target."), Sensor->HasBestTarget());
	TestEqual(TEXT("PriorityBonus contributes to scoring."), Sensor->GetBestTargetActor(), BoostedCollectible);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneCompanionSensorDisabledMarkerLostTest, "DroneCompanion.Sensor.DisabledMarkerIsIgnoredAndLostBroadcasts", DRONE_COMPANION_TEST_FLAGS)
bool FDroneCompanionSensorDisabledMarkerLostTest::RunTest(const FString& Parameters)
{
	using namespace DroneCompanionAutomation;

	FTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world was created."), TestWorld.IsValid()))
	{
		return false;
	}

	UDroneCompanionConfigDataAsset* Config = NewConfig();
	Config->bRequireLineOfSightForDetection = false;
	AActor* Owner = SpawnSphereActor(TestWorld.Get(), FVector::ZeroVector, DefaultCollisionRadius, ECC_Pawn, ECR_Ignore);
	UDroneCompanionSensorComponent* Sensor = AddSensor(Owner, Config);
	UDroneCompanionCollectibleMarkerComponent* Marker = nullptr;
	SpawnMarkedActor(TestWorld.Get(), FVector(100.0f, 0.0f, 0.0f), Marker);

	Sensor->ScanForTargets();
	TestTrue(TEXT("Sensor initially finds the enabled marker."), Sensor->HasBestTarget());

	bool bLostBroadcast = false;
	Sensor->OnBestTargetLost.AddLambda([&bLostBroadcast]()
	{
		bLostBroadcast = true;
	});

	Marker->bIsDetectable = false;
	Sensor->ScanForTargets();

	TestFalse(TEXT("Disabled marker is ignored."), Sensor->HasBestTarget());
	TestTrue(TEXT("Best target lost delegate fires when the only target becomes undetectable."), bLostBroadcast);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneCompanionSensorDestroyedTargetLostTest, "DroneCompanion.Sensor.DestroyedBestTargetIsHandledSafely", DRONE_COMPANION_TEST_FLAGS)
bool FDroneCompanionSensorDestroyedTargetLostTest::RunTest(const FString& Parameters)
{
	using namespace DroneCompanionAutomation;

	FTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world was created."), TestWorld.IsValid()))
	{
		return false;
	}

	UDroneCompanionConfigDataAsset* Config = NewConfig();
	Config->bRequireLineOfSightForDetection = false;
	AActor* Owner = SpawnSphereActor(TestWorld.Get(), FVector::ZeroVector, DefaultCollisionRadius, ECC_Pawn, ECR_Ignore);
	UDroneCompanionSensorComponent* Sensor = AddSensor(Owner, Config);
	UDroneCompanionEnemyMarkerComponent* Marker = nullptr;
	AActor* Target = SpawnMarkedActor(TestWorld.Get(), FVector(100.0f, 0.0f, 0.0f), Marker);

	Sensor->ScanForTargets();
	TestTrue(TEXT("Sensor initially finds the target before destruction."), Sensor->HasBestTarget());

	bool bLostBroadcast = false;
	Sensor->OnBestTargetLost.AddLambda([&bLostBroadcast]()
	{
		bLostBroadcast = true;
	});

	Target->Destroy();
	TickWorld(TestWorld.Get());
	Sensor->ScanForTargets();

	TestFalse(TEXT("Destroyed best target is no longer valid."), Sensor->HasBestTarget());
	TestTrue(TEXT("Best target lost delegate fires when the best target is destroyed."), bLostBroadcast);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneCompanionFollowDisabledPreservesTargetTest, "DroneCompanion.Follow.DisabledFollowPreservesTarget", DRONE_COMPANION_TEST_FLAGS)
bool FDroneCompanionFollowDisabledPreservesTargetTest::RunTest(const FString& Parameters)
{
	using namespace DroneCompanionAutomation;

	FTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world was created."), TestWorld.IsValid()))
	{
		return false;
	}

	UDroneCompanionMovementComponent* Movement = nullptr;
	USphereComponent* Collision = nullptr;
	APawn* Pawn = SpawnMovementPawn(TestWorld.Get(), FVector::ZeroVector, Movement, Collision);
	AActor* Target = SpawnActor(TestWorld.Get(), FVector(500.0f, 0.0f, 0.0f));
	UDroneCompanionFollowComponent* Follow = AddFollowComponent(Pawn, Movement, nullptr, Target);

	Follow->SetFollowEnabled(false);
	FActorComponentTickFunction TickFunction;
	Follow->TickComponent(TickDeltaSeconds, LEVELTICK_All, &TickFunction);

	TestFalse(TEXT("Follow reports disabled state."), Follow->IsFollowEnabled());
	TestEqual(TEXT("Disabling follow does not clear the follow target."), Follow->GetFollowTarget(), Target);

	Follow->SetFollowEnabled(true);
	TestTrue(TEXT("Follow can be re-enabled."), Follow->IsFollowEnabled());
	TestEqual(TEXT("Re-enabling follow preserves the target."), Follow->GetFollowTarget(), Target);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneCompanionFollowInvalidMoveSpeedFallbackTest, "DroneCompanion.Follow.InvalidMoveSpeedUsesFallback", DRONE_COMPANION_TEST_FLAGS)
bool FDroneCompanionFollowInvalidMoveSpeedFallbackTest::RunTest(const FString& Parameters)
{
	using namespace DroneCompanionAutomation;

	FTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world was created."), TestWorld.IsValid()))
	{
		return false;
	}

	UDroneCompanionConfigDataAsset* Config = NewConfig();
	Config->FollowDistance = 0.0f;
	Config->FollowHeight = 0.0f;
	Config->MoveSpeed = -1.0f;
	Config->AcceptanceRadius = 1.0f;

	UDroneCompanionMovementComponent* Movement = nullptr;
	USphereComponent* Collision = nullptr;
	APawn* Pawn = SpawnMovementPawn(TestWorld.Get(), FVector::ZeroVector, Movement, Collision, Config);
	AActor* Target = SpawnSphereActor(TestWorld.Get(), FVector(300.0f, 0.0f, 0.0f), DefaultCollisionRadius, ECC_WorldDynamic, ECR_Ignore);
	UDroneCompanionFollowComponent* Follow = AddFollowComponent(Pawn, Movement, Config, Target);

	FActorComponentTickFunction TickFunction;
	Follow->TickComponent(TickDeltaSeconds, LEVELTICK_All, &TickFunction);

	TestTrue(TEXT("Invalid MoveSpeed falls back to a usable speed."), Pawn->GetActorLocation().X > 1.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneCompanionMovementMissingUpdatedComponentTest, "DroneCompanion.Movement.MissingUpdatedComponentFailsSafely", DRONE_COMPANION_TEST_FLAGS)
bool FDroneCompanionMovementMissingUpdatedComponentTest::RunTest(const FString& Parameters)
{
	UDroneCompanionMovementComponent* Movement = NewObject<UDroneCompanionMovementComponent>(GetTransientPackage());

	TestFalse(TEXT("MoveTowardLocation fails safely without an UpdatedComponent."), Movement->MoveTowardLocation(FVector(100.0f, 0.0f, 0.0f), 600.0f, 10.0f, 0.1f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneCompanionMovementReachableTargetTest, "DroneCompanion.Movement.MovesTowardReachableTarget", DRONE_COMPANION_TEST_FLAGS)
bool FDroneCompanionMovementReachableTargetTest::RunTest(const FString& Parameters)
{
	using namespace DroneCompanionAutomation;

	FTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world was created."), TestWorld.IsValid()))
	{
		return false;
	}

	UDroneCompanionMovementComponent* Movement = nullptr;
	USphereComponent* Collision = nullptr;
	APawn* Pawn = SpawnMovementPawn(TestWorld.Get(), FVector::ZeroVector, Movement, Collision);

	const bool bReached = Movement->MoveTowardLocation(FVector(100.0f, 0.0f, 0.0f), 1000.0f, 1.0f, TickDeltaSeconds);

	TestTrue(TEXT("Movement reaches a reachable target within acceptance radius."), bReached);
	TestTrue(TEXT("Pawn moved toward the desired location."), Pawn->GetActorLocation().X > 90.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneCompanionMovementBlockingWallTest, "DroneCompanion.Movement.DoesNotPassThroughBlockingWall", DRONE_COMPANION_TEST_FLAGS)
bool FDroneCompanionMovementBlockingWallTest::RunTest(const FString& Parameters)
{
	using namespace DroneCompanionAutomation;

	FTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world was created."), TestWorld.IsValid()))
	{
		return false;
	}

	UDroneCompanionMovementComponent* Movement = nullptr;
	USphereComponent* Collision = nullptr;
	APawn* Pawn = SpawnMovementPawn(TestWorld.Get(), FVector::ZeroVector, Movement, Collision);
	SpawnWall(TestWorld.Get(), FVector(100.0f, 0.0f, 0.0f), FVector(10.0f, 200.0f, 200.0f));
	TickWorld(TestWorld.Get());

	Movement->MoveTowardLocation(FVector(300.0f, 0.0f, 0.0f), 1000.0f, 1.0f, 1.0f);

	TestTrue(TEXT("Swept movement does not cross the blocking wall."), Pawn->GetActorLocation().X < 80.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneCompanionPawnInvalidCollisionRadiusFallbackTest, "DroneCompanion.Pawn.InvalidCollisionRadiusUsesFallback", DRONE_COMPANION_TEST_FLAGS)
bool FDroneCompanionPawnInvalidCollisionRadiusFallbackTest::RunTest(const FString& Parameters)
{
	using namespace DroneCompanionAutomation;

	FTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world was created."), TestWorld.IsValid()))
	{
		return false;
	}

	UDroneCompanionConfigDataAsset* Config = NewConfig();
	Config->DroneCollisionRadius = -10.0f;

	ADroneCompanionPawn* Pawn = TestWorld.Get()->SpawnActor<ADroneCompanionPawn>(ADroneCompanionPawn::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Drone pawn spawned."), Pawn))
	{
		return false;
	}

	if (!TestTrue(TEXT("Config property was assigned through reflection for the test."), SetPawnConfig(Pawn, Config)))
	{
		return false;
	}

	Pawn->DispatchBeginPlay();

	USphereComponent* Collision = Pawn->FindComponentByClass<USphereComponent>();
	if (!TestNotNull(TEXT("Pawn has a sphere collision component."), Collision))
	{
		return false;
	}

	TestEqual(TEXT("Invalid collision radius uses the pawn fallback radius."), Collision->GetUnscaledSphereRadius(), PawnFallbackCollisionRadius);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneCompanionCombatNullOutOfRangeTest, "DroneCompanion.Combat.NullAndOutOfRangeTargetsFailSafely", DRONE_COMPANION_TEST_FLAGS)
bool FDroneCompanionCombatNullOutOfRangeTest::RunTest(const FString& Parameters)
{
	using namespace DroneCompanionAutomation;

	FTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world was created."), TestWorld.IsValid()))
	{
		return false;
	}

	UDroneCompanionConfigDataAsset* Config = NewConfig();
	Config->AttackRange = 100.0f;
	Config->bApplyDamageOnHit = false;
	AActor* Owner = SpawnSphereActor(TestWorld.Get(), FVector::ZeroVector, DefaultCollisionRadius, ECC_Pawn, ECR_Ignore);
	UDroneCompanionCombatComponent* Combat = NewObject<UDroneCompanionCombatComponent>(Owner);
	Owner->AddInstanceComponent(Combat);
	Combat->RegisterComponent();
	Combat->InitializeCombat(Owner->GetRootComponent(), Config);
	AActor* FarTarget = SpawnSphereActor(TestWorld.Get(), FVector(1000.0f, 0.0f, 0.0f));

	TestFalse(TEXT("TryFireAtTarget fails safely with null target."), Combat->TryFireAtTarget(nullptr));
	TestFalse(TEXT("Out-of-range target cannot be fired on."), Combat->TryFireAtTarget(FarTarget));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneCompanionCombatCooldownAndRangeFallbackTest, "DroneCompanion.Combat.RespectsCooldownAndInvalidRangeFallback", DRONE_COMPANION_TEST_FLAGS)
bool FDroneCompanionCombatCooldownAndRangeFallbackTest::RunTest(const FString& Parameters)
{
	using namespace DroneCompanionAutomation;

	FTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world was created."), TestWorld.IsValid()))
	{
		return false;
	}

	UDroneCompanionConfigDataAsset* Config = NewConfig();
	Config->AttackRange = -1.0f;
	Config->FireCooldown = 0.5f;
	Config->bApplyDamageOnHit = false;
	AActor* Owner = SpawnSphereActor(TestWorld.Get(), FVector::ZeroVector, DefaultCollisionRadius, ECC_Pawn, ECR_Ignore);
	UDroneCompanionCombatComponent* Combat = NewObject<UDroneCompanionCombatComponent>(Owner);
	Owner->AddInstanceComponent(Combat);
	Combat->RegisterComponent();
	Combat->InitializeCombat(Owner->GetRootComponent(), Config);
	AActor* Target = SpawnSphereActor(TestWorld.Get(), FVector(500.0f, 0.0f, 0.0f));

	TestTrue(TEXT("Invalid AttackRange falls back to the default combat range."), Combat->TryFireAtTarget(Target));
	TestFalse(TEXT("CanFire is false immediately after a successful shot starts cooldown."), Combat->CanFire());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneCompanionCombatBlockedShotTest, "DroneCompanion.Combat.BlockedShotFails", DRONE_COMPANION_TEST_FLAGS)
bool FDroneCompanionCombatBlockedShotTest::RunTest(const FString& Parameters)
{
	using namespace DroneCompanionAutomation;

	FTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world was created."), TestWorld.IsValid()))
	{
		return false;
	}

	UDroneCompanionConfigDataAsset* Config = NewConfig();
	Config->AttackRange = 1000.0f;
	Config->bApplyDamageOnHit = false;
	AActor* Owner = SpawnSphereActor(TestWorld.Get(), FVector::ZeroVector, DefaultCollisionRadius, ECC_Pawn, ECR_Ignore);
	UDroneCompanionCombatComponent* Combat = NewObject<UDroneCompanionCombatComponent>(Owner);
	Owner->AddInstanceComponent(Combat);
	Combat->RegisterComponent();
	Combat->InitializeCombat(Owner->GetRootComponent(), Config);
	AActor* Target = SpawnSphereActor(TestWorld.Get(), FVector(500.0f, 0.0f, 0.0f));
	SpawnWall(TestWorld.Get(), FVector(250.0f, 0.0f, 0.0f), FVector(10.0f, 100.0f, 100.0f));

	TestFalse(TEXT("HasClearShot fails when a blocking actor is between muzzle and target."), Combat->HasClearShot(Target));
	TestFalse(TEXT("Blocked shot does not count as a successful hit."), Combat->TryFireAtTarget(Target));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneCompanionBrainFollowToInspectionTest, "DroneCompanion.Brain.FollowTransitionsToCollectibleInspection", DRONE_COMPANION_TEST_FLAGS)
bool FDroneCompanionBrainFollowToInspectionTest::RunTest(const FString& Parameters)
{
	using namespace DroneCompanionAutomation;

	FTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world was created."), TestWorld.IsValid()))
	{
		return false;
	}

	FBrainTestRig Rig = CreateBrainRig(TestWorld.Get());
	if (!TestNotNull(TEXT("Brain component exists."), Rig.Brain) || !TestNotNull(TEXT("Sensor component exists."), Rig.Sensor))
	{
		return false;
	}

	UDroneCompanionCollectibleMarkerComponent* Marker = nullptr;
	SpawnMarkedActor(TestWorld.Get(), FVector(200.0f, 0.0f, 0.0f), Marker);
	Rig.Sensor->ScanForTargets();
	TickBrain(Rig.Brain);

	TestEqual(TEXT("Follow state transitions to collectible inspection."), Rig.Brain->GetCurrentStateName(), FName(TEXT("InspectCollectible")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneCompanionBrainEnemyInterruptsInspectionTest, "DroneCompanion.Brain.EnemyInterruptsInspection", DRONE_COMPANION_TEST_FLAGS)
bool FDroneCompanionBrainEnemyInterruptsInspectionTest::RunTest(const FString& Parameters)
{
	using namespace DroneCompanionAutomation;

	FTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world was created."), TestWorld.IsValid()))
	{
		return false;
	}

	FBrainTestRig Rig = CreateBrainRig(TestWorld.Get());
	if (!TestNotNull(TEXT("Brain component exists."), Rig.Brain) || !TestNotNull(TEXT("Sensor component exists."), Rig.Sensor))
	{
		return false;
	}

	UDroneCompanionCollectibleMarkerComponent* CollectibleMarker = nullptr;
	SpawnMarkedActor(TestWorld.Get(), FVector(200.0f, 0.0f, 0.0f), CollectibleMarker);
	Rig.Sensor->ScanForTargets();
	TickBrain(Rig.Brain);
	TestEqual(TEXT("Brain entered collectible inspection before the interrupt."), Rig.Brain->GetCurrentStateName(), FName(TEXT("InspectCollectible")));

	UDroneCompanionEnemyMarkerComponent* EnemyMarker = nullptr;
	SpawnMarkedActor(TestWorld.Get(), FVector(300.0f, 0.0f, 0.0f), EnemyMarker);
	Rig.Sensor->ScanForTargets();
	TickBrain(Rig.Brain);

	TestEqual(TEXT("Enemy best target interrupts inspection and enters attack state."), Rig.Brain->GetCurrentStateName(), FName(TEXT("AttackEnemy")));

	return true;
}

#undef DRONE_COMPANION_TEST_FLAGS

#endif // WITH_DEV_AUTOMATION_TESTS
