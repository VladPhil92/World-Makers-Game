# World Makers — Authored Visual Production Roadmap

## Purpose

The V1–V8 track established the visual systems and the fail-closed device-certification contract. This roadmap covers the next stage: replacing procedural/source-visible stand-ins with authored production art while preserving the runtime contracts already proven by CI.

The phases below do not redefine gameplay or pedagogy. They deliver the binary art that the visual systems are waiting for.

## P1 — Authored Asset Production Pipeline

Status: **source-complete; binary art population pending.**

- canonical asset registry for environment, character, material, animation, VFX and presentation targets;
- centimeters / +Z up / +X forward DCC contract;
- `/Game/WorldMakers/` namespace and kind-specific naming rules;
- explicit `authoredPresent` truth flag;
- LOD and material-slot budgets enforced by the native loader;
- deterministic environment HISM replacement bridge;
- automatic player Skeletal Mesh preparation;
- procedural fallback preserved when authored assets are absent or invalid;
- production queue report and CI gate.

Exit: the project can accept real assets without code-path rewrites or silent loss of fallback/collision authority.

## P2 — Authored Rainforest Asset Pack

Status: **source-art pack complete; native `.uasset` import and visual review pending.**

Implemented source-production contract:

- nine environment families exactly matching the P1 rainforest takeover set;
- original deterministic OBJ geometry generator using centimeters / +Z up / +X forward;
- explicit UV0 and vertex normals;
- three distinct tree silhouettes and a separate hero ceiba;
- explicit authored LOD groups: Ground 3, Terrain 3, Tree A/B/C 3, Understory 2, Rock 3, Water Edge 2, Hero Ceiba 4;
- material-slot ceilings remain within P1 budgets;
- canonical SHA-256 for every generated source mesh;
- source validator regenerates the pack and verifies hashes, UV/normals/faces, LOD counts and materials;
- Blender background conversion from OBJ LOD groups to per-LOD FBX payloads;
- Unreal Editor Python import path using the native Static Mesh editor LOD pipeline;
- native import report for LOD count, material slots, UV channels and triangle presence;
- manual self-hosted P2 import workflow;
- P1 all-or-nothing rainforest takeover and collision proxies remain authoritative;
- no automatic `authoredPresent=true` mutation.

Exit for source phase: reproducible geometry, LOD/material contract, Blender→FBX bridge, Unreal import bridge, docs and CI pass.

Exit for art/native phase: all nine `.uasset` Static Meshes exist at their P1 object paths, pass native import and human silhouette/UV/LOD/shading review, then all nine manifest entries may be deliberately marked `authoredPresent=true` and the authored runtime takeover may be tested.

See `docs/p2-authored-rainforest-asset-pack.md` and `content/visual/authored/rainforest-p2-source-pack.json`.

## P3 — Authored Character + Modular Cosmetics

Status: **source-complete; native `.uasset`, deformation, IK/Physics and device review pending.**

Implemented source-production contract:

- deterministic original Child Explorer source geometry preserving the V4 158 cm / five-head silhouette;
- exact 19-joint V4 hierarchy with explicit rest locations;
- normalized skin weights with a hard ceiling of four influences per vertex;
- all eight V4 customization slots: body, hair, top, bottom, footwear, head accessory, back accessory and hand prop;
- body/top/bottom/footwear with four authored LODs, hair with three and small socket accessories with two;
- strictly decreasing source triangle counts and V4/P1 budget validation;
- UV0, source normals and stable material roles in the canonical source bundle;
- canonical SHA-256 over the complete generated source bundle;
- Blender armature/vertex-group construction and per-module/per-LOD FBX export;
- Unreal Skeletal Mesh import first, shared Skeleton reuse for skinned cosmetics and Static Mesh import for socket accessories;
- Physics Asset creation request on native body import while deformation, Physics Asset and IK Rig quality remain explicit review items;
- `ACharacter.GetMesh()` remains the authored body destination and `ACharacter.CapsuleComponent` remains collision authority;
- modular plan uses shared Skeleton + leader pose for hair/top/bottom/footwear and stable sockets for head/back/hand accessories;
- no biometric capture, photo-derived avatar generation, face training or voice training;
- manual self-hosted P3 Blender→FBX→Unreal import workflow;
- no automatic `authoredPresent=true` mutation; procedural fallback remains authoritative until native approval.

Exit for source phase: reproducible skinned geometry, rig/weights/LOD contract, modular cosmetic source set, Blender→FBX bridge, Unreal import bridge, docs and Repository Quality gate pass.

