# M2 — Mission Runtime & Mathematics Mission v1

## Purpose

M2 is the first executable pedagogical loop in World Makers. It connects the Building Core to a data-driven mission definition while keeping learning evidence minimized and gameplay rewards compatible with the Trust Economy.

This is a source/runtime prototype. It does not override the native UE 5.8.2 certification blocker in Issue #9 and it does not mark draft pedagogical content as release-approved.

## Canonical mission

`mission.mathematics.measure-and-build-01`

Age band: 7–8. Target span: 300 cm. Tolerance: ±20 cm.

Learning objectives:

- `math.measure.compare-lengths`
- `math.spatial.plan-to-constraint`

Required evidence order:

1. `measurement_used_before_build`
2. `structure_fits_target_span`

A structure that already fits before measurement does not complete the mission. After measuring, the player must place, move, remove, undo/redo, or otherwise adjust the structure so a subsequent geometry observation demonstrates the target constraint.

## Runtime architecture

`UWMMissionRuntimeSubsystem` owns the active mission state for the world. It loads the packaged JSON payload from `Content/WorldMakers/Missions` and parses only stable runtime fields.

`FWMMissionProgressModel` contains deterministic evaluation logic and is intentionally independent of UI and commerce.

The Building Core remains a geometry producer, not a pedagogy engine. It calculates the current X-axis span from placed build-piece dimensions and rotation, then sends only that number to Mission Runtime.

## Evidence minimization

`FWMLearningEvidenceRecord` contains only:

- mission ID;
- evidence event ID;
- objective ID;
- numeric observation;
- deterministic sequence number.

It contains no child name, account ID, email, free-text answer, chat, audio, image, or advertising identifier. Backend transmission is not implemented in M2; a later Learning Evidence service must preserve the same minimization boundary.

## Trust Economy integration

Mission completion references existing M1.10 reward IDs:

- `reward.math.measurement-tool-01`
- `reward.math.spatial-builder-badge-01`

Mission runtime cannot mint money, premium currency, entitlements, purchases, or transferable value. It records deterministic gameplay reward IDs only.

## Packaging

The canonical editable source remains under `content/missions`. A byte-equivalent JSON runtime payload is staged under `game/Content/WorldMakers/Missions` as NonUFS data. CI fails if these two payloads drift.

Long term, approved mission sources may be imported into Unreal Data Assets/Data Tables, but stable mission/objective/evidence/reward IDs remain the contract.

## Current prototype limitation

The measuring tool currently records the known target span when the player presses **Measure**. This proves mission ordering and evidence plumbing but is not the final spatial measurement mechanic. A later phase should introduce visible mission anchors or a world-space measuring tool that derives the measurement from two player-selected points.

The current structure evaluator measures the aggregate X-axis span of all placed player build pieces in the prototype world. A production mission must scope geometry to mission-owned anchors/volumes so unrelated constructions cannot satisfy the objective.

## Exit criteria for M2 source completion

- full mission schema subset validation;
- canonical/runtime mission parity;
- active/completed runtime states;
- measurement-before-build enforcement;
- geometry-derived structure-fit evidence;
- deterministic reward IDs verified against M1.10 catalog;
- no free-text or PII in evidence records;
- child HUD measurement affordance;
- `WorldMakers.Missions.*` automation tests;
- M2 source gate in GitHub Actions.
