# World Makers — Authored Visual Production Roadmap

## Purpose

The V1–V8 track established the visual systems and the fail-closed device-certification contract. This roadmap covers the next stage: replacing procedural/source-visible stand-ins with authored production art while preserving the runtime contracts already proven by CI.

The phases below do not redefine gameplay or pedagogy. They deliver the binary art that the visual systems are waiting for.

## P1 — Authored Asset Production Pipeline

Status: **source-complete target on `feat/p1-authored-asset-production-pipeline`; binary art population pending.**

- canonical asset registry for environment, character, material, animation, VFX and presentation targets;
- centimeters / +Z up / +X forward DCC contract;
- `/Game/WorldMakers/` namespace and kind-specific naming rules;
- explicit `authoredPresent` truth flag;
- LOD and material-slot budgets enforced by the native loader;
- deterministic environment HISM replacement bridge;
- automatic player Skeletal Mesh adoption;
- procedural fallback preserved when authored assets are absent or invalid;
- production queue report and CI gate.

Exit: the project can accept real assets without code-path rewrites or silent loss of fallback/collision authority.

## P2 — Authored Rainforest Asset Pack

Goal: complete the first real environment takeover.

- ground family;
- terrain mound family;
- Tree A/B/C silhouettes;
- understory family;
- rock family;
- water-edge family;
- hero ceiba;
- production UVs, normals/tangents and V2 material instances;
- authored LOD chains within V8 budgets;
- native asset audit and screenshot comparison against the procedural composition;
- set all nine rainforest manifest entries to `authoredPresent=true` only after native validation.

Exit: the active rainforest uses authored meshes at runtime while collision/world layout remain unchanged.

## P3 — Authored Character + Modular Cosmetics

Goal: replace the V4 procedural child with the production `SK_WM_ChildExplorer` family.

- final child base mesh and skeleton;
- skin weights and deformation review;
- four LODs minimum;
- modular hair/top/bottom/footwear/accessory slots;
- Physics Asset and IK Rig;
- V2 material instances and controlled palette variation;
- avatar silhouette/readability review at gameplay distance;
- no biometric/photo-derived avatar pipeline.

Exit: V5 animation read-model drives an authored Skeletal Mesh without changing the gameplay capsule.

## P4 — Authored Animation, VFX & Presentation

Goal: replace procedural motion/effects/presentation stand-ins with production binary assets.

- `ABP_WM_ChildExplorer` and locomotion clips;
- interaction montages for build/measure/observe/pickup/science;
- foot placement and retargeting;
- Niagara systems bound to V6 semantic events;
- final camera Data Asset;
- short Level Sequences for adventure/science reveals;
- UI motion art respecting V7 reduced-motion states.

Exit: V5–V7 runtime semantics are represented by authored animation/VFX/cinematic assets, not source proxies.

## P5 — Art Polish & Device Certification

Goal: prove the authored game rather than the proxy game.

- final material/lighting polish;
- foliage overdraw and shadow pass;
- LOD/HLOD/culling tuning;
- texture streaming and residency pass;
- animation foot sliding/deformation review;
- VFX overdraw/concurrency review;
- camera comfort and UI legibility review;
- V8 captures across all four stress scenarios;
- Low, Medium and High tablet tiers;
- representative Android and iPadOS packages;
- exact-build SHA-256 evidence and manual V8 `CERTIFIED` result.

Exit: the authored visual stack is device-certified for the captured build and profiles.

## Production rule

A phase may be source-complete without being art-complete. An asset path, manifest row, generated proxy or successful source validator is not evidence that a final binary asset has been authored. `authoredPresent=true` is reserved for assets that actually exist at their declared Unreal path and have passed the P1 runtime contract.
