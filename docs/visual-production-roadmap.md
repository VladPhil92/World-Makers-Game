# World Makers — Visual Production Roadmap

## Purpose

This track moves World Makers from source-controlled visual proxies to an original, production-quality stylized 3D identity. It runs alongside gameplay and learning-system development and has its own acceptance criteria.

The target remains: **premium stylized 3D, readable on tablets, expressive rather than photoreal, richer than a voxel sandbox, and recognizably World Makers.**

A source-complete phase is not automatically art-certified. Binary Unreal assets, authored maps, final capture, and representative-device performance require Unreal Editor/DCC production and measured evidence.

## V1 — Visual Production Foundation / Look Development

Status: **source-complete in PR #49; native visual certification pending.**

- extend `UWMVisualProfileSettings` beyond density budgets into reproducible look-development controls;
- introduce atmospheric sun, skylight, sky atmosphere and height fog into the current Caribbean Rainforest slice;
- make camera FOV part of the visual profile;
- add deterministic procedural movement to the primitive avatar so camera, cadence and silhouette can be judged in motion;
- define the V1–V8 production sequence and visual acceptance boundary;
- add automation/source gates for look settings and proxy motion;
- preserve explicit proxy status: engine primitives are not production art.

Exit: source CI passes and the prototype has a reproducible atmosphere + motion layer. Native visual approval remains blocked until rendered in the certified Unreal environment.

## V2 — Materials & Surface Language

Status: **source implementation deployed on `feat/v2-materials-surface-language`; native authored-material certification pending.**

Goal: establish the material grammar that makes different asset families feel like one game.

Implemented source/runtime contract:

- stable surface roles for earth, terrain, bark, foliage, stone, water, neutral/Eco construction, placement states and magical accents;
- profile-driven base color, roughness, metallic, emissive, wind and opacity intent;
- stable Custom Primitive Data layout for future authored materials;
- temporary visible `/Engine/BasicShapes/BasicShapeMaterial` fallback so existing proxies receive semantic color now;
- rainforest proxy surface pass across ground, terrain, trunks, canopy, stone and water;
- build surface differentiation plus valid/invalid preview roles;
- valid/invalid custom-depth stencil preserved so placement state never becomes color-only;
- per-tier material-slot, texture-edge and sampled-texture ceilings;
- machine-readable handoff contract for `M_WM_MasterSurface` and `M_WM_Water`;
- source/automation tests and Repository Quality gate.

Authored production target:

- `M_WM_MasterSurface` with controlled base tint, roughness, restrained normal detail, packed masks and stylized macro variation;
- material instances for earth/clay, stone, wood/bark, leaves, painted construction pieces and fantasy surfaces;
- masked/two-sided foliage treatment with restrained overdraw and tablet fallback;
- `M_WM_Water` for shallow edges, rivers and later magical/scientific states, with intentional opaque Low-tier fallback until profiling validates translucency;
- trim-sheet / atlas strategy for reusable architecture and props.

Exit: representative proxy and first authored meshes can share a coherent surface system without bespoke shaders per asset. Source exit can pass before binary `.uasset` art certification; authored master materials still require Unreal Editor and native visual review.

See `docs/v2-materials-surface-language.md` and `content/visual/materials/surface-language-v2.json`.

## V3 — Environment Art / Biome Production

V3 status: source-complete procedural art pass; authored DCC mesh replacement and native/device art certification pending.

Goal: replace the visible Caribbean Rainforest primitive composition with an original modular environment language while keeping gameplay collision semantics independent from art iteration.

Implemented source/runtime contract:

- explicit `ProceduralMeshComponent` runtime dependency;
- six render-only procedural art components for ground, terrain, bark/roots, foliage, stone and water;
- original deterministic triangle builders instead of wrapping `/Engine/BasicShapes/*` in the new render path;
- irregular ground silhouette and faceted terrain mounds;
- tapered trunks with deterministic lean and width variation;
- buttress roots and three distinct canopy silhouettes;
- radial understory leaf clusters;
- faceted asymmetric boulders;
- continuous sinuous river-edge strip;
- deterministic hero-ceiba landmark with six buttress roots and multi-mass crown;
- V2 surface-role integration across all V3 visual families;
- hidden Engine-primitive HISM path retained only for collision/fallback and reversible visual rollback;
- quality-tier structural tessellation for ground, tree/canopy facets, river and understory density;
- canonical/staged machine-readable environment-family manifest;
- Unreal Automation tests for determinism, tree silhouette composition, understory and continuous river geometry;
- dedicated Repository Quality V3 gate.

Authored production target:

- replace each procedural family with original DCC-authored static meshes under `/Game/WorldMakers/Environment/CaribbeanRainforest/...`;
- preserve the V2 material-role contract when replacing geometry;
- preserve quiet build clearing, denser perimeter framing, one-sided water boundary and hero-landmark readability;
- add explicit LOD/HLOD or equivalent fallback strategy and cull distances per authored family;
- add measured triangle, draw-call, texture and foliage-overdraw evidence on representative tablets;
- retain gameplay collision independently from cosmetic silhouette where practical.

Exit: no major visible rainforest family depends on `/Engine/BasicShapes/*` in the active V3 render path. Source exit may pass with procedural source art; final environment-art certification still requires authored `.uasset` families, native Unreal rendering and V8 device evidence.

See `docs/v3-environment-art-biome-production.md` and `content/visual/environment/caribbean-rainforest-v3.json`.

## V4 — Character Art & Rig

Goal: replace the six-part primitive child proxy with the production avatar foundation.

- original 4.5–5-head stylized child silhouette;
- neutral inclusive base proportions;
- production skeletal mesh and skeleton;
- modular hair, clothing, skin tone and accessory attachment contract;
- expressive face strategy appropriate to platform budget;
- hand/foot readability for building and exploration;
- LOD and skinning budgets;
- collision and gameplay capsule kept independent from cosmetic silhouette.

Exit: the production avatar can locomote and receive modular customization without changing gameplay collision semantics.

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

Exit: the primitive procedural motion proof is no longer the production animation path.

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
