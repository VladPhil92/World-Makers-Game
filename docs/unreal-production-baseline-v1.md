# World Makers — Unreal Production Baseline v1

Status: **LOCKED FOR PRODUCTION READINESS**  
Engine baseline: **Unreal Engine 5.8.2**  
Machine-readable contract: `content/production/unreal-production-baseline-v1.json`

## 1. Purpose

This document connects the existing GDD, TDD, art direction, Unreal readiness audit, visual-production work and native-certification infrastructure. It freezes the cross-disciplinary assumptions that must remain coherent while World Makers moves from source-heavy development into authored Unreal production.

The objective is not simply to open the project in Unreal. The objective is a scalable, high-fidelity, premium-stylized game whose gameplay, science systems, learning evidence, animation, rendering and performance survive production without a late architectural rewrite.

## 2. Product target

World Makers remains a premium-stylized 3D open-world learning sandbox for children, with the Caribbean Rainforest as the first production proof.

The core production loop is:

`Explore -> Observe -> Discover -> Experiment -> Understand -> Craft/Transform -> Build -> World Reacts -> Evidence/Progress -> Continue or Stop Naturally`

Learning must be structural to play. The vertical slice is not representative if learning appears only in text or quiz UI.

## 3. Architecture freeze

Authoritative gameplay remains **C++ + deterministic data-driven runtime**. Blueprints are a composition, presentation and safe-extension layer, not the sole authority for progression, science truth, mission evidence or durable state.

Required boundaries:

- gameplay rules independent from rendering;
- science outcomes independent from VFX;
- mission evidence independent from presentation;
- save contracts independent from transient UObject implementation details;
- child gameplay independent from payment SDKs;
- visual assets replaceable without changing stable gameplay IDs.

Human-reviewable canonical data stays outside binary Unreal assets where that improves reviewability. Data Assets/Data Tables may be authored or generated as Unreal runtime adaptation layers.

## 4. Unreal runtime baseline

Production readiness is locked to **Unreal Engine 5.8.2** until an ADR explicitly changes the baseline.

Required project:

`game/WorldMakers.uproject`

First certification level:

`/Game/WorldMakers/Maps/WM_PrototypeCertification`

A hosted source check is not native certification. Native evidence requires a successful `WorldMakersEditor Win64 Development` build and required `WorldMakers.*` automation.

### C++ / Blueprint / authored assets

Use C++ for authoritative simulation/progression, performance-sensitive reusable systems, stable runtime interfaces, save contracts, validation-critical state and reusable interaction semantics.

Use Blueprints for level composition, authored actor assembly, presentation orchestration, designer-safe tuning, animation/UI binding and sequencing that does not redefine simulation truth.

Use authored Unreal assets for maps, materials, meshes, skeletal rigs, animations, animation blueprints, Niagara systems, sound/MetaSounds where adopted, UMG assets and PCG graphs.

## 5. Visual target

The target is **premium stylized high fidelity**, not photorealism and not voxel abstraction.

Required qualities:

- strong silhouette hierarchy;
- rich but readable environmental composition;
- physically coherent stylized materials;
- high-quality desktop lighting;
- explicit constrained-device fallbacks;
- environmental storytelling without clutter;
- presentation that communicates gameplay state.

Desktop reference may use Nanite, Lumen, Virtual Shadow Maps, volumetrics, Niagara and high-quality post-processing when measured and justified. These are capabilities, not universal requirements. Tablet tiers remain performance-governed.

The material grammar remains reusable stylized PBR. Production surfaces should support controlled wetness, dirt/mud, age, ecological state, damage where appropriate, emissive/scientific feedback and biome macro variation. Gameplay state must not rely on color alone.

## 6. Animation target

Animation is a first-class system, not end-stage polish.

### First-person stack

The primary runtime view must progress from source proxies to authored assets using:

