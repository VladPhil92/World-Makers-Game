# World Makers — Visual Production Roadmap

## Purpose

This track moves World Makers from source-controlled visual proxies to an original, production-quality stylized 3D identity. It runs alongside gameplay and learning-system development and has its own acceptance criteria.

The target remains: **premium stylized 3D, readable on tablets, expressive rather than photoreal, richer than a voxel sandbox, and recognizably World Makers.**

A source-complete phase is not automatically art-certified. Binary Unreal assets, authored maps, final capture, and representative-device performance require Unreal Editor/DCC production and measured evidence.

## V1 — Visual Production Foundation / Look Development

Status: **source-complete in PR #49; native visual certification pending.**

- reproducible atmosphere, sunlight, skylight, fog and camera FOV;
- deterministic temporary avatar motion;
- V1–V8 production sequence and source gate.

Exit: source CI passes and the prototype has a reproducible atmosphere + motion layer.

## V2 — Materials & Surface Language

Status: **source-complete in PR #50; native authored-material certification pending.**

- stable material roles and Custom Primitive Data layout;
- semantic proxy coloration and build-placement feedback;
- tablet-first material/texture budgets;
- master-surface and water handoff contracts.

Exit: representative proxy and first authored meshes can share a coherent surface system without bespoke shaders per asset.

## V3 — Environment Art / Biome Production

V3 status: source-complete procedural art pass.

Status: **source-complete in PR #51; authored DCC mesh replacement and native/device art certification pending.**

- procedural render-only art for ground, terrain, bark/roots, foliage, stone and water;
- irregular ground, faceted terrain, multiple tree/canopy silhouettes, buttress roots, understory, boulders, river and hero ceiba;
- hidden Engine primitives retained only for collision/fallback;
- quality-tier tessellation, environment-family manifest and V3 gate.

Exit: no major visible rainforest family depends on `/Engine/BasicShapes/*` in the active V3 render path.

## V4 — Character Art & Rig

Status: **source-complete implementation on `feat/v4-character-art-rig`; authored Skeletal Mesh/skin/IK certification pending.**

Goal: replace the six-part visible primitive child proxy with an original, animation-ready avatar foundation while preserving gameplay movement and collision authority.

Implemented source/runtime contract:

- `ChildExplorerV1` default silhouette at 158 cm and exactly 5 heads tall;
- neutral child base with no gender-default marker;
- original procedural torso, faceted head, clustered hair, segmented arms/hands, legs and readable feet;
- 19 stable rig joints from `root` through head, arms and legs;
- V1 gait redirected through the V4 rig pivots for source-visible motion proof;
- 8 stable customization slots: body, hair, top, bottom, footwear, head accessory, back accessory and hand prop;
- stable head/back/left-hand/right-hand socket intent;
- `ACharacter::GetMesh()` reserved as the production Skeletal Mesh destination;
- automatic visual-path selection between procedural V4 art, authored Skeletal Mesh and legacy six-piece fallback;
- Low/Mid/High triangle, bone, influence and material-slot budgets;
- canonical/staged avatar-rig manifest with authored asset targets;
- explicit privacy boundary: no biometric capture, photo-avatar generation or child face/voice training;
- Unreal Automation tests for proportions, rig hierarchy and procedural geometry budgets;
- dedicated Repository Quality V4 gate.

Authored production target:

- `/Game/WorldMakers/Characters/Player/SK_WM_ChildExplorer`;
- `/Game/WorldMakers/Characters/Player/SKEL_WM_ChildExplorer`;
- production UVs, skin weights and modular clothing/hair meshes;
- IK Rig and Physics Asset matching the 19-joint semantic chains;
- facial strategy appropriate to the tablet performance tier;
- measured LOD, skinning and draw-call evidence;
- preserve `CharacterMovementComponent` and gameplay capsule authority.

Exit: the source avatar has a stable character silhouette, rig vocabulary, customization/attachment contract and skeletal handoff without changing gameplay collision semantics. Final character-art certification still requires authored binary assets and native Unreal review.

See `docs/v4-character-art-rig.md` and `content/visual/character/avatar-rig-v4.json`.

## V5 — Character & Interaction Animation

Goal: make the world feel responsive through readable authored motion.

- idle, walk, jog/run, start/stop, turns, jump/fall/land;
- additive look/aim and interaction poses;
- build/place/remove/measure/observe interaction animations;
- pickup, inspect and science manipulation actions;
- contextual reactions and non-manipulative celebratory feedback;
- animation blueprint/state-machine or equivalent production graph;
- retargeting and montage conventions;
- restrained secondary motion and animation LOD.

Exit: the primitive/procedural motion proof is no longer the production animation path.

## V6 — VFX & Fantastic/Scientific Feedback

Goal: communicate causality and wonder visually without obscuring the learning signal.

- Niagara effect language for discovery, construction, rewards and transitions;
- chemistry state/reaction feedback tied to simulation outcomes rather than fake spectacle;
- forces, circuits, energy and wave visualization;
- biological/cellular and botanical state visualization;
- ecology recovery/stress cues;
- fantasy portals, runes, story transformations and environmental magic;
- accessibility-safe flash/contrast/duration limits;
- VFX LOD, overdraw and particle budgets.

Exit: important world-system changes have a readable visual consequence and scalable VFX tier.

## V7 — Camera, Cinematics & UI Motion

Goal: create a coherent presentation language from free building to fantastic adventures.

- exploration/build camera tuning and collision behavior;
- contextual framing for observation, science and dialogue;
- short in-engine adventure reveals rather than interruptive long cinematics;
- camera transitions with motion-sickness-safe defaults;
- UI entrance/exit/progress motion system;
- mission beat transitions synchronized with world feedback;
- screenshot/trailer capture presets for consistent marketing evidence.

Exit: gameplay, adventures and UI transitions share one visual rhythm and camera grammar.

## V8 — Visual Optimization & Device Certification

Goal: prove the visual target on representative hardware rather than assuming desktop/editor quality translates to tablets.

- measured GPU/CPU/frame-time captures by visual tier;
- texture memory and streaming budgets;
- draw-call/material-slot audit;
- foliage overdraw audit;
- LOD/HLOD/culling verification;
- shadow and lighting scalability;
- Nanite/Lumen eligibility with documented fallback paths;
- representative iPadOS and Android visual/performance evidence;
- authored certification map and deterministic screenshot route;
- visual regression/reference capture set.

Exit: visual quality claims are backed by native Unreal and representative-device evidence. Issue #9 or its successor certification infrastructure must be resolved before this state can be claimed.

## Production principle

**Silhouette → composition → motion → material → effects → detail.**

Surface fidelity must not be used to compensate for weak silhouettes, unreadable scenes or inert movement. World Makers should feel alive at the lowest viable visual tier before higher-end rendering features are layered on top.
