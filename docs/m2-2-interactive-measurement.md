# M2.2 — Interactive Measurement Tool & Multi-Mission Orchestration

## Goal

M2.1 established correct world-space geometry, but the child-facing Measure action still resolved a pre-authored pair of anchors and the runtime bootstrapped one fixed JSON path. M2.2 turns measurement into an actual two-point interaction and turns mission loading into a small data-driven catalog.

## Interactive measurement

`UWMMissionMeasurementComponent` belongs to the player and is deliberately separate from Building.

The interaction is:

1. aim at a surface inside the active mission build zone;
2. tap/click **Measure** to capture point A;
3. aim at a second surface inside the same zone;
4. tap/click **Measure** again to capture point B;
5. compute `FVector::Dist(A, B)` in Unreal centimeters;
6. record only the numeric distance through Mission Runtime.

The deterministic `FWMMissionMeasurementModel` has four states:

- `Idle`;
- `AwaitingFirstPoint`;
- `AwaitingSecondPoint`;
- `Complete`.

Outside-zone points and second points below the minimum separation are rejected without advancing state.

## Visual feedback

`AWMMissionGeometryActor` now owns non-colliding runtime-only visualization components:

- interactive start marker;
- interactive end marker;
- a cube-mesh segment scaled to the measured distance.

The segment is visual feedback only. It has no collision and does not enter building, physics, save data, rewards or evidence.

`Reset measure` clears the visual and local measurement model.

## Child UI and input

The bottom HUD exposes:

- **Measure** — captures the current view hit as A or B;
- **Reset measure** — clears A/B and the segment;
- **Next mission** — resets transient measurement state and cycles the runtime catalog.

Keyboard/gamepad prototype mappings:

- `F` / D-pad Up — Measure;
- `X` / D-pad Down — Reset measurement;
- `N` / D-pad Left — Next mission.

Touch users use the same large HUD buttons introduced by the tablet UX phase.

## Mission catalog

`UWMMissionRuntimeSubsystem::ReloadMissionCatalog()` scans staged `WorldMakers/Missions/*.json` definitions, parses them through the existing runtime definition contract, rejects duplicate `MissionId` values and sorts IDs deterministically.

Public orchestration methods:

- `ReloadMissionCatalog()`;
- `ActivateMission(MissionId)`;
- `CycleMission(Direction)`;
- `GetAvailableMissionIds()`;
- `GetActiveMissionId()`;
- `GetMissionCount()`.

The old `ReloadAndActivatePrototypeMission()` remains as a compatibility bootstrap. It now reloads the catalog and selects the original mission by stable ID instead of reading a fixed file path.

## Two prototype missions

The packaged catalog now proves multi-mission behavior with:

- `mission.mathematics.measure-and-build-01` — target 300 cm, tolerance 20 cm;
- `mission.mathematics.measure-and-build-02` — target 200 cm, tolerance 15 cm.

Both remain `prototypeOnly=true` because pedagogy and safety review are still draft. The second mission reuses existing deterministic reward IDs; no commercial or child-identity state is introduced.

When a mission is activated, the mission geometry updates its reference anchor span to the mission target and clears interactive visuals.

## Architecture boundary

The child-facing path is now:

`Player/HUD -> MissionMeasurementComponent -> world trace -> mission-zone validation -> numeric distance -> MissionRuntime`

Building still reports only structure geometry. The legacy M2.1 `RecordActiveGeometryMeasurement()` helper remains available for compatibility and source tests, but the HUD no longer calls it.

## Tests

M2.2 adds:

- `WorldMakers.Missions.Measurement.CapturesTwoValidPoints`;
- `WorldMakers.Missions.Catalog.MultipleDefinitions`.

Existing M2.1 geometry tests remain in force.

## Certification boundary

This is a source-complete phase only. Issue #9 still blocks claims for:

- UE 5.8.2 native compilation;
- execution of the Unreal Automation suite on the locked self-hosted runner;
- authored `.umap` runtime evidence;
- manual tablet/runtime certification.
