# World Makers — Unreal Production Baseline v1

Status: **LOCKED FOR PRODUCTION READINESS**  
Engine baseline: **Unreal Engine 5.8.2**  
Machine-readable contract: `content/production/unreal-production-baseline-v1.json`

## 1. Purpose

This document is the production baseline that connects the existing Game Design Document, Technical Design Document, art direction, Unreal readiness audit, visual production work and native certification infrastructure.

It does not replace those sources. It freezes the cross-disciplinary decisions that must remain coherent while World Makers moves from source-heavy development into authored Unreal production.

The goal is not merely to make World Makers open in Unreal Engine. The goal is to create a scalable, high-fidelity, premium-stylized game whose gameplay, science systems, learning evidence, animation, visual presentation and performance can survive production without a late architectural rewrite.

## 2. Product target

World Makers is a premium-stylized 3D open-world learning sandbox for children. The initial production proof remains the Caribbean Rainforest vertical slice.

The production experience must preserve the established product rules:

- learning is structural to play rather than a worksheet overlay;
- exploration, observation, experimentation, construction and world transformation form the core experiential loop;
- the child runtime remains separated from payment execution;
- visual fidelity must scale across desktop and representative tablet hardware;
- authored production must remain deterministic enough to certify, profile and reproduce from source control.

## 3. Core experiential loop

The baseline production loop is:

`Explore -> Observe -> Discover -> Experiment -> Understand -> Craft/Transform -> Build -> World Reacts -> Evidence/Progress -> Continue or Stop Naturally`

The vertical slice is not considered representative if learning only appears in text or quiz UI. At least one complete route must demonstrate a causal chain where observation and science-informed action change the playable world.

## 4. Architecture freeze

### 4.1 Authoritative gameplay

Authoritative gameplay rules remain in C++ and deterministic data-driven systems. Blueprints are a composition, presentation and safe-extension layer; they are not the sole authoritative source for progression, science truth, mission evidence or durable game state.

Required separation:

- gameplay rules are independent from rendering;
- science outcomes are independent from VFX;
- mission evidence is independent from presentation;
- save state is independent from transient UObject implementation details;
- child gameplay is independent from payment SDKs and provider checkout;
- authored visual assets may be replaced without changing stable gameplay IDs.

This separation is mandatory because high-fidelity production will replace proxies, materials, animation clips, VFX and meshes repeatedly.

### 4.2 Data-driven production

Reusable content must be representable through stable definitions rather than hard-coded per-object branches where practical. Examples include:

- items and resources;
- science substances/reactions;
- mission requirements;
- world-state consequences;
- buildable interventions;
- biome metadata;
- animation action IDs;
- authored asset slots;
- VFX/audio presentation mappings.

Human-reviewable canonical data stays outside binary Unreal assets where it improves reviewability. Unreal Data Assets/Data Tables may be generated or authored as runtime presentation/adaptation layers.

## 5. Unreal runtime baseline

### 5.1 Engine

Production readiness is locked to Unreal Engine **5.8.2** until an Architecture Decision Record explicitly changes the baseline.

The required project is:

`game/WorldMakers.uproject`

The first certification level remains:

`/Game/WorldMakers/Maps/WM_PrototypeCertification`

A hosted source check is not proof of native Unreal readiness. Native evidence requires a successful `WorldMakersEditor Win64 Development` build plus the required `WorldMakers.*` automation suite.

### 5.2 C++ / Blueprint / authored-content rule

Use C++ for:

- authoritative simulation and progression;
- performance-sensitive reusable systems;
- stable runtime interfaces;
- save/persistence contracts;
- validation-critical gameplay state;
- reusable interaction semantics.

Use Blueprint for:

- level composition;
- authored actor assembly;
- presentation orchestration;
- safe designer-tunable behavior;
- animation/UI bindings;
- visual sequencing that does not redefine simulation truth.

Use authored Unreal assets for:

- maps;
- materials/material instances;
- meshes;
- skeletal rigs;
- animation sequences and animation blueprints;
- Niagara systems;
- sound assets and MetaSounds where adopted;
- UMG presentation assets;
- PCG graphs and environment composition.

## 6. Visual target

The visual target is **premium stylized high fidelity**, not photorealism and not voxel-like abstraction.

Required characteristics:

- strong silhouette hierarchy;
- rich but readable environmental composition;
- physically coherent stylized materials;
- high-quality lighting at desktop reference tier;
- intentionally simplified fallbacks on constrained devices;
- environmental storytelling without clutter;
- visual feedback that communicates gameplay state;
- no dependence on excessive microdetail to create perceived quality.

### 6.1 Rendering strategy

Desktop reference may use, when measured and appropriate:

- Nanite;
- Lumen;
- Virtual Shadow Maps;
- high-quality volumetrics;
- budgeted Niagara systems;
- high-fidelity post-processing.

These technologies are **capabilities, not universal requirements**. Every production asset family that uses a high-tier path must define a lower-cost fallback where the target device matrix requires it.

Tablet tiers remain performance-governed. A beautiful asset that violates the profile budget is not production-ready.

### 6.2 Material strategy

World Makers continues with a reusable stylized-PBR material grammar rather than one expensive bespoke shader per asset.

Production materials should support controlled state variation such as:

- wetness;
- dirt/mud;
- age;
- ecological state;
- damage where appropriate;
- emissive/scientific feedback;
- biome-specific macro variation.

