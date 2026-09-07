# M2.1 — Spatial Measurement & Mission Geometry

## Goal

Replace the M2 prototype shortcut that recorded the configured target as if it had been measured. M2.1 derives measurement from actual world-space anchors and evaluates only construction located inside the active mission zone.

## Runtime geometry

`AWMMissionGeometryActor` owns:

- a non-colliding `UBoxComponent` build/evaluation zone;
- `MeasureStart` and `MeasureEnd` anchor components;
- lightweight visible sphere markers for the prototype;
- a default 300 cm anchor separation matching the current mathematics mission.

The actor registers itself with `UWMMissionRuntimeSubsystem` during `BeginPlay` and unregisters on `EndPlay`.

## Measurement contract

The Measure action now follows:

`Measure button -> BuildingComponent -> MissionRuntime -> active geometry -> world-space anchor distance -> learning evidence`

The measurement is computed through `UWMMissionGeometryLibrary::MeasureWorldDistanceCm`. The child HUD displays the resulting centimeters and meters, but learning evidence remains numeric/stable-ID only; no free text or identity data is added.

The former shortcut `RecordMeasurement(GetTargetSpanCm())` is forbidden by the M2.1 CI gate.

## Mission-scoped structure evaluation

Building still emits a neutral geometry observation. It does not decide whether the mathematics objective was satisfied.

For each placed piece, Building supplies a geometry sample containing:

- world center;
- catalog dimensions in cm;
- yaw.

`CalculateScopedSpanAlongZoneX` then:

1. transforms piece centers into mission-local coordinates;
2. excludes any piece whose center lies outside the build zone;
3. projects rotated X/Y dimensions onto the mission-local X axis;
4. returns the min-to-max span of only the included pieces.

This means the mission actor may be translated or rotated without changing the mathematical meaning of its local build axis.

## Prototype world integration

`AWMGameMode` source-spawns the mission geometry while `bSpawnPrototypeMissionGeometry=True`. This keeps M2.1 testable in the existing source-only prototype and does not pretend that the missing authored certification `.umap` has been created.

## Tests

Automation coverage:

- `WorldMakers.Missions.Geometry.WorldDistance`
- `WorldMakers.Missions.Geometry.ZoneContainment`
- `WorldMakers.Missions.Geometry.ScopedStructureSpan`

The scoped-span test also verifies that a large piece outside the mission zone cannot affect the result and that a 90-degree rotated wall contributes its projected 25 cm width along mission-local X.

## Product boundary

The mathematics mission remains `prototypeOnly=true` while pedagogy and safety review are draft. M2.1 improves geometric validity; it does not confer pedagogical approval or native UE certification.

Issue #9 remains the independent blocker for UE 5.8.2 compile, Automation execution, authored map and manual runtime evidence.
