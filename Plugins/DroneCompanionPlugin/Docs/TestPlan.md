# Drone Companion Test Plan

This document is a practical verification checklist for the `DroneCompanion` Runtime plugin in Unreal Engine 5.5. It separates command-line checks from manual Play-In-Editor tests. Unless a result is explicitly marked as executed, the manual tests below are instructions for final demo verification, not claimed test evidence.

## Verification Status

| Area | Status |
| --- | --- |
| Command-line checks | To run before final submission. |
| C++ automation tests | Added as `DroneCompanion.*`; run from command line before final submission. |
| Manual Play-In-Editor tests | Not executed by this document. |
| Phase 6B build after movement refactor | Rerun with Unreal Editor closed if the plugin DLL is locked. |

The known Phase 6B build issue was environmental: Unreal Editor was running and holding `UnrealEditor-DroneCompanionRuntime.dll` during final link. That does not prove a code failure, but the build should be rerun cleanly before submission.

## Command-Line Checks

Run these from the project root:

```powershell
git diff --check
```

Expected result: no whitespace or patch-format errors.

Build command:

```powershell
& "G:\Program Files\Epic Games\UE_5.5\Engine\Build\BatchFiles\Build.bat" DroneCompanionEditor Win64 Development -Project="f:\DBGA\DroneCompanion\DroneCompanion.uproject" -WaitMutex -NoHotReloadFromIDE
```

Expected result: UnrealHeaderTool processes the plugin headers, `DroneCompanionRuntime` compiles, and the command exits successfully. Close Unreal Editor first so the plugin DLL can be linked.

Automation test command:

```powershell
& "G:\Program Files\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "f:\DBGA\DroneCompanion\DroneCompanion.uproject" -ExecCmds="Automation RunTests DroneCompanion; Quit" -TestExit="Automation Test Queue Empty" -unattended -nop4 -NullRHI -NoSound -NoSplash -log
```

Expected result: all tests under `DroneCompanion.*` complete successfully. These tests are asset-free C++ automation checks and should be run with Unreal Editor closed, for the same DLL-lock reason as the build.

Current strategic automation coverage:

| Test Group | What It Protects |
| --- | --- |
| `DroneCompanion.Targets.*` | Marker defaults and target classification metadata. |
| `DroneCompanion.Sensor.*` | Enemy priority, same-type distance scoring, marker priority bonuses, disabled/destroyed target filtering, and best-target changed/lost events. |
| `DroneCompanion.Follow.*` | Follow pause/resume behavior and invalid move-speed fallback. |
| `DroneCompanion.Movement.*` | Missing `UpdatedComponent` safety, reachable movement, and swept collision against a blocking wall. |
| `DroneCompanion.Pawn.*` | Invalid collision-radius fallback on the pawn root sphere. |
| `DroneCompanion.Combat.*` | Null/out-of-range target safety, invalid range fallback, cooldown, and blocked line traces. |
| `DroneCompanion.Brain.*` | Follow-to-inspection transition and enemy interruption of collectible inspection. |

Static review checklist:

| Check | Expected Result | Status |
| --- | --- | --- |
| `DroneCompanionRuntime.Build.cs` dependencies | Only `Core`, `CoreUObject`, and `Engine`. | Not executed by this document. |
| Movement architecture review | `ADroneCompanionPawn` uses `CollisionComponent` as root and movement runs through `UDroneCompanionMovementComponent`. | Not executed by this document. |
| Debug gating review | Debug drawing and noisy logs are controlled by config booleans. | Not executed by this document. |

## Editor Setup Checklist

Manual status: not executed by this document.

1. Enable the `Drone Companion` plugin.
2. Compile the project with Unreal Engine 5.5.
3. Create or open a `UDroneCompanionConfigDataAsset`.
4. Create a fresh child Blueprint from `ADroneCompanionPawn` if the old child Blueprint was made before the collision-root refactor.
5. Confirm the inherited root component is `CollisionComponent`.
6. Assign the DataAsset to the pawn's `Config` property.
7. Place a PlayerStart or otherwise ensure Player 0 has a valid pawn.
8. Add `UDroneCompanionCollectibleMarkerComponent` to blockout collectible actors.
9. Add `UDroneCompanionEnemyMarkerComponent` to blockout enemy actors.
10. Make enemy actors block the `Visibility` channel if hitscan hits should register.

Recommended starting config for movement tests:

| Property | Suggested Value |
| --- | --- |
| `DroneCollisionRadius` | `40.0` |
| `MoveSpeed` | `600.0` |
| `FollowDistance` | `300.0` |
| `FollowHeight` | `150.0` |
| `AcceptanceRadius` | `25.0` |
| `bAutoAcquirePlayerOnBeginPlay` | `true` |

## Raw C++ Pawn Movement Test

