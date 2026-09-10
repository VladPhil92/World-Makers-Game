# M3.2 — World Interaction & Deliberate Discovery

## Goal

Convert M3.1 semantic proximity exploration into a deliberate interaction loop: a child looks toward a meaningful environmental point, receives a simple localized Observe affordance, explicitly chooses to observe it, and the biome runtime records stable discovery/observation IDs without coupling the interaction layer to any specific mission.

## Explicit intent instead of proximity completion

M3.1 established zones, points of interest and session-local discovery state. M3.2 makes the Caribbean Rainforest POIs deliberate. Walking near a ceiba, bromeliad cluster or water edge no longer counts as having observed it. `UWMBiomeRuntimeSubsystem::ObserveLocation` continues to resolve zone state but skips POIs marked `requiresInteraction=true`.

A deliberate POI is completed through:

1. the player enters its configured interaction range;
2. the camera/view direction satisfies the focus threshold;
3. the child receives an explicit `Observe` action;
4. `UWMInteractionComponent` sends the selected stable PointId through the interactable target;
5. biome authority re-validates the PointId and distance;
6. stable ObservationId and DiscoveryId values are registered idempotently.

This prevents passive proximity from being confused with deliberate observation and gives future science missions a cleaner evidence boundary.

## Data-driven interaction contract

Each deliberate POI may define:

- `requiresInteraction`;
- `interactionMode` (`observe` or future `inspect`);
- `interactionRadiusCm`;
- `focusRadiusCm`;
- `promptKey`;
- `observationId`.

The canonical Caribbean Rainforest currently defines three deliberate prototype observations: ceiba, bromeliad cluster and water edge. The canonical JSON and packaged Unreal JSON remain byte-equivalent.

`promptKey` is a localization key, not child-authored text. Runtime evidence uses stable IDs only.

## Reusable interaction boundary

`IWMInteractable` defines the reusable world contract for focus and interaction. `AWMEnvironmentalInteractableActor` is the first implementation and acts as a lightweight semantic anchor generated from active biome POI definitions.

The actor contains no Mission runtime dependency. It asks the active biome runtime to validate and register the deliberate action. This lets later environment actors, NPCs or authored assets implement the same interface without making the interaction system understand pedagogy.

## Player focus model

`UWMInteractionComponent` belongs to the player and refreshes focus at a bounded interval. A candidate must:

- be a semantic `AWMEnvironmentalInteractableActor`;
- be inside its interaction radius;
- be inside the global focus distance;
- be sufficiently aligned with the current player view direction.

When several candidates qualify, stronger view alignment wins and distance is used as a deterministic tie-breaker. Explicit interaction also has a short anti-spam interval. Re-observation is allowed, but it does not duplicate stable IDs.

## Child-facing UX

The code-driven HUD receives `FocusedPromptKey`, resolves known prototype keys through `LOCTEXT`, and shows a large `Observe` touch target only while a valid target is focused. Keyboard `G` and the left gamepad face button provide prototype desktop/controller parity.

A raw touch on the world still performs no observation. This preserves the M1.7 safe-touch boundary: interaction requires an explicit action rather than an ambiguous viewport tap.

## Privacy and evidence boundaries

M3.2 stores session-local sets of stable DiscoveryIds and ObservationIds only. It does not introduce child name, email, PII, free-text responses, advertising identifiers, commercial state or raw behavioral profiling.

Observation IDs are an infrastructure primitive for later learning evidence, not themselves a claim that a curriculum objective was mastered. A future mission adapter may consume the biome event, but M3.2 itself has no Mission runtime import.

## Automation and source gates

Unreal Automation coverage includes:

- `WorldMakers.Interaction.Definition.DeliberateMetadata`;
- `WorldMakers.Interaction.Focus.RequiresAimAndRange`;
- `WorldMakers.Interaction.Observation.IdempotentStableIds`.

`scripts/validate-m3-2-world-interaction.py` enforces data/package parity, deliberate POI metadata, focus/range architecture, stable IDs, input/HUD integration, mission decoupling and CI wiring.

## Certification boundary

M3.2 is source-level gameplay infrastructure. It does not certify final interaction feel, accessibility, tablet ergonomics, authored environment art, collision fidelity, UE compilation or device performance.

Issue #9 remains the independent blocker for the locked UE 5.8.2 self-hosted build/test, authored certification `.umap` and retained native/manual evidence. A skipped native job must not be interpreted as certification.
