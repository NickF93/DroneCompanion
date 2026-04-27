#include "DroneCompanionPawn.h"

#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DroneCompanionConfigDataAsset.h"
#include "DroneCompanionFollowComponent.h"
#include "DroneCompanionRuntimeModule.h"
#include "Kismet/GameplayStatics.h"
#include "Math/Vector.h"

ADroneCompanionPawn::ADroneCompanionPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DroneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DroneMesh"));
	DroneMesh->SetupAttachment(SceneRoot);

	MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
	MuzzlePoint->SetupAttachment(DroneMesh);
	MuzzlePoint->SetRelativeLocation(FVector(75.0f, 0.0f, 0.0f));

	StatusLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StatusLight"));
	StatusLight->SetupAttachment(SceneRoot);
	StatusLight->SetRelativeLocation(FVector(0.0f, 0.0f, 35.0f));
	StatusLight->SetIntensity(500.0f);

	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	AudioComponent->SetupAttachment(SceneRoot);
	AudioComponent->SetAutoActivate(false);

	FollowComponent = CreateDefaultSubobject<UDroneCompanionFollowComponent>(TEXT("FollowComponent"));
}

void ADroneCompanionPawn::BeginPlay()
{
	Super::BeginPlay();

	if (!Config)
	{
		UE_LOG(LogDroneCompanion, Warning, TEXT("%s has no Drone Companion config asset assigned. Follow defaults will be used."), *GetName());
	}

	if (!FollowComponent)
	{
		UE_LOG(LogDroneCompanion, Warning, TEXT("%s has no Drone Companion follow component."), *GetName());
		return;
	}

	FollowComponent->SetConfig(Config);

	if (FollowComponent->HasValidFollowTarget())
	{
		return;
	}

	if (InitialFollowTarget)
	{
		FollowComponent->SetFollowTarget(InitialFollowTarget);
		UE_LOG(LogDroneCompanion, Log, TEXT("%s acquired explicit follow target %s."), *GetName(), *InitialFollowTarget->GetName());
		return;
	}

	const bool bShouldAutoAcquirePlayer = !Config || Config->bAutoAcquirePlayerOnBeginPlay;
	if (!bShouldAutoAcquirePlayer)
	{
		UE_LOG(LogDroneCompanion, Log, TEXT("%s did not acquire a follow target because player auto-acquire is disabled."), *GetName());
		return;
	}

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

void ADroneCompanionPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
