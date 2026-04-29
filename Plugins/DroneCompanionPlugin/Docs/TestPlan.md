# Drone Companion Test Plan

This document separates checks that can be run from the command line from manual Unreal Editor playtests. Manual playtests are not executed by this document; they are a checklist for final demo verification.

## Phase 7 Command-Line Status

Executed on April 29, 2026 from `f:\DBGA\DroneCompanion`.

| Check | Result |
| --- | --- |
| `git diff --check` | Passed. |
| UE 5.5 build command below | Passed. |
| Forbidden-system scan over source and plugin descriptor | Passed. |
| Manual Unreal Editor playtests | Not executed in this pass. |

## Build Verification

Command:

```powershell
& "G:\Program Files\Epic Games\UE_5.5\Engine\Build\BatchFiles\Build.bat" DroneCompanionEditor Win64 Development -Project="f:\DBGA\DroneCompanion\DroneCompanion.uproject" -WaitMutex -NoHotReloadFromIDE
```

Expected result: UnrealHeaderTool processes the plugin headers successfully, `DroneCompanionRuntime` compiles, and the command exits successfully.

## Static Checks

| Check | Expected Result |
| --- | --- |
| Run `git diff --check`. | No whitespace or patch-format errors. |
| Inspect `DroneCompanionRuntime.Build.cs`. | Dependencies are only `Core`, `CoreUObject`, and `Engine`. |
| Scan source/descriptors for forbidden systems. | No AI Perception, Behavior Tree, StateTree, EQS, NavMesh, replication, projectiles, custom health, enemy AI, inventory, Editor module, UMG, Slate, InputCore, or Blueprint gameplay logic. |
| Inspect folder structure. | Runtime files are organized under `Public/Core`, `Public/Components`, `Public/Targets`, `Public/Module`, and matching private folders. |
| Inspect private brain state files. | Follow, InspectCollectible, and AttackEnemy remain private non-UObject state classes. |
| Inspect debug code. | Debug drawing/logging is gated by config booleans where it would otherwise be noisy. |

## Editor Setup Checklist

Manual status: not executed by this document.

1. Enable the `Drone Companion` plugin.
2. Compile the project with Unreal Engine 5.5.
3. Create a `DroneCompanionConfigDataAsset`.
4. Place `ADroneCompanionPawn` or an optional child Blueprint in the level.
5. Assign the DataAsset to the pawn's `Config` property.
6. Assign optional mesh/material/audio assets through the pawn or child Blueprint.
7. Add `UDroneCompanionCollectibleMarkerComponent` to at least one blockout collectible actor.
8. Add `UDroneCompanionEnemyMarkerComponent` to at least one blockout enemy actor.
9. Ensure enemy actors block the `Visibility` channel if hitscan hits should register.

## Follow Test

Manual status: not executed by this document.

Steps:

1. Leave `bAutoAcquirePlayerOnBeginPlay` enabled.
2. Press Play with a Player 0 pawn present.
3. Move the player.

Expected result: the drone follows behind and above Player 0 using `FollowDistance`, `FollowHeight`, `MoveSpeed`, and `AcceptanceRadius`.

## Sensing Test

Manual status: not executed by this document.

Steps:

1. Place marked collectible and enemy actors within `ScanRadius`.
2. Enable `bEnableSensorDebug`.
3. Press Play.

Expected result: the scan sphere, candidate lines, and best-target marker are drawn. Enemies normally outrank collectibles because `EnemyBaseScore` is higher than `CollectibleBaseScore`.

## Collectible Inspection Test

Manual status: not executed by this document.

Steps:

1. Place a collectible marker inside sensor range with no higher-priority enemy.
2. Press Play.

Expected result: the drone pauses follow movement, flies above the collectible using `CollectibleHoverHeight`, uses collectible feedback, waits for `InspectDuration` after reaching `InspectAcceptanceRadius`, then returns to follow.

## Enemy Attack Test

Manual status: not executed by this document.

Steps:

1. Place an enemy marker inside `ScanRadius` and `AttackRange`.
2. Make sure the enemy blocks the `Visibility` trace channel.
3. Optionally enable `bEnableCombatDebug`.
4. Press Play.

Expected result: the drone enters combat feedback, faces the enemy, fires only when within `MaxFireAngleDegrees`, respects `FireCooldown`, uses hitscan from `MuzzlePoint` when available, and returns to follow when the enemy is invalid, lost, blocked, or out of range.

## Enemy Priority Over Collectible Test

Manual status: not executed by this document.

Steps:

1. Start collectible inspection.
2. Introduce or move an enemy so it becomes the best target.

Expected result: collectible inspection aborts and the drone transitions to enemy attack. No collectible behavior continues while the enemy is prioritized.

## Missing Config Fallback Test

Manual status: not executed by this document.

Steps:

1. Clear the pawn's `Config` property.
2. Press Play.

Expected result: the drone does not crash. Components use local fallback values for movement, scanning, inspection, combat, and feedback. Missing-config warnings may appear.

## Invalid or Destroyed Target Safety Test

Manual status: not executed by this document.

Steps:

1. Start collectible inspection or enemy attack.
2. Destroy or remove the current target actor during Play.

Expected result: the drone handles the invalid target safely and returns to follow behavior without crashing.

## Debug Visualization Test

Manual status: not executed by this document.

Steps:

1. Enable each debug boolean one at a time:
   - `bEnableFollowDebug`
   - `bEnableSensorDebug`
   - `bEnableInspectionDebug`
   - `bEnableCombatDebug`
   - `bEnableBrainDebug`
2. Press Play and observe output.

Expected result: debug output appears only for the enabled category. With all debug booleans disabled, logs and debug drawing remain quiet except limited lifecycle/warning messages.