Manual status: not executed by this document.

Steps:

1. Place `ADroneCompanionPawn` directly in the level, not a child Blueprint.
2. Assign the config DataAsset.
3. Place the pawn above the floor so the sphere root is not initially overlapping geometry.
4. Press Play and move Player 0.

Expected result: the drone follows the player. If the raw C++ pawn moves but a child Blueprint does not, the child Blueprint is probably stale and should be recreated.

## Recreated Blueprint Test

Manual status: not executed by this document.

Steps:

1. Create a new child Blueprint from `ADroneCompanionPawn`.
2. Confirm the root is `CollisionComponent`.
3. Assign a simple mesh to `DroneMesh`.
4. Assign the config DataAsset.
5. Leave the Event Graph empty.
6. Place the Blueprint in the level and press Play.

Expected result: behavior matches the raw C++ pawn. The Blueprint only supplies editor-facing asset wiring.

## Player Follow Test

Manual status: not executed by this document.

Steps:

1. Keep `bAutoAcquirePlayerOnBeginPlay` enabled.
2. Ensure Player 0 exists.
3. Press Play and move the player.

Expected result: the drone follows behind and above Player 0 using `FollowDistance`, `FollowHeight`, `MoveSpeed`, and `AcceptanceRadius`.

## Wall-Behind-Player Collision Test

Manual status: not executed by this document.

Steps:

1. Place a simple blockout wall behind the player.
2. Put the drone on the opposite side of the wall from its desired follow position.
3. Enable `bEnableMovementDebug` and optionally `bEnableFollowDebug`.
4. Press Play and move the player near the wall.

Expected result: the desired follow position may be behind the wall, but the drone should not pass through the wall. It should stop or slide according to `SafeMoveUpdatedComponent` and `SlideAlongSurface`.

## Target Sensing Test

Manual status: not executed by this document.

Steps:

1. Place a marked collectible and a marked enemy within `ScanRadius`.
2. Enable `bEnableSensorDebug`.
3. Press Play.

Expected result: the sensor debug drawing shows the scan area, candidate lines, and best target. Enemy targets normally outrank collectibles because `EnemyBaseScore` is higher than `CollectibleBaseScore`.

## Collectible Inspection Test

Manual status: not executed by this document.

Steps:

1. Place a collectible marker inside sensor range with no higher-priority enemy nearby.
2. Press Play.

Expected result: the drone pauses follow movement, moves above the collectible using `CollectibleHoverHeight`, plays collectible feedback, waits for `InspectDuration` after reaching `InspectAcceptanceRadius`, then returns to follow.

## Collectible Inspection Near Wall Test

Manual status: not executed by this document.

Steps:

1. Place a collectible near a blockout wall.
2. Enable `bEnableMovementDebug` and `bEnableInspectionDebug`.
3. Press Play.

Expected result: the inspection state computes the hover target, but translation remains collision-aware through `UDroneCompanionMovementComponent`. The drone should not tunnel through the wall.

## Enemy Attack Test

Manual status: not executed by this document.

Steps:

1. Place an enemy marker inside `ScanRadius` and `AttackRange`.
2. Ensure the enemy blocks `Visibility`.
3. Optionally enable `bEnableCombatDebug`.
4. Press Play.

Expected result: the drone enters combat feedback, faces the enemy, fires only when within `MaxFireAngleDegrees`, respects `FireCooldown`, traces from `MuzzlePoint` when available, and returns to follow when the enemy is invalid, lost, blocked, or out of range.

## Enemy Priority Over Collectible Test

Manual status: not executed by this document.

Steps:

1. Start collectible inspection.
2. Move or spawn an enemy marker so it becomes the best target.

Expected result: collectible inspection aborts and the drone transitions to enemy attack. The drone does not keep inspecting while an enemy is prioritized.

## Missing Config Fallback Test

Manual status: not executed by this document.

Steps:

1. Clear the pawn's `Config` property.
2. Press Play.

Expected result: the drone does not crash. Components use local fallback values. Missing-config warnings may appear.

## Invalid or Destroyed Target Safety Test

Manual status: not executed by this document.

Steps:

1. Start collectible inspection or enemy attack.
2. Destroy or remove the current target actor during Play.

Expected result: the drone handles the invalid target safely and returns to follow behavior without crashing.

## Debug Visualization Test

Manual status: not executed by this document.

Steps:

1. Enable one debug option at a time:
   - `bEnableFollowDebug`
   - `bEnableMovementDebug`
   - `bEnableSensorDebug`
   - `bEnableInspectionDebug`
   - `bEnableCombatDebug`
   - `bEnableBrainDebug`
2. Press Play and observe the viewport and Output Log.

Expected result: only the selected debug category appears. With all debug booleans disabled, the demo remains quiet except for limited lifecycle or warning messages.
