# M3.5 — Creative Ecological Interventions & Unlocks

## Purpose

M3.5 connects the Building Core to the M3.4 reactive ecosystem so building becomes a way to care for the world, not a parallel sandbox disconnected from learning. The phase introduces deterministic spatial build interventions and persistent creative unlocks while keeping the Building and Environment domains directionally separated.

The core loop is:

`understand problem → build solution → evaluate world geometry → change ecosystem → unlock new creative possibility`

There are no coins, premium currency, timers, streaks, energy systems, randomized rewards, paid recovery, XP grinding, or FOMO gates in this loop.

## Architectural boundary

Building does **not** import Environment.

`UWMBuildWorldStateSubsystem` publishes a neutral read projection containing only:

- placed `PieceId`;
- world location;
- yaw;
- monotonically increasing session revision.

`UWMEcologicalBuildSubsystem` lives on the Environment side and consumes that projection. It owns the ecological interpretation of construction and calls `UWMEnvironmentStateSubsystem::ApplyTrustedEffect` only after a data-driven intervention has been satisfied.

This dependency direction keeps the Building Core reusable for mathematics, free construction, future biomes and multiplayer without embedding rainforest rules into placement code.

## Data-driven intervention profile

Canonical source:

`content/biomes/caribbean-rainforest/build-interventions.json`

Packaged runtime copy:

`game/Content/WorldMakers/Biomes/biome.caribbean-rainforest.build-interventions.json`

The two payloads must remain byte-equivalent.

Each intervention contains:

- stable intervention ID;
- bounded world-space anchor and radius;
- prerequisite intervention IDs;
- required build piece IDs and counts;
- bounded ecological effect delta;
- deterministic creative reward ID.

A global piece count is never enough. A required piece must be placed inside the intervention area to contribute.

## Prototype creative sequence

### 1. Soil buffer

`intervention.ecosystem.soil-buffer`

The player constructs two `prototype.wall` pieces inside the soil-buffer intervention area. Completion applies a bounded soil/vegetation improvement and grants:

`reward.eco.leaf-roof-unlock`

which unlocks:

`eco.leaf-roof`

### 2. Shade shelter

`intervention.ecosystem.shade-shelter`

This intervention is unavailable until the soil buffer has completed. It requires two `prototype.pillar` pieces plus one newly unlocked `eco.leaf-roof` inside its area. Completion improves shade/vegetation and grants:

`reward.eco.rainforest-planter-unlock`

which unlocks:

`eco.rainforest-planter`

### 3. Habitat garden

`intervention.ecosystem.habitat-garden`

This intervention depends on the shade shelter. It requires two `eco.rainforest-planter` pieces plus one `prototype.floor` inside its area. Completion improves vegetation/soil and grants:

`reward.eco.bamboo-bridge-unlock`

which unlocks:

`eco.bamboo-bridge`

The bamboo bridge is therefore a creative consequence of demonstrated world care, not a purchased advantage.

## Trusted environmental effects

M3.4 remains the sole authority over environmental values. M3.5 does not write `VegetationHealth`, `WaterFlow`, `SoilProtection`, `ShadeCoverage`, or `HabitatQuality` directly.

The ecological evaluator submits a stable intervention ID plus a bounded `FWMEnvironmentStateDelta` through the native-only trusted-domain entrypoint. The state model clamps all values to `[0,1]`, derives habitat quality, and allows each intervention effect at most once.

Observation remains separate: an `ObservationId` never mutates environmental state.

## Creative unlock persistence

`UWMBuildUnlockSubsystem` is a GameInstance subsystem with a dedicated prototype save slot:

`WM_CreativeUnlocks_Prototype`

It persists only stable reward IDs. It stores no child name, email, free text, analytics identifier, commerce record, engagement counter, streak, or time-gated state.

The build catalog remains authoritative for piece definitions. `FWMBuildPieceSpec::RequiredRewardId` optionally gates a creative piece.

Base pieces have no reward requirement. Ecological pieces require their corresponding deterministic reward.

## Save/load bypass protection

The unlock gate is enforced by `UWMBuildingComponent::ResolvePieceSpec` rather than by UI alone. Therefore it applies to:

- direct selection;
- piece cycling;
- preview/spawn;
- move/undo/redo restoration;
- world save validation;
- world load validation.

A world-save record referencing a locked creative piece is rejected until the required reward exists. A modified save file therefore cannot manufacture access to unrewarded content.

Once a reward has legitimately been granted, loading worlds that already contain that piece is allowed.

## Session intervention state vs. persistent creative ownership

Intervention completion is session/world-state derived. Creative ownership is persistent.

This distinction is deliberate: a fresh world may begin with previously earned creative pieces available, but the new world's ecosystem state still has to be improved through its own valid interventions. Persistent unlock ownership does not silently mutate a fresh biome.

## Trust Economy boundary

The new rewards use the existing `creative_unlock` reward type and retain all locked Trust Economy invariants:

- deterministic;
- not purchasable;
- not transferable;
- not convertible to money;
- non-expiring;
- no streak requirement.

M3.5 adds creative possibility rather than numerical power or economic status. It does not introduce a parallel wallet, virtual currency, loot box, random drop, scarcity timer, daily reward or pay-to-skip route.

## Read model for M3.6

`FWMEcologicalBuildInterventionReadModel` exposes only stable, child-safe progress data:

- intervention ID;
- completion state;
- prerequisite readiness;
- satisfied requirement count;
- total requirement count;
- reward ID.

This is intended for M3.6 Child Journey UX, where technical mission cycling and system IDs can be presented as understandable child-facing adventures.

## Automation coverage

M3.5 adds source/native automation contracts for:

- parsing the intervention sequence;
- spatial and prerequisite-aware evaluation;
- idempotent reward-gated creative unlocks;
- bounded one-shot trusted ecosystem effects.

The repository validator additionally enforces canonical/package parity, Trust Economy reward invariants, the Building → neutral projection → Environment dependency direction, and save/load unlock enforcement.

## Certification boundary

M3.5 is source-complete only after its repository and Unreal source-validation gates pass.

Issue #9 remains the independent native certification blocker. A locked Windows x64 runner with the exact Unreal Engine 5.8.2 baseline, authored certification map, native automation execution and retained evidence are still required before claiming runtime/device certification.

A skipped native job is not certification.

## Next phase

M3.6 — Child Journey UX should turn the current technical mission/intervention/read models into a coherent child-facing navigation layer such as **My Adventures**, showing exploration, science missions, ecological build goals and earned creative possibilities without XP pressure, streaks or manipulative retention mechanics.
