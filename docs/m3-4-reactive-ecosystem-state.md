# M3.4 — Reactive Ecosystem State

## Purpose

M3.4 turns the Caribbean Rainforest vertical slice into a deterministic reactive ecosystem. M3.1 made the biome semantic, M3.2 made observation deliberate, and M3.3 connected observations to science-learning evidence. M3.4 adds a separate causal layer: explicit care actions change the environmental state and produce visible reaction bands without making the Environment domain depend on any mission implementation.

The core rule is intentionally strict:

**Observation is evidence; care action is causation.**

Simply observing a ceiba, bromeliad cluster or water edge never changes ecological state.

## Data-driven ecosystem profile

The canonical profile is:

`content/biomes/caribbean-rainforest/ecosystem.json`

The packaged runtime copy is:

`game/Content/WorldMakers/Biomes/biome.caribbean-rainforest.ecosystem.json`

They must remain byte-equivalent.

The profile contains only stable IDs, normalized environmental values, interaction coordinates and deterministic transition rules. It contains no child PII, free text, analytics identifiers, commerce state or engagement counters.

## Primary state dimensions

`FWMEnvironmentStateSnapshot` exposes four primary normalized values in `[0,1]`:

- `VegetationHealth`
- `WaterFlow`
- `SoilProtection`
- `ShadeCoverage`

`HabitatQuality` is **derived**, not authored. M3.4 computes it as the arithmetic mean of those four primary dimensions and clamps the result to `[0,1]`.

No UI, interaction actor or mission can write habitat quality directly.

## Baseline

The Caribbean Rainforest prototype begins with:

- vegetation health: `0.30`
- water flow: `0.30`
- soil protection: `0.25`
- shade coverage: `0.35`
- derived habitat quality: `0.30`

This resolves to:

`reaction.ecosystem.stressed`

The baseline is deliberately recoverable rather than catastrophic. The child is invited to help, not punished for an inherited loss state.

## Explicit care actions

M3.4 defines three care actions:

1. `action.ecosystem.protect-soil`
2. `action.ecosystem.restore-shade`
3. `action.ecosystem.clear-water-path`

Each action has a dedicated semantic target, prompt key, world-space prototype location, interaction radius, focus radius and deterministic state delta.

Each prototype action has `maxApplications: 1`. Repeated input therefore cannot farm environmental improvement, inflate progress or create unbounded values.

After all three accepted actions, the deterministic state is:

- vegetation health: `0.55`
- water flow: `0.75`
- soil protection: `0.70`
- shade coverage: `0.80`
- habitat quality: `0.70`

## Reaction bands

The profile maps habitat quality to stable reaction IDs:

- `reaction.ecosystem.stressed` — up to `0.34`
- `reaction.ecosystem.recovering` — up to `0.64`
- `reaction.ecosystem.thriving` — up to `1.00`

`UWMEnvironmentStateSubsystem` emits `OnEnvironmentStateChanged` after every accepted care action and `OnEnvironmentReaction` when an action crosses a reaction-band boundary.

These stable reaction IDs are the visual/audio contract for later authored work: foliage density/material response, water presentation, ambient audio, particles, wildlife proxies and other level-art reactions can subscribe without changing ecological rules.

## Reusable interaction contract

`IWMInteractable` now distinguishes:

- interaction mode;
- observation ID;
- action ID.

Existing environmental observation actors return an observation ID and no action ID.

`AWMEnvironmentActionActor` returns a care action ID and no observation ID.

`UWMInteractionComponent` considers both actor families under the same camera-focus, distance and anti-spam rules. The read state exposes `FocusedInteractionMode`, `FocusedObservationId` and `FocusedActionId`, so UI can represent intent without guessing from text.

## Visible prototype feedback

The existing child HUD now exposes the current ecosystem reaction state:

- stressed — the habitat needs care;
- recovering — the player's actions are helping;
- thriving — the habitat has improved.

The interaction button remains the M3.2 `ObserveWorldButton` input surface for compatibility, but its label changes contextually:

- `Observe` for observation targets;
- `Help` for care-action targets.

This is a source-level visible consequence. Authored biome VFX/material/foliage reactions remain a later vertical-slice art task.

## Architecture boundary

The Environment domain remains **mission-agnostic**. `UWMEnvironmentStateSubsystem`, action actors and interaction components import no Mission runtime classes.

A future mission may consume environment state or reaction events through a mission-side adapter, just as M3.3 consumes biome observations. The dependency direction must remain one-way from Mission adapters toward Environment APIs, never Environment toward Mission.

## Persistence and privacy

M3.4 state is session-local prototype state. It contains normalized numeric values, action application counts and stable IDs only.

It does not persist:

- child names;
- email addresses;
- authored/free-text responses;
- raw interaction histories;
- ad identifiers;
- purchases or receipts;
- streaks or engagement-pressure metrics.

Persistence, if later required for a child journey, must be separately reviewed against the privacy boundary before being added.

## Trust and child-safety boundary

Environmental recovery is not monetized. There is no paid skip, premium environmental boost, loot box, energy timer, streak loss, daily-reset pressure or punishment for stopping play.

The child may stop at any environmental state without losing prior mission completion, creative work or account access.

## Certification boundary

M3.4 is source-complete only after its repository/Unreal source gates pass. **Issue #9 remains the independent native certification blocker** for the locked Windows x64 Unreal Engine 5.8.2 runner, authored certification map, automation evidence and manual smoke evidence.

A skipped `unreal-build-and-test` job is not native certification.

## Next phase

M3.5 should connect the reactive environment to **creative ecological interventions and unlocks**: building structures that affect environmental dimensions, deterministic creative rewards, and world consequences that arise from construction rather than only fixed care targets.