Exit for art/native phase: `SK_WM_ChildExplorer` and approved modular assets exist as native `.uasset` files, pass Skeleton/deformation/socket/Physics Asset/IK Rig/material review, work with V5 animation and representative devices, then the P1 character entry may be deliberately marked `authoredPresent=true`.

See `docs/p3-authored-character-cosmetics.md` and `content/visual/authored/character-p3-source-pack.json`.

## P4 — Authored Animation, VFX & Presentation

Status: **source-complete; native AnimBP/Niagara/Level Sequence art review pending.**

Implemented source-production contract:

- 17 authored animation clips exactly covering the 9 V5 locomotion states and 8 interaction actions;
- 30 fps in-place curves on the P3/V4 19-joint Skeleton with root motion disabled;
- interaction durations and layer semantics locked to V5;
- deterministic `ABP_WM_ChildExplorer` state/read-model plan using the existing V5 animation read model;
- Blender baking/export bridge producing one animation FBX per clip from the P3 armature;
- Unreal import bridge targeting the P3 Skeleton and reporting native `AnimSequence` import readiness;
- 17 authored VFX recipes exactly matching V6 event IDs, domains, shapes and Niagara target paths;
- tablet-first particle ceilings, reduced-motion scale, semantic intensity/direction parameters and no collision/light renderer requirement;
- source presentation pack copying all six V7 camera modes and six UI cues without semantic drift;
- two authored microsequence timelines: Adventure Reveal and Science Reveal, both under the V7 2.5 s ceiling;
- no input lock, forced ViewTarget or global time-scale changes;
- Unreal handoff can create Level Sequence containers/playback ranges while camera-track composition remains native human-authored work;
- canonical SHA-256 over the complete generated P4 source bundle;
- manual self-hosted Blender→FBX→Unreal handoff workflow;
- no automatic `authoredPresent=true` mutation; V5/V6/V7 procedural/runtime fallbacks remain authoritative until native approval.

Exit for source phase: deterministic animation curves, VFX recipes, camera/UI timelines, Blender animation export, Unreal animation/presentation import bridge, docs and Repository Quality gate pass.

Exit for art/native phase: imported AnimSequences, final `ABP_WM_ChildExplorer`, reviewed Niagara systems, final camera Data Asset and Level Sequence camera tracks exist as native assets; timing/deformation/overdraw/camera comfort/UI motion pass review on representative devices before P1 authored flags change.

See `docs/p4-authored-animation-vfx-presentation.md` and `content/visual/authored/p4-motion-vfx-presentation.json`.

## P5 — Art Polish & Device Certification

Status: **source-complete certification infrastructure; native authored/device certification remains BLOCKED until real evidence exists.**

Implemented source-production contract:

- final material/lighting review contract for master surface, water, bounded exposure and shadow quality;
- environment polish gates for LOD transitions, HLOD/culling, foliage overdraw, texture streaming and zero texture-pool over-budget allowance;
- character review requiring final deformation/IK/Physics Asset/cosmetic approval, <=3 cm observed foot sliding and zero critical deformation/clipping defects;
- VFX review for all 17 semantic systems, reduced-motion behavior, overdraw and profile-specific GPU p95 ceilings;
- camera/UI review for comfort, safe areas, legibility and non-intrusive reveal behavior;
- native Unreal inventory collector covering all 16 P1 targets, all 17 AnimSequence assets, all 17 Niagara targets and the final rig/presentation assets;
- four SHA-256-verified human review artifacts;
- composition with V8 certification plus a stronger six-package requirement: Android Low/Mid/High and iPadOS Low/Mid/High from one exact commit;
- final fail-closed assessor and manual self-hosted Unreal certification workflow;
- source CI explicitly cannot self-certify production art.

Exit for source phase: P5 contract, template, native inventory collector, assessor self-test, docs and Repository Quality gate pass.

Exit for native/device phase: every required `.uasset` exists and is deliberately approved, all polish reviews pass, all six Android/iPadOS tier packages pass V8 on the same build, artifact hashes verify, and the P5 assessor returns `CERTIFIED`.

See `docs/p5-art-polish-device-certification.md` and `content/visual/authored/p5-art-polish-certification.json`.

## Production rule

A phase may be source-complete without being art-complete. An asset path, manifest row, generated proxy or successful source validator is not evidence that a final binary asset has been authored. `authoredPresent=true` is reserved for assets that actually exist at their declared Unreal path and have passed the P1 runtime contract.
