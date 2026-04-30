# Drone Companion Final Report

## Project Overview

`DroneCompanion` is a Runtime C++ plugin for Unreal Engine 5.5. It implements a small companion drone for a Game Programming course exercise: the drone follows the player, identifies simple marked targets, inspects collectibles, attacks enemies with a basic hitscan weapon, and returns to normal follow behavior when those actions end.

The project deliberately stays within blockout scope. It demonstrates clear Unreal C++ component design and a few focused design patterns without relying on large AI frameworks.

## Goal and Scope

The goal was to build a readable companion-drone system that can be demonstrated in a simple level. The final behavior covers four visible actions:

- follow Player 0;
- sense collectible and enemy actors marked in the editor;
- inspect collectibles by hovering above them for a configured duration;
- attack enemies with simple hitscan fire.

The implementation intentionally avoids AI Perception, Behavior Tree, StateTree, EQS, NavMesh, replication, projectile weapons, custom health systems, enemy AI, inventory logic, an Editor module, and Blueprint gameplay logic. Those systems would be reasonable in a larger game, but they would also make this course exercise harder to explain and verify.

## Runtime Plugin Rationale

The feature is implemented as a Runtime plugin so it is independent from a project-specific game module. The drone types can be enabled, compiled, and reused as a self-contained runtime feature. The module remains small and depends only on Unreal's core runtime modules:

| Module Dependency | Reason |
| --- | --- |
| `Core` | Basic Unreal types and module support. |
| `CoreUObject` | Reflection, UObject types, DataAssets, and object references. |
| `Engine` | Actor, component, collision, debug drawing, world, timer, and damage APIs. |

No Editor-only module is required.

## C++ and Blueprint Split

The runtime behavior is implemented in C++. Blueprint is reserved for editor-facing setup:

- optional child Blueprint of `ADroneCompanionPawn`;
- visual asset assignment for the drone mesh/materials;
- DataAsset assignment;
- optional sound assignment through the config asset;
- marker component placement on blockout actors.

Blueprint graphs are not needed for gameplay logic. This keeps the behavior inspectable in C++ and makes the architecture easier to discuss during assessment.

## Architecture Overview

`ADroneCompanionPawn` acts as the composition root. It creates the default subobjects, exposes editor-visible component references, initializes the components in `BeginPlay`, and stops the runtime systems in `EndPlay`. It does not implement follow, sensing, inspection, combat, or feedback behavior itself.

The main responsibilities are split as follows:

| Type | Responsibility |
| --- | --- |
| `ADroneCompanionPawn` | Owns the collision root and default components; wires dependencies. |
| `UDroneCompanionConfigDataAsset` | Stores tunable values and optional sound references. |
| `UDroneCompanionMovementComponent` | Executes collision-aware pawn movement. |
| `UDroneCompanionFollowComponent` | Stores the follow target and computes the desired follow position. |
| `UDroneCompanionSensorComponent` | Scans, filters, classifies, scores, and stores the current best target. |
| `UDroneCompanionBrainComponent` | Coordinates high-level behavior and owns state transitions. |
| Private brain states | Implement state-specific behavior for follow, inspection, and attack. |
| `UDroneCompanionFeedbackComponent` | Controls status light and audio feedback. |
| `UDroneCompanionCombatComponent` | Performs range checks, cooldown checks, hitscan traces, and optional standard Unreal damage. |
| Marker components | Mark actors as detectable collectible or enemy targets. |

This separation keeps authority clear. The sensor never commands movement. The combat component never chooses states. The feedback component never decides behavior. The brain decides what mode the drone should be in, then delegates the actual work to the component that owns that responsibility.

## Movement Architecture

The movement refactor introduced a more Unreal-style pawn movement path. `ADroneCompanionPawn` now uses `CollisionComponent`, a `USphereComponent`, as its root. `DroneMesh`, `MuzzlePoint`, `StatusLight`, and `AudioComponent` are attached below that root.

Actual translation is centralized in `UDroneCompanionMovementComponent`, which derives from `UPawnMovementComponent`. Follow and collectible inspection code compute desired locations, but they do not move the actor directly. They call `MoveTowardLocation`, and the movement component moves the updated collision component with `SafeMoveUpdatedComponent`. When a blocking hit occurs, it uses `SlideAlongSurface` so simple blockout walls can stop or deflect the drone instead of being ignored.

This is not a navigation system. There is no NavMesh, pathfinding, or search for alternative routes. The design is intentionally simple: it gives the drone collision-aware movement for blockout demonstrations while keeping movement authority in one place.

