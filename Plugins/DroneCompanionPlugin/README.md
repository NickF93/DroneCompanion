# Drone Companion Plugin

`DroneCompanion` is a Runtime C++ plugin for Unreal Engine 5.5. It provides a small companion drone suitable for a Game Programming course blockout: the drone follows Player 0, senses marked collectible and enemy actors, inspects collectibles, and attacks enemy targets with a simple hitscan weapon.

The plugin is intentionally C++ first. Blueprint is only used as an editor-facing layer for placing actors, creating an optional child Blueprint of the drone pawn, assigning mesh/material/audio assets, assigning the config DataAsset, and adding marker components to blockout actors. No Blueprint gameplay logic is required.

## What Is Included

- `ADroneCompanionPawn`, a placeable pawn that owns the drone components.
- `UDroneCompanionConfigDataAsset`, an editor-authored configuration asset for movement, sensing, inspection, combat, feedback, and debug settings.
- `UDroneCompanionMovementComponent`, a `UPawnMovementComponent` that executes swept, collision-aware movement.
- Follow, sensor, brain, feedback, and combat components with clear runtime responsibilities.
- Marker components for collectible and enemy blockout actors.
- Private C++ brain states for follow, collectible inspection, and enemy attack.

## Movement and Collision Note

The drone pawn now uses `CollisionComponent`, a `USphereComponent`, as its root component. Visual and feedback components are children of that collision root. Movement execution is centralized in `UDroneCompanionMovementComponent`, which moves the collision root with `SafeMoveUpdatedComponent` and handles blocking hits with `SlideAlongSurface`.

If you created a child Blueprint before this collision-root refactor, recreate it from `ADroneCompanionPawn`. Keeping an old child Blueprint can leave stale inherited component data and make the drone appear not to move correctly.

## Setup

1. Enable the `Drone Companion` plugin in Unreal Editor 5.5 if it is not already enabled.
2. Compile the `DroneCompanionEditor` target.
3. Create a Data Asset using `UDroneCompanionConfigDataAsset`.
4. Place either the raw `ADroneCompanionPawn` or a fresh child Blueprint in the level.
5. Assign the DataAsset to the pawn's `Config` property.
6. Ensure a playable Player 0 pawn exists in the level or assign `InitialFollowTarget` on the placed drone.

For a fresh child Blueprint, create a Blueprint Class with parent `ADroneCompanionPawn`. The inherited root should be `CollisionComponent`. Leave the Event Graph empty and use the Blueprint only for editor wiring, such as assigning a mesh to `DroneMesh` or sounds through the config asset.

## Config DataAsset

Create a `UDroneCompanionConfigDataAsset` and tune the exposed categories:

| Category | Main Values |
| --- | --- |
| Follow | `FollowDistance`, `FollowHeight`, `MoveSpeed`, `AcceptanceRadius`, `bAutoAcquirePlayerOnBeginPlay` |
| Movement | `DroneCollisionRadius`, `bEnableMovementDebug` |
| Scanning | `ScanRadius`, `ScanInterval`, line-of-sight and scoring values |
| Collectibles | `CollectibleHoverHeight`, `InspectDuration`, `InspectAcceptanceRadius` |
| Combat | `AttackRange`, `FireCooldown`, `CombatDamage`, `AimInterpSpeed`, `MaxFireAngleDegrees` |
| Feedback | Light colors/intensities and optional `CollectibleFeedbackSound` / `FireSound` |

The runtime code has fallback values for missing or invalid config values, but the demo is easier to read with an assigned DataAsset.

## Marking Targets

Collectibles and enemies are regular blockout actors with marker components:

- Add `UDroneCompanionCollectibleMarkerComponent` to a collectible actor.
- Add `UDroneCompanionEnemyMarkerComponent` to an enemy actor.

The base marker contains `TargetType`, `bIsDetectable`, and `PriorityBonus`. The derived collectible and enemy markers set the target type automatically. Enemy actors should block the `Visibility` trace channel if hitscan shots and optional standard Unreal damage should register.

## Simple Demo Flow

In Play-In-Editor, the drone tries to acquire Player 0 when `bAutoAcquirePlayerOnBeginPlay` is enabled. It follows behind and above the player using the follow config values. The sensor periodically scans for marked targets. If a collectible is the best target, the drone pauses follow movement, moves above the collectible, plays collectible feedback, waits for `InspectDuration`, and returns to follow. If an enemy becomes the best target, it has priority: the drone faces the enemy, fires hitscan shots through the combat component, and returns to follow when combat is no longer valid.

The system does not use NavMesh or pathfinding. The collision-aware movement is intended for simple blockout walls and surfaces, not full navigation around complex level geometry.

## Debug Options

Debug output is opt-in through the config asset:

- `bEnableFollowDebug`: desired follow position and line.
- `bEnableMovementDebug`: movement target, movement delta, and blocking-hit normal.
- `bEnableSensorDebug`: scan sphere, candidate lines, and best-target marker.
- `bEnableInspectionDebug`: collectible hover position and line.
- `bEnableCombatDebug`: fire traces, hit markers, and combat-specific logs.
- `bEnableBrainDebug`: state transition logs.

Keep these disabled for a clean final demo, then enable them selectively when diagnosing setup issues.

## Architecture Summary

`ADroneCompanionPawn` is the composition root. It owns the collision root and default runtime components, then wires them in `BeginPlay`. Follow and inspection code compute desired positions, but physical movement is executed only by `UDroneCompanionMovementComponent`. Sensing is isolated in `UDroneCompanionSensorComponent`; combat is isolated in `UDroneCompanionCombatComponent`; feedback is isolated in `UDroneCompanionFeedbackComponent`; high-level behavior is coordinated by `UDroneCompanionBrainComponent`.

The main GoF-style patterns are:

- State: private C++ states implement `Follow`, `InspectCollectible`, and `AttackEnemy`.
- Observer: the sensor emits `OnBestTargetChanged` and `OnBestTargetLost`.
- Facade: the sensor hides overlap queries, line traces, marker filtering, scoring, and best-target storage behind a compact component API.

## Intentional Limitations

This is a blockout course exercise. It intentionally omits AI Perception, Behavior Tree, StateTree, EQS, NavMesh, replication, projectile weapons, custom health, enemy AI, inventory/pickup logic, an Editor module, and Blueprint gameplay logic.
