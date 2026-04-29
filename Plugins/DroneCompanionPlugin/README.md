# Drone Companion Plugin

Runtime C++ companion drone plugin for an Unreal Engine 5.5 Game Programming course exercise.

The plugin provides a placeable drone pawn that can follow Player 0, sense marked targets, inspect collectibles, and attack enemy targets with a simple hitscan. The implementation is C++ first. Blueprint is used only as an editor-facing layer for optional child Blueprints, mesh/material/audio assignment, DataAsset assignment, and marker placement on blockout actors.

## Features

- Runtime plugin with one `DroneCompanionRuntime` module.
- `ADroneCompanionPawn` composition root with follow, sensor, brain, feedback, and combat components.
- Configurable `UDroneCompanionConfigDataAsset` for movement, sensing, inspection, combat, feedback, and debug values.
- Marker components for blockout collectibles and enemies.
- C++ state flow for following, collectible inspection, and enemy attack.
- Optional debug drawing controlled by config booleans.

## Setup

1. Enable the `Drone Companion` plugin in Unreal Editor if it is not already enabled.
2. Regenerate project files if the editor or IDE asks for it.
3. Compile the `DroneCompanionEditor` target with Unreal Engine 5.5.
4. Create a `DroneCompanionConfigDataAsset` Data Asset.
5. Place `ADroneCompanionPawn` or an optional child Blueprint in the level.
6. Assign the DataAsset to the pawn's `Config` property.

## DataAsset Setup

Create a Data Asset using `DroneCompanionConfigDataAsset`, then tune the exposed categories:

- Follow: distance, height, movement speed, acceptance radius, and Player 0 auto-acquisition.
- Scanning: scan radius, scan interval, line-of-sight requirement, target scoring, and sensor debug.
- Collectibles: hover height, inspection duration, inspection acceptance radius, and inspection debug.
- Combat: attack range, fire cooldown, damage toggle/value, aim speed, fire angle, and combat debug.
- Feedback: idle, collectible, and combat light colors/intensities plus optional sounds.

If `Config` is missing or contains invalid values such as zero movement speed, the runtime components use local fallback values so the demo remains safe.

## Level Setup

- Assign drone mesh/material/audio assets through the placed pawn or an optional child Blueprint.
- Leave `bAutoAcquirePlayerOnBeginPlay` enabled to follow Player 0 automatically, or assign `InitialFollowTarget` on the placed drone.
- Add `UDroneCompanionCollectibleMarkerComponent` to blockout collectible actors.
- Add `UDroneCompanionEnemyMarkerComponent` to blockout enemy actors.
- Enemy actors should block the `Visibility` trace channel if hitscan shots and optional standard Unreal damage should register.

## Demo Flow

1. Press Play.
2. The drone follows Player 0 using `FollowDistance`, `FollowHeight`, and `MoveSpeed`.
3. The sensor scans marked actors inside `ScanRadius` and classifies them as collectible or enemy.
4. A collectible best target makes the drone pause following, hover above the collectible, show feedback, wait for `InspectDuration`, then return to following.
5. An enemy best target has priority, interrupts inspection, makes the drone face the enemy, fires hitscan shots using `AttackRange` and `FireCooldown`, then returns to following when combat is no longer valid.

## Debug Options

All debug drawing is opt-in through the config asset:

- `bEnableFollowDebug`: desired follow location and line.
- `bEnableSensorDebug`: scan sphere, candidate lines, and best target marker.
- `bEnableInspectionDebug`: collectible hover location and line.
- `bEnableCombatDebug`: fire traces, hit markers, and combat-specific logs.
- `bEnableBrainDebug`: state transition logs.

## Architecture

- `ADroneCompanionPawn` owns default subobjects and wires components together.
- `UDroneCompanionFollowComponent` owns player-follow movement and pause/resume state.
- `UDroneCompanionSensorComponent` is a facade over overlap scanning, visibility traces, marker filtering, scoring, best-target storage, and C++ target events.
- `UDroneCompanionBrainComponent` coordinates high-level behavior and owns the active private C++ state.
- `UDroneCompanionFeedbackComponent` owns status light/audio feedback.
- `UDroneCompanionCombatComponent` owns hitscan firing, range checks, cooldown, traces, and optional `UGameplayStatics::ApplyDamage`.
- Marker components contain target metadata only.
- The config DataAsset contains tunable data and optional asset references only.

## Patterns Used

- State: private C++ states implement Follow, InspectCollectible, and AttackEnemy behavior.
- Observer: the sensor emits C++ multicast delegates when the best target changes or is lost.
- Facade: the sensor hides overlap, trace, filter, scoring, and best-target details behind a compact component API.

## Intentional Limitations

This is a blockout course exercise. It intentionally does not include AI Perception, Behavior Tree, StateTree, EQS, NavMesh, replication, projectile weapons, a custom health system, enemy AI, inventory/pickup logic, an Editor module, or Blueprint gameplay logic.
