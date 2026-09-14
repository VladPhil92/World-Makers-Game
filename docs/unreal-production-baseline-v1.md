# World Makers — Unreal Production Baseline v1

Status: **LOCKED FOR PRODUCTION READINESS**  
Engine baseline: **Unreal Engine 5.8.2**  
Machine-readable contract: `content/production/unreal-production-baseline-v1.json`

## 1. Purpose

This document freezes the cross-disciplinary assumptions required to move World Makers from source-heavy development into authored Unreal production without a late architecture rewrite. It connects the GDD, TDD, visual-production work, Unreal-readiness audit, runtime performance catalog and certification infrastructure.

This baseline is a **source-governance contract**. Passing its validator does not mean that Unreal compiles, that the certification map exists, or that representative devices are certified.

## 2. Product target

World Makers is a premium-stylized 3D open-world learning sandbox for children. The Caribbean Rainforest remains the first production vertical slice and the primary runtime view remains first person.

The core production loop is:

`Explore -> Observe -> Discover -> Experiment -> Understand -> Craft/Transform -> Build -> World Reacts -> Evidence/Progress -> Continue or Stop Naturally`

Learning is structural to play. A route is not representative if learning appears only as text or quiz UI without affecting player action or world state.

## 3. Architecture freeze

Authoritative gameplay remains **C++ plus deterministic data-driven runtime**. Blueprints are a composition, presentation and safe-extension layer; they are not the sole authority for progression, science truth, mission evidence or durable state.

The following boundaries are mandatory and are mirrored exactly in the machine-readable contract:

- gameplay rules are independent from rendering;
- science outcomes are independent from VFX;
- mission evidence is independent from presentation;
- save state is independent from transient UObject implementation details;
- child gameplay is independent from payment SDKs and provider checkout;
- authored visual assets may be replaced without changing stable gameplay IDs.

Human-reviewable canonical data should remain outside binary Unreal assets when that improves reviewability. Unreal Data Assets and Data Tables may act as authored or generated runtime adaptation layers.

### 3.1 C++ / Blueprint / authored-content rule

Use C++ for authoritative simulation and progression, performance-sensitive reusable systems, stable runtime interfaces, save contracts, validation-critical state and reusable interaction semantics.

Use Blueprints for level composition, authored actor assembly, presentation orchestration, designer-safe tuning, animation/UI binding and sequencing that does not redefine simulation truth.

Use authored Unreal assets for maps, materials, meshes, skeletal rigs, animation sequences and Animation Blueprints, Niagara systems, audio/MetaSounds where adopted, UMG presentation assets and PCG graphs.

## 4. Unreal runtime baseline

Production readiness is locked to **Unreal Engine 5.8.2** until an Architecture Decision Record explicitly changes the baseline.

Required project:

`game/WorldMakers.uproject`

First certification level:

`/Game/WorldMakers/Maps/WM_PrototypeCertification`

A hosted source check is not proof of native Unreal readiness. Native evidence requires a successful `WorldMakersEditor Win64 Development` build plus the required `WorldMakers.*` automation suite.

## 5. Visual target

The visual target is **premium stylized high fidelity**, not photorealism and not voxel abstraction.

Required qualities include:

- strong silhouette hierarchy;
- rich but readable environmental composition;
- physically coherent stylized materials;
- high-quality desktop lighting;
- explicit constrained-device fallbacks;
- environmental storytelling without clutter;
- presentation that communicates gameplay state.

Desktop reference may use Nanite, Lumen, Virtual Shadow Maps, volumetrics, Niagara and high-quality post-processing when measured and justified. These are capabilities, not universal requirements. Tablet paths remain governed by the canonical performance profiles.

### 5.1 Material strategy

World Makers uses a reusable stylized-PBR material grammar rather than one expensive bespoke shader per asset. Production surfaces may express controlled wetness, dirt/mud, age, ecological state, damage where appropriate, emissive/scientific feedback and biome-scale macro variation. Gameplay state must not rely on color alone.

## 6. Animation target

Animation is a first-class production system rather than end-stage polish.

### 6.1 First-person stack

The primary runtime view must progress from source proxies to authored assets using:

- authored skeletal arms/hands;
- authored tools and science/building devices;
- Animation Blueprint control;
- IK Rig;
- Control Rig;
- contextual interaction alignment;
- Motion Warping where useful;
- Reduced Motion alternatives.

Animation must communicate weight, intent and gameplay state rather than resembling generic transforms attached to a camera.

### 6.2 Full-body and NPC direction

