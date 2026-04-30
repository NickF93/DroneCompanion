#include "Core/DroneCompanionPawn.h"

#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DroneCompanionBrainComponent.h"
#include "Components/DroneCompanionCombatComponent.h"
#include "Components/DroneCompanionFeedbackComponent.h"
#include "Components/DroneCompanionFollowComponent.h"
#include "Components/DroneCompanionMovementComponent.h"
#include "Components/DroneCompanionSensorComponent.h"
#include "Core/DroneCompanionConfigDataAsset.h"
#include "Engine/EngineTypes.h"
#include "Module/DroneCompanionRuntimeModule.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Math/Vector.h"
#include "UObject/UObjectGlobals.h"

namespace DroneCompanionPawnDefaults
{
	constexpr float CollisionRadius = 40.0f;
	const FVector MuzzlePointRelativeLocation(75.0f, 0.0f, 0.0f);
	const FVector StatusLightRelativeLocation(0.0f, 0.0f, 35.0f);
	constexpr float StatusLightInitialIntensity = 500.0f;

	float PositiveOrDefault(float Value, float DefaultValue)
	{
		return Value > 0.0f ? Value : DefaultValue;
	}
}

ADroneCompanionPawn::ADroneCompanionPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(DroneCompanionPawnDefaults::CollisionRadius);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(ECC_Pawn);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
	CollisionComponent->SetCanEverAffectNavigation(false);
	SetRootComponent(CollisionComponent);

	DroneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DroneMesh"));
	DroneMesh->SetupAttachment(CollisionComponent);
	DroneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
	MuzzlePoint->SetupAttachment(CollisionComponent);
	MuzzlePoint->SetRelativeLocation(DroneCompanionPawnDefaults::MuzzlePointRelativeLocation);

	StatusLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StatusLight"));
	StatusLight->SetupAttachment(CollisionComponent);
	StatusLight->SetRelativeLocation(DroneCompanionPawnDefaults::StatusLightRelativeLocation);
	StatusLight->SetIntensity(DroneCompanionPawnDefaults::StatusLightInitialIntensity);

	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	AudioComponent->SetupAttachment(CollisionComponent);
	AudioComponent->SetAutoActivate(false);

	MovementComponent = CreateDefaultSubobject<UDroneCompanionMovementComponent>(TEXT("MovementComponent"));
	FollowComponent = CreateDefaultSubobject<UDroneCompanionFollowComponent>(TEXT("FollowComponent"));
	SensorComponent = CreateDefaultSubobject<UDroneCompanionSensorComponent>(TEXT("SensorComponent"));
	FeedbackComponent = CreateDefaultSubobject<UDroneCompanionFeedbackComponent>(TEXT("FeedbackComponent"));
	CombatComponent = CreateDefaultSubobject<UDroneCompanionCombatComponent>(TEXT("CombatComponent"));
	BrainComponent = CreateDefaultSubobject<UDroneCompanionBrainComponent>(TEXT("BrainComponent"));
}

void ADroneCompanionPawn::BeginPlay()
{
	Super::BeginPlay();

	if (!Config)
	{
		UE_LOG(LogDroneCompanion, Warning, TEXT("%s has no Drone Companion config asset assigned. Component defaults will be used."), *GetName());
	}

	if (CollisionComponent)
	{
		const float CollisionRadius = Config
			? DroneCompanionPawnDefaults::PositiveOrDefault(Config->DroneCollisionRadius, DroneCompanionPawnDefaults::CollisionRadius)
			: DroneCompanionPawnDefaults::CollisionRadius;
		CollisionComponent->SetSphereRadius(CollisionRadius, true);
	}

	if (MovementComponent)
	{
		MovementComponent->InitializeMovement(CollisionComponent, Config);
	}
	else
	{
		UE_LOG(LogDroneCompanion, Warning, TEXT("%s has no Drone Companion movement component."), *GetName());
	}

	if (FollowComponent)
	{
		FollowComponent->SetConfig(Config);
		FollowComponent->SetMovementComponent(MovementComponent);

		if (!FollowComponent->HasValidFollowTarget())
		{
			if (IsValid(InitialFollowTarget))
			{
				FollowComponent->SetFollowTarget(InitialFollowTarget);
				UE_LOG(LogDroneCompanion, Log, TEXT("%s acquired explicit follow target %s."), *GetName(), *GetNameSafe(InitialFollowTarget));
			}
			else
			{
				const bool bShouldAutoAcquirePlayer = !Config || Config->bAutoAcquirePlayerOnBeginPlay;
				if (bShouldAutoAcquirePlayer)
				{
					APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
					if (PlayerPawn)
					{
						FollowComponent->SetFollowTarget(PlayerPawn);
						UE_LOG(LogDroneCompanion, Log, TEXT("%s acquired Player 0 follow target %s."), *GetName(), *PlayerPawn->GetName());
					}
					else
					{
						UE_LOG(LogDroneCompanion, Warning, TEXT("%s could not find Player 0 pawn to follow."), *GetName());
					}
				}
				else
				{
					UE_LOG(LogDroneCompanion, Log, TEXT("%s did not acquire a follow target because player auto-acquire is disabled."), *GetName());
				}
			}
		}
	}
	else
	{
		UE_LOG(LogDroneCompanion, Warning, TEXT("%s has no Drone Companion follow component."), *GetName());
	}

	if (SensorComponent)
	{
		SensorComponent->SetConfig(Config);
		SensorComponent->StartSensing();
	}
	else
	{
		UE_LOG(LogDroneCompanion, Warning, TEXT("%s has no Drone Companion sensor component."), *GetName());
	}

	if (FeedbackComponent)
	{
		FeedbackComponent->InitializeFeedback(StatusLight, AudioComponent, Config);
	}
	else
	{
		UE_LOG(LogDroneCompanion, Warning, TEXT("%s has no Drone Companion feedback component."), *GetName());
	}

	if (CombatComponent)
	{
		CombatComponent->InitializeCombat(MuzzlePoint, Config);
	}
	else
	{
		UE_LOG(LogDroneCompanion, Warning, TEXT("%s has no Drone Companion combat component."), *GetName());
	}

	if (BrainComponent)
	{
		BrainComponent->InitializeBrain(this, Config, FollowComponent, MovementComponent, SensorComponent, FeedbackComponent, CombatComponent);
		BrainComponent->StartBrain();
	}
	else
	{
		UE_LOG(LogDroneCompanion, Warning, TEXT("%s has no Drone Companion brain component."), *GetName());
	}
}

void ADroneCompanionPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BrainComponent)
	{
		BrainComponent->StopBrain();
	}

	if (SensorComponent)
	{
		SensorComponent->StopSensing();
	}

	Super::EndPlay(EndPlayReason);
}

void ADroneCompanionPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

UPawnMovementComponent* ADroneCompanionPawn::GetMovementComponent() const
{
	return MovementComponent;
}
