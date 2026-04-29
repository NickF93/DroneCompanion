# Drone Companion Plugin

Runtime C++ companion drone plugin for an Unreal Engine 5.5 Game Programming course exercise.

The drone is implemented as a C++ `APawn` with runtime components for following the player, sensing marked targets, inspecting collectibles, and attacking enemies with a simple hitscan. Blueprint use is intentionally limited to editor-facing asset wiring: optional child Blueprints, meshes, materials, sounds, DataAsset assignment, and marker components on blockout actors.

## Setup

1. Enable the `Drone Companion` plugin in the Unreal Editor if it is not already enabled.
2. Regenerate project files if the editor or IDE asks for it.
3. Compile the `DroneCompanionEditor` target with Unreal Engine 5.5.
4. Create a `DroneCompanionConfigDataAsset` Data Asset and assign it to the pawn's `Config` property.
5. Place `ADroneCompanionPawn` or an optional child Blueprint in the level.

## Level Setup

- Assign a mesh, materials, and optional sounds through the pawn child Blueprint or placed instance.
- Leave `bAutoAcquirePlayerOnBeginPlay` enabled to follow Player 0 automatically, or assign `InitialFollowTarget` on the placed drone.
- Add `UDroneCompanionCollectibleMarkerComponent` to blockout collectible actors.
- Add `UDroneCompanionEnemyMarkerComponent` to blockout enemy actors.
- Enemy actors must block the `Visibility` trace channel if they should be hit by the simple hitscan weapon.

## Demo Flow

1. Press Play.
2. The drone follows Player 0 using `FollowDistance`, `FollowHeight`, and `MoveSpeed`.
3. Marked targets inside `ScanRadius` are classified as collectible or enemy.
4. Collectible targets make the drone pause following, hover above the collectible, show collectible feedback, wait for `InspectDuration`, then return to following.
5. Enemy targets take priority, interrupt inspection, make the drone face the enemy, fire hitscan shots using `AttackRange` and `FireCooldown`, then return to following when combat is no longer valid.

## Architecture

- `ADroneCompanionPawn` is the composition root for presentation components and runtime behavior components.
- `UDroneCompanionFollowComponent` handles player-follow movement.
- `UDroneCompanionSensorComponent` is a small sensing facade over overlap, optional visibility trace, filtering, classification, and scoring.
- `UDroneCompanionBrainComponent` coordinates high-level behavior through private C++ state objects.
- `UDroneCompanionFeedbackComponent` controls light and audio feedback.
- `UDroneCompanionCombatComponent` encapsulates simple hitscan firing and optional standard Unreal damage.

## Patterns Used

- State: Follow, InspectCollectible, and AttackEnemy are private C++ behavior states.
- Observer: the sensor exposes C++ multicast delegates for best-target changed and lost events.
- Facade: the sensor component hides overlap, trace, filter, and scoring details behind a compact API.

## Intentional Limitations

This is a blockout course exercise. It intentionally does not use AI Perception, Behavior Tree, StateTree, EQS, NavMesh, replication, projectile weapons, custom health systems, enemy AI, inventory, or Blueprint gameplay logic.
