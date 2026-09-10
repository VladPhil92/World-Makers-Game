# M3.3 — Science Mission Runtime v1

## Purpose

M3.3 turns the M3.1 semantic biome and M3.2 deliberate observation loop into the first playable science-learning mission contract. The mission runtime now supports more than one evaluator without forcing mathematics-specific fields onto every mission.

The first science mission is:

`mission.science.rainforest-ecosystem-01`

It asks the player to deliberately observe three parts of the Caribbean Rainforest prototype: the ceiba, a bromeliad cluster, and the water edge.

## Multi-evaluator mission runtime

`FWMMissionRuntimeDefinition` now carries an explicit evaluator ID.

Supported evaluators in M3.3:

- `measure-and-build` — existing mathematics flow;
- `observe-ecosystem` — science observation flow.

The shared runtime contract still owns stable mission IDs, prerequisites, rewards, prototype state and journey completion. Evaluator-specific fields are validated semantically in C++ and by repository gates.

`measure-and-build` continues to require `targetSpanCm` and `toleranceCm`.

`observe-ecosystem` deliberately omits those fields and instead requires `observationRequirements`.

## Observation evidence mapping

Each science observation requirement maps three stable IDs:

1. an Environment `observationId`;
2. a Mission evidence event ID;
3. a declared learning-objective ID.

The Caribbean Rainforest mission uses:

- `observation.rainforest.ceiba` → `science.rainforest.ceiba-observed`;
- `observation.rainforest.bromeliad-cluster` → `science.rainforest.bromeliad-observed`;
- `observation.rainforest.water-edge` → `science.rainforest.water-edge-observed`.

Observation order is intentionally flexible. Repeating an observation is allowed as play, but it cannot increase progress or duplicate learning evidence.

## One-way Environment → Mission adapter

`UWMMissionObservationAdapterSubsystem` owns the dependency edge between the two domains.

The Environment layer emits `OnObservationRegistered` using stable IDs. It does not know whether an observation belongs to a mission. The mission-side adapter subscribes to that event, asks the active mission whether the observation is required, and forwards only relevant observations to `RecordObservationEvidence`.

This keeps the Environment module reusable and mission-agnostic while allowing future science, history/culture and narrative missions to consume world events through adapters.

## Progress and completion

For `observe-ecosystem`, progress is deterministic:

`unique required observations recorded / total required observations`

The prototype mission therefore advances through 0/3, 1/3, 2/3 and 3/3. Completion occurs only when all three required observations have been recorded.

Completion uses the same `FWMMissionJourneyModel` introduced in M2.3. There is no parallel science progression currency or alternate reward ledger.

## Trust Economy reward

The science mission grants:

`reward.science.rainforest-observer-badge-01`

It is deterministic, non-purchasable, non-transferable, non-convertible to money, non-expiring and requires no streak. There is no XP, daily reward, premium currency, loot box, paid shortcut or loss-aversion mechanic.

## Child-facing UX

The existing `Observe` action remains the explicit interaction affordance. When the active mission uses `observe-ecosystem`, the mission HUD shows observation progress rather than mathematics measurement instructions.

The player can explore the three semantic zones in any order. Deliberate observation, not passive proximity, generates the evidence used by the science mission.

## Privacy boundary

M3.3 stores and processes stable IDs plus the existing minimized numeric/sequence evidence fields. It adds no child name, email, free-text response, advertising identifier, commerce payload or raw behavioral transcript.

The science mission remains `prototypeOnly=true` while pedagogy, safety and cultural review are draft.

## Source certification

Repository validators and Unreal Automation tests can establish source-level contract integrity. They cannot prove native runtime behavior on the locked engine/device baseline.

Issue #9 remains the independent blocker for:

- Windows x64 self-hosted runner;
- exact Unreal Engine 5.8.2;
- authored `WM_PrototypeCertification.umap`;
- native `WorldMakers.*` automation execution;
- manual interaction and device evidence.

A skipped native Unreal job is not runtime certification.

## Exit condition

M3.3 is source-complete when mathematics missions still pass, the rainforest science mission parses without mathematics span fields, three real deliberate biome observations complete the mission idempotently, the mission-side adapter owns the Environment-to-Mission bridge, the Trust Economy reward is valid, and all repository CI gates pass.