- authored skeletal arms/hands;
- authored tools/science/building devices;
- Animation Blueprint control;
- IK Rig;
- Control Rig;
- contextual interaction alignment;
- Motion Warping where useful;
- Reduced Motion alternatives.

Animation must communicate weight, intent and state rather than looking like generic transforms attached to the camera.

### Full-body/NPC high-fidelity direction

For visible full-body characters and advanced NPC locomotion, production may evaluate **Motion Matching**, Full Body IK, terrain adaptation, contextual interaction alignment, authored transitions, expressive idles/gestures and animation LOD.

Motion Matching is a quality target after native stability; it must not block the first certification map.

## 7. World production strategy

World Makers uses a hybrid authored/procedural strategy:

- **World Partition** when world scale justifies streaming;
- **PCG**-assisted distribution for vegetation, rocks and biome dressing;
- handcrafted points of interest, mission spaces and science interaction scenes;
- deterministic authored certification routes.

Governing rule: **procedural macro-distribution + handcrafted meaningful spaces**.

The world must visibly react to player action. The vertical slice must demonstrate at least one chain such as:

`observe condition -> diagnose system -> science/building intervention -> world state changes -> environment visibly responds -> mission evidence records outcome`

## 8. Performance baseline

| Tier | Target | Maximum p95 frame time | Screen percentage |
| --- | ---: | ---: | ---: |
| Tablet Low | 30 FPS | 33.34 ms | 70% |
| Tablet Medium | 30 FPS | 33.34 ms | 85% |
| Tablet High | 60 FPS | 16.67 ms | 100% |
| Desktop Reference | 60 FPS | 16.67 ms | 100% |

Every production art handoff must identify intended tier, geometry/LOD or Nanite strategy, texture/memory intent, material/shader complexity, collision policy, skeletal/animation cost where relevant, VFX cost, fallback behavior and profiling status.

## 9. First production vertical slice contract

One coherent certification route must prove:

1. Spawn and stable first-person control.
2. Observe/scan a world object or condition.
3. Collect or access a resource.
4. Perform a science interaction driven by the simulation/runtime rather than a scripted fake result.
5. Craft or transform material/state.
6. Build or place a meaningful intervention.
7. Trigger a visible ecosystem/world consequence.
8. Record valid mission/learning evidence.
9. Save and reload relevant persistent state.

The route must also establish authored certification geometry, production-path asset naming, camera comfort, authored first-person animation foundations, stylized material/lighting language, budgeted VFX, spatial/ambient audio foundation and measured performance evidence.

## 10. Production gates

### G0 — Source and architecture readiness

Hosted source checks, production-baseline validation, Unreal source preflight, LFS policy and project/engine identity must be coherent.

### G1 — Native Unreal readiness

`WorldMakersEditor Win64 Development` must compile on the locked UE environment, required automation must pass and no known fatal compiler defect may remain.

### G2 — Authored vertical slice

`WM_PrototypeCertification.umap` must exist at the required stable path through Git LFS; the first-person route must be playable and authored assets must resolve through stable production paths.

### G3 — Visual and animation target

Placeholder proxies must be absent from the certified route except explicit fallback/debug paths. Lighting, materials, animation, VFX, camera comfort, Reduced Motion and gameplay readability must pass review.

### G4 — Performance certification

Desktop reference budget plus representative iPadOS and Android evidence must pass. Certification remains fail-closed for missing or inconsistent evidence.

## 11. Change control

An ADR is required before changing the engine family, certification patch, authoritative gameplay boundary, primary runtime view, performance targets or child-safety/commerce trust boundary.

## 12. Phase 0 exit condition

Phase 0 is complete when the machine-readable contract exists, this production baseline exists, CI validates the contract, and engine version, project path, LFS policy, certification map path, performance budgets and G0–G4 gates remain mutually consistent.

Completing Phase 0 does **not** mean authored Unreal production is certified. It means the project has a stable production contract against which the next native and authored phases can be executed.
