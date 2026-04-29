# Drone Companion Final Report

## Overview

`DroneCompanion` is a Runtime C++ plugin for Unreal Engine 5.5. It implements a course-ready companion drone that follows the player, detects marked collectibles and enemies, inspects collectibles, attacks enemy targets with a simple hitscan, and returns to following when special behavior ends.

## Project Goal

The goal was to build a small, explainable companion AI exercise without relying on large AI frameworks. The plugin demonstrates component-based Unreal C++ architecture, configurable behavior through a DataAsset, and a small GoF State pattern implementation suitable for a blockout Game Programming course submission.

## Runtime Plugin Rationale

The feature is implemented as a Runtime plugin so the drone can be enabled, reused, and packaged independently from a specific game module. The plugin exposes editor-usable C++ types while keeping behavior inside the runtime module.

The module remains intentionally small and depends only on:

- `Core`
- `CoreUObject`
- `Engine`

## C++ and Blueprint Split

Gameplay logic is implemented in C++. Blueprint use is limited to editor-facing work:

- optional child Blueprint of `ADroneCompanionPawn`;
- assigning meshes, materials, sounds, and the config DataAsset;
- adding marker components to blockout actors.

No Blueprint gameplay logic is required for the demo.

## Final Feature Summary

- Player-follow behavior with configurable offset, height, move speed, and acceptance radius.
- Target sensing with sphere overlap, optional visibility line trace, marker filtering, classification, and scoring.
- Collectible inspection that pauses follow movement, hovers above the collectible, plays feedback, waits, and returns to follow.
- Enemy priority that interrupts collectible inspection.
- Enemy attack behavior that faces the enemy, respects range/cooldown/fire angle, fires a simple hitscan, and optionally applies standard Unreal damage.
- Light/audio feedback for idle, collectible, and combat states.
- Debug drawing and logs gated by config booleans.

## Architecture Overview

The pawn acts as the composition root. Runtime behavior is split across focused components, and high-level behavior is coordinated by a small brain component with private state objects.

| Class | Responsibility |
| --- | --- |
| `ADroneCompanionPawn` | Creates default subobjects, exposes editor-facing references, wires components in `BeginPlay`, and stops runtime systems in `EndPlay`. |
| `UDroneCompanionConfigDataAsset` | Stores tunable values and optional sound references. Contains no gameplay logic. |
| `UDroneCompanionFollowComponent` | Owns follow target storage, follow movement, and follow pause/resume. |
| `UDroneCompanionSensorComponent` | Owns overlap scanning, line-of-sight validation, marker filtering, scoring, best-target storage, and target events. |
| `UDroneCompanionBrainComponent` | Owns behavior orchestration, state transitions, and subscriptions to sensor events. |
| Private brain states | Own state-specific behavior for Follow, InspectCollectible, and AttackEnemy. |
| `UDroneCompanionFeedbackComponent` | Owns status light/audio feedback only. |
| `UDroneCompanionCombatComponent` | Owns hitscan execution, cooldown, range checks, fire trace, and optional standard Unreal damage. |
| Marker components | Identify actors as drone-detectable collectible or enemy targets with metadata only. |

## GoF Patterns

State is used in the private brain state classes. `Follow`, `InspectCollectible`, and `AttackEnemy` are separate behavior units with consistent `Enter`, `Exit`, `Tick`, and `GetName` methods.

Observer is used through `UDroneCompanionSensorComponent` delegates. The brain subscribes to best-target changed and lost events, so target changes are communicated without the sensor commanding behavior.

Facade is used in `UDroneCompanionSensorComponent`. Other classes do not need to know the details of overlaps, visibility traces, marker filtering, scoring, or best-target storage.

## SOLID and Authority Boundaries

The design emphasizes Single Responsibility and clear authority boundaries. The pawn composes systems but does not implement behavior. The sensor senses but does not move or attack. The combat component fires but does not choose states. The feedback component controls light/audio but does not decide behavior. The brain decides state transitions but delegates sensing and weapon execution to the relevant components.

Public APIs are intentionally small. Internal state classes remain private to the module. The code avoids project-specific gameplay class references; targets are identified by marker components and generic `AActor` references.

## Configuration

`UDroneCompanionConfigDataAsset` centralizes editor-authored values for follow movement, sensing, collectible inspection, combat, feedback, and debug options. Runtime components handle missing config assets and invalid values with local named fallback constants. Config data is not modified at runtime.

## Target Detection

Targets are marked by adding `UDroneCompanionCollectibleMarkerComponent` or `UDroneCompanionEnemyMarkerComponent` to blockout actors. The sensor scans with a sphere overlap, optionally validates line of sight with `ECC_Visibility`, classifies markers, scores candidates, stores the best target, and emits C++ delegates when meaningful target changes happen.

## Collectible Behavior

When a collectible is the best target, the brain enters `InspectCollectible`. Follow movement is paused, the drone moves to `CollectibleLocation + CollectibleHoverHeight`, collectible feedback is enabled, and the inspection timer counts only after the drone reaches the hover location. Invalid or lost targets return the drone to follow behavior.

## Combat Behavior

When an enemy is the best target, the brain enters `AttackEnemy`. Enemy priority interrupts collectible inspection. Follow movement is paused, the drone rotates toward the enemy, checks fire angle, range, cooldown, and clear shot, then asks the combat component to fire. The combat component performs a visibility hitscan and optionally calls `UGameplayStatics::ApplyDamage` only on a valid enemy hit.

## Intentional Limitations

The plugin intentionally omits AI Perception, Behavior Tree, StateTree, EQS, NavMesh, replication, projectile weapons, custom health systems, enemy AI, inventory/pickup systems, an Editor module, and Blueprint gameplay logic. These omissions keep the submission focused on a readable runtime C++ blockout exercise.

## Possible Future Improvements

- Add obstacle avoidance or pathfinding if the drone must navigate complex levels.
- Add richer feedback assets such as particles or animations.
- Add custom target interfaces if the project later needs target-specific data.
- Add multiplayer support and replication if required by the game mode.
- Add designer tools in a separate Editor module if the project grows beyond the course scope.

## Final Verification Summary

Phase 7 verification was performed from `f:\DBGA\DroneCompanion`.

- Source/descriptors were reviewed for forbidden systems and unnecessary dependencies.
- `DroneCompanionRuntime.Build.cs` remained unchanged with `Core`, `CoreUObject`, and `Engine`.
- `git diff --check` passed on April 29, 2026.
- The UE 5.5 build passed on April 29, 2026 with this command:

```powershell
& "G:\Program Files\Epic Games\UE_5.5\Engine\Build\BatchFiles\Build.bat" DroneCompanionEditor Win64 Development -Project="f:\DBGA\DroneCompanion\DroneCompanion.uproject" -WaitMutex -NoHotReloadFromIDE
```

Manual editor playtests were not executed during this documentation pass. They are documented in `Docs/TestPlan.md` as a checklist for final demonstration evidence.