Because the pawn root changed, old child Blueprints created before this refactor may contain stale inherited component data. For a clean demo, create a fresh child Blueprint from `ADroneCompanionPawn`.

## Target Detection

Targets are not hardcoded by game class. A blockout actor becomes detectable by adding one of the marker components:

- `UDroneCompanionCollectibleMarkerComponent`;
- `UDroneCompanionEnemyMarkerComponent`.

The sensor performs a timer-based sphere overlap using `ScanRadius`, ignores invalid actors and non-detectable markers, optionally validates line of sight with `ECC_Visibility`, computes a deterministic score, and stores the best target. Enemy targets normally outrank collectibles because the default enemy base score is higher than the collectible base score.

The sensor is also the Observer source for target changes. It exposes `OnBestTargetChanged` and `OnBestTargetLost`, which the brain subscribes to.

## Collectible Inspection

When the best target is a collectible, the brain enters `InspectCollectible`. Follow movement is paused, collectible feedback starts, and the state computes a hover position above the target using `CollectibleHoverHeight`. The movement component moves the drone toward that position. The inspection timer starts only after the drone reaches `InspectAcceptanceRadius`.

If the collectible is lost, destroyed, or replaced by a higher-priority enemy target, the inspection state exits safely. Enemy priority can interrupt inspection directly.

## Enemy Attack

When the best target is an enemy, the brain enters `AttackEnemy`. The drone does not chase enemies. It pauses follow movement, faces the current enemy target, checks range, line of sight, fire cooldown, and fire angle, then asks `UDroneCompanionCombatComponent` to fire.

The combat component starts the hitscan trace from `MuzzlePoint` when available, falls back safely if needed, ignores the drone owner, and optionally calls `UGameplayStatics::ApplyDamage` only when the trace hits the target actor. No custom health system is introduced.

## Configuration and Debugging

`UDroneCompanionConfigDataAsset` keeps the demo tunable without putting gameplay in Blueprint. It includes follow offsets, movement speed, collision radius, scan settings, target scoring, inspection timing, combat values, feedback colors/intensities, optional sounds, and debug booleans.

Debug output is opt-in. Follow, movement, sensing, inspection, combat, and brain transition diagnostics each have a dedicated config toggle. With all debug options disabled, the plugin keeps logging limited to lifecycle messages and useful warnings.

## GoF Patterns Used

### State

The brain uses a small finite state machine implemented with the GoF State pattern. `IDroneCompanionBrainState` defines the state interface, and the private state classes implement the behavior:

- `FDroneCompanionFollowState`;
- `FDroneCompanionInspectCollectibleState`;
- `FDroneCompanionAttackEnemyState`.

Each state has its own `Enter`, `Exit`, `Tick`, and `GetName` methods. This keeps state-specific logic out of a large switch statement.

### Observer

`UDroneCompanionSensorComponent` exposes C++ multicast delegates for meaningful target changes:

- `OnBestTargetChanged`;
- `OnBestTargetLost`.

The brain subscribes to those events. The sensor reports target information, but it does not decide how the drone should react.

### Facade

`UDroneCompanionSensorComponent` also acts as a lightweight Facade for sensing. Other classes do not need to know the details of overlap queries, marker lookup, line-of-sight traces, scoring, debug drawing, or best-target storage. They interact with a compact sensor API and the target-change delegates.

## SOLID and Authority Boundaries

The code favors Single Responsibility over aggressive code reuse. Some fallback constants are local to their owning component because that makes authority clearer: movement fallback values belong near movement, combat fallback values belong near combat, and sensor fallback values belong near sensing.

The design avoids project-specific gameplay classes. Targets are generic `AActor` instances with marker components. Combat uses standard Unreal damage rather than a custom health system. Public APIs remain narrow, while private state classes stay internal to the runtime module.

## Verification Status

Earlier Phase 7 command-line verification recorded a passing `git diff --check` and a passing UE 5.5 build before the movement refactor. After the Phase 6B movement refactor, the build reached UnrealHeaderTool and C++ compilation, but the final link was blocked because `UnrealEditor.exe` was running and holding `UnrealEditor-DroneCompanionRuntime.dll`. That should be rerun with the editor closed before final submission.

Manual Play-In-Editor tests are not claimed as executed by this report. They are documented as verification steps in `Docs/TestPlan.md`.

## Limitations and Future Work

The current movement is collision-aware but not navigational. It can stop or slide against simple blockout walls, but it will not plan a path around obstacles. Future versions could add pathfinding, richer avoidance, animation, particles, target interfaces, multiplayer support, or editor tooling. Those are intentionally outside the current course exercise scope.