Gameplay state must not depend on color alone.

## 7. Animation target

Animation quality is a first-class production system rather than final polish.

### 7.1 First-person production stack

The primary runtime view must progress from source proxies to authored assets using:

- authored skeletal arms/hands;
- authored tools and science/building devices;
- Animation Blueprint control;
- IK Rig where required;
- Control Rig for procedural correction and authoring support;
- contextual interaction alignment;
- Motion Warping where it improves deterministic authored interaction placement;
- Reduced Motion alternatives for camera and presentation movement.

Animation must communicate weight, intent and state. Tool use may not look like a generic transform animation attached to a camera.

### 7.2 Full-body and NPC high-fidelity direction

For visible full-body characters and advanced NPC locomotion, the production path may evaluate:

- Motion Matching;
- Full Body IK;
- terrain adaptation;
- contextual interaction alignment;
- authored transition clips;
- expressive idles and gestures;
- animation LOD/scalability.

Motion Matching is a quality target to evaluate after native stability; it is not allowed to become a blocker for the first certification map.

## 8. World production strategy

World Makers should use a hybrid authored/procedural world strategy.

Target architecture:

- World Partition for scalable world streaming when the authored world size justifies it;
- PCG-assisted distribution for vegetation, rocks, biome dressing and repeatable environment families;
- handcrafted points of interest, mission spaces and science interaction scenes;
- deterministic authored certification routes even when procedural systems are available.

The governing rule is:

**procedural macro-distribution + handcrafted meaningful spaces**.

A fully procedural world without authored composition is not the visual target.

## 9. Living-world requirement

The environment must react visibly to player action. The vertical slice must demonstrate at least one causal chain such as:

`observe condition -> diagnose system -> perform science/building intervention -> world state changes -> environment visibly responds -> mission evidence records the outcome`

This is the core differentiator between an educational overlay and a playable systems world.

## 10. Performance baseline

Performance is a design constraint from the beginning.

| Tier | Target | Maximum p95 frame time | Screen percentage baseline |
| --- | ---: | ---: | ---: |
| Tablet Low | 30 FPS | 33.34 ms | 70% |
| Tablet Medium | 30 FPS | 33.34 ms | 85% |
| Tablet High | 60 FPS | 16.67 ms | 100% |
| Desktop Reference | 60 FPS | 16.67 ms | 100% |

Every production art handoff must identify at minimum:

- intended tier;
- geometry/LOD or Nanite strategy;
- texture resolution and memory intent;
- material slots and shader complexity;
- collision policy;
- skeletal/animation cost where relevant;
- VFX cost where relevant;
- fallback behavior;
- profiling status.

No quality claim is complete without measured evidence on the relevant target class.

## 11. First production vertical slice contract

The certification route must prove all of the following in one coherent playable path:

1. Spawn and stable first-person control.
2. Observe/scan a world object or condition.
3. Collect or access a resource.
4. Perform a science interaction whose outcome comes from the simulation/runtime rather than a scripted fake result.
5. Craft or transform material/state.
6. Build or place a meaningful intervention.
7. Trigger a visible ecosystem/world consequence.
8. Record valid mission/learning evidence.
9. Save and reload relevant persistent state.

The visual route must also establish:

- authored certification geometry rather than primitives alone;
- production-path asset naming;
- stable camera comfort;
- authored first-person animation foundation;
- stylized material and lighting language;
- budgeted VFX;
- spatial/ambient audio foundation;
- measured performance evidence.

## 12. Production gates

### G0 — Source and architecture readiness

Pass only when:

- hosted source CI is green;
- the production-baseline validator is green;
- Unreal source preflight is green;
- Git LFS policy is clean;
- current engine/project identity is coherent.

### G1 — Native Unreal readiness

Pass only when:

- `WorldMakersEditor Win64 Development` builds successfully on the locked UE environment;
- required `WorldMakers.*` automation passes;
- no known fatal compiler defects remain.

### G2 — Authored vertical slice

Pass only when:

- `WM_PrototypeCertification.umap` exists at the required stable path and is committed through LFS;
- the first-person route is playable;
- authored assets resolve through stable production paths;
- required gameplay state survives map/runtime transitions as designed.

### G3 — Visual and animation target

Pass only when:

- placeholder proxies are absent from the certified route except explicit fallback/debug paths;
- lighting, materials, animation and VFX pass a visual review;
- camera comfort and Reduced Motion paths pass review;
- gameplay affordances remain readable under final presentation.

### G4 — Performance certification

Pass only when:

- desktop reference budget passes;
- representative iPadOS evidence exists;
- representative Android evidence exists;
- certification remains fail-closed for missing/incoherent evidence.

## 13. Change control

An ADR is required before changing any of the following production assumptions:

- Unreal engine family;
- certification patch baseline;
- authoritative gameplay boundary;
- primary runtime view;
- performance targets;
- child safety / commerce trust boundary.

This prevents a visually attractive prototype from silently changing the architecture required for production.

## 14. Phase 0 exit condition

This production-baseline phase is complete when:

- the machine-readable contract exists;
- this baseline is referenced from the project documentation;
- Repository Quality validates the contract;
- engine version, project path, LFS policy, certification map path, performance budgets and production gates are mutually consistent.

Completing this phase does **not** mean authored Unreal production is certified. It means the project has a stable contract against which the next native and authored phases can be executed.
