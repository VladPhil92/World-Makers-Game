# M3.1 — Data-driven Caribbean Rainforest Biome Runtime & Exploration Core

## Goal

Promote the M1.8 Caribbean Rainforest visual proof into a semantic, data-driven world layer that future science/narrative missions can observe without coupling pedagogical logic to the environment renderer.

## Runtime content boundary

`content/biomes/caribbean-rainforest/runtime.json` is the canonical semantic definition. An exact copy is packaged under `game/Content/WorldMakers/Biomes/biome.caribbean-rainforest.json` and staged as NonUFS content so Unreal can discover definitions at runtime.

The existing `biome.yaml` remains the review/provenance/art-budget record. It is not parsed by gameplay code.

The runtime JSON contains only stable IDs, numeric geometry, booleans and controlled category strings. It contains no child PII, names, email addresses, free-text responses, analytics identifiers or commercial transaction data.

## Semantic model

`FWMBiomeRuntimeDefinition` owns:

- stable biome ID and schema version;
- deterministic biome origin;
- semantic zones with center/extents, build permission and optional Mission IDs;
- points of interest (POIs) with stable point/discovery IDs, category, location and discovery radius.

M3.1 Caribbean Rainforest defines four semantic zones:

- build clearing;
- canopy trail;
- understory west;
- water edge.

It also defines three prototype POIs: a ceiba, a bromeliad cluster and the water edge.

## Biome runtime subsystem

`UWMBiomeRuntimeSubsystem` is a `UWorldSubsystem`. It discovers packaged biome definitions, rejects duplicate IDs through catalog construction, activates the prototype Caribbean Rainforest, resolves the current zone from world-space location and exposes nearby POIs.

`ObserveLocation` updates the active zone and registers nearby discoveries. Zone changes and discoveries are exposed through delegates so later systems can observe them without the biome layer importing or depending on the Mission runtime.

## Exploration component

`UWMExplorationComponent` is owned by `AWMPlayerCharacter`. It observes the local player's location at a bounded interval (default 0.25 seconds) and feeds that position to the biome subsystem.

This is deliberately an observation layer: it does not grant mission completion, currency or commerce value. Future mission evaluators may subscribe to stable exploration events independently.

## Discovery state

M3.1 discovery progress is session-local and idempotent. `FWMExplorationProgressModel` stores only stable discovery IDs in memory. Re-entering the radius of an already discovered POI does not emit the discovery again.

Persistence across sessions is intentionally deferred until profile-scoped storage can be aligned with the privacy-minimized journey model introduced in M2.3.

## Tests and CI

Unreal Automation coverage includes:

- `WorldMakers.Exploration.Definition.ParsesRuntimeContract`;
- `WorldMakers.Exploration.Geometry.ZoneContainment`;
- `WorldMakers.Exploration.Discovery.IdempotentStableIds`.

`scripts/validate-m3-1-biome-runtime.py` checks canonical/package parity, semantic zone and POI integrity, mission references, privacy boundaries, player integration, test presence and CI wiring.

## Boundaries

- M3.1 is semantic/runtime source infrastructure, not final environment art.
- Cultural motifs and claims remain subject to the provenance/community review recorded in the biome source record.
- The visual prototype actor remains a renderer/proxy and is not the source of semantic zone or POI truth.
- Discovery progress is session-local in this phase.
- M3.1 does not couple exploration directly to any specific mission evaluator.
- Issue #9 remains the independent blocker for native UE 5.8.2 compilation, authored `.umap`, full Unreal Automation execution and manual/device evidence.