For visible full-body characters and advanced NPC locomotion, production may evaluate **Motion Matching**, Full Body IK, terrain adaptation, contextual interaction alignment, authored transitions, expressive idles/gestures and animation LOD.

Motion Matching is a quality direction to evaluate after native stability; it must not become a blocker for the first certification map.

## 7. World production strategy

World Makers uses a hybrid authored/procedural strategy:

- **World Partition** when world scale justifies streaming;
- **PCG**-assisted distribution for vegetation, rocks and biome dressing;
- handcrafted points of interest, mission spaces and science interaction scenes;
- deterministic authored certification routes.

The governing rule is **procedural macro-distribution + handcrafted meaningful spaces**.

The world must visibly react to player action. The first vertical slice must demonstrate at least one causal chain such as:

`observe condition -> diagnose system -> perform science/building intervention -> world state changes -> environment visibly responds -> mission evidence records outcome`

## 8. Performance baseline

The machine-readable baseline must stay synchronized with `content/performance/tablet-performance-profiles.json`. The production validator compares target FPS, p95 frame-time budget and screen percentage for every locked tier.

| Tier | Target | Maximum p95 frame time | Screen percentage |
| --- | ---: | ---: | ---: |
| Tablet Low | 30 FPS | 33.34 ms | 70% |
| Tablet Medium | 30 FPS | 33.34 ms | 85% |
| Tablet High | 60 FPS | 16.67 ms | 100% |
| Desktop Reference | 60 FPS | 16.67 ms | 100% |

Every production-art handoff should identify intended tier, geometry/LOD or Nanite strategy, texture/memory intent, material/shader complexity, collision policy, skeletal/animation cost where relevant, VFX cost, fallback behavior and profiling status.

## 9. First production vertical-slice contract

One coherent certification route must prove all of the following:

1. Spawn and stable first-person control.
2. Observe/scan a world object or condition.
3. Collect or access a resource.
4. Perform a science interaction driven by the simulation/runtime rather than a scripted fake result.
5. Craft or transform material/state.
6. Build or place a meaningful intervention.
7. Trigger a visible ecosystem/world consequence.
8. Record valid mission/learning evidence.
9. Save and reload relevant persistent state.

The route must also establish authored certification geometry, production-path asset naming, camera comfort, an authored animation foundation, stylized material/lighting language, budgeted VFX, spatial/ambient audio foundations and measured performance evidence.

## 10. Production gates

### G0 — Source and architecture readiness

Required evidence:

- hosted source CI green;
- production-baseline validator green;
- LFS policy clean;
- Unreal source preflight green.

### G1 — Native Unreal readiness

Required evidence:

- `WorldMakersEditor Win64 Development` build green;
- `WorldMakers.*` automation green;
- no known fatal compiler defects.

### G2 — Authored vertical slice

Required evidence:

- certification map committed through Git LFS;
- first-person route playable;
- authored assets resolve through stable production paths.

### G3 — Visual and animation target

Required evidence:

- lighting/material/animation/VFX quality review;
- Reduced Motion review;
- no placeholder proxies in the certified route except explicit fallback/debug paths.

### G4 — Performance certification

Required evidence:

- desktop reference budget pass;
- representative iPadOS evidence;
- representative Android evidence;
- fail-closed certification result.

## 11. Git LFS boundary

Authored Unreal binary assets remain governed by `.gitattributes`. At minimum, `.uasset` and `.umap` files must remain tracked through Git LFS. A pointer mismatch is a repository/content-delivery defect, not a reason to reinstall Unreal Engine.

## 12. Change control

An ADR is required before changing any of these locked decisions:

- engine family;
- certification patch;
- authoritative gameplay boundary;
- primary runtime view;
- performance targets;
- child-safety/commerce trust boundary.

Changes to the production gate names/evidence, mandatory architecture boundaries, vertical-slice capabilities or canonical performance bindings require coordinated updates to the machine-readable baseline and its validator.

## 13. Phase 0 exit condition

Phase 0 is complete when the machine-readable contract exists, this document is coherent with it, hosted validation is green, and engine version, project path, LFS policy, certification map path, performance budgets, architecture boundaries and G0–G4 evidence requirements remain mutually consistent.

Completing Phase 0 does **not** mean authored Unreal production is certified. It establishes the stable contract against which native and authored phases can be executed.

## 14. Certification boundary

Source governance, native Unreal readiness, authored-route certification and representative-device certification remain separate states. A green hosted workflow must never be represented as proof that the game is runtime- or device-certified.
