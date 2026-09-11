# P2 — Authored Rainforest Asset Pack

## Status

**Source-art pack implemented; native `.uasset` import/review still required.**

P2 is the first production phase after P1 where World Makers contains reproducible authored environment geometry rather than only procedural runtime stand-ins and future object paths.

## Scope

P2 covers exactly the **nine environment families** required by the P1 rainforest takeover contract:

1. ground;
2. terrain mound;
3. Tree A — tall/columnar canopy;
4. Tree B — leaning/split canopy;
5. Tree C — broad/umbrella canopy;
6. understory;
7. rock;
8. water edge;
9. hero ceiba.

The pack intentionally does not add buildings, cultural architecture or a second biome.

## Source-art format

`scripts/generate-p2-rainforest-source.py` deterministically generates nine OBJ source meshes under `SourceArt/WorldMakers/Environment/Rainforest/GeneratedP2` (or another requested output directory).

Every generated mesh uses:

- centimeters;
- +Z up;
- +X forward;
- explicit vertex normals;
- UV0 coordinates;
- stable `LOD0`, `LOD1`, ... object groups;
- no more material slots than the P1 ceiling;
- original World Makers low-poly silhouettes rather than Engine BasicShapes.

The canonical hashes and required LOD counts live in `content/visual/authored/rainforest-p2-source-pack.json`. The Repository Quality P2 gate regenerates the pack in a temporary directory and compares SHA-256, UV/normal/faces, LOD count and material usage.

## Art direction

The first authored rainforest pack preserves the silhouette language proven in V3 while converting it into an external production surface:

- trees remain visibly different at mid-distance, not palette swaps;
- the hero ceiba is substantially larger than ordinary trees and retains a landmark crown/trunk ratio;
- foliage is massed into readable forms instead of realistic leaf noise;
- rocks use asymmetric faceting;
- terrain remains soft/faceted rather than voxel-like;
- water edge is visually separated from gameplay collision;
- understory is lightweight and suitable for instancing.

This is a stylized tablet-first base pack, not botanical photorealism.

## LOD contract

The P2 source pack meets the P1 minimum LOD floors:

| Family | Source LODs | Material slots |
|---|---:|---:|
| Ground | 3 | 1 |
| Terrain | 3 | 1 |
| Tree A | 3 | 2 |
| Tree B | 3 | 2 |
| Tree C | 3 | 2 |
| Understory | 2 | 1 |
| Rock | 3 | 1 |
| Water edge | 2 | 1 |
| Hero ceiba | 4 | 2 |

LOD groups are authored explicitly; P2 does not rely on Unreal auto-reduction as the only LOD strategy.

## Native production bridge

The source path is:

`P2 OBJ source → Blender background export → per-LOD FBX → Unreal Editor Python import → native contract report`.

`scripts/blender/export-p2-rainforest-fbx.py` converts each OBJ's LOD groups into separate FBX payloads.

`scripts/unreal/import-p2-rainforest-assets.py` imports LOD0, adds subsequent FBX files through the Static Mesh Editor subsystem, then records:

- imported LOD count;
- material slots;
- UV channel count;
- LOD0 triangle count;
- pass/fail against the P2/P1 contract.

The manual `.github/workflows/p2-rainforest-native-import.yml` executes this pipeline on the self-hosted Windows/X64/Unreal runner when both `UNREAL_ENGINE_ROOT` and `BLENDER_EXE` are available.

## Takeover rule

P1's **all-or-nothing rainforest bridge remains authoritative**. The procedural fallback stays visible until all nine imported Static Meshes pass their native contract and P1's `authoredPresent` values are deliberately reviewed and changed.

P2 import scripts do **not** set `authoredPresent=true` automatically. A successful import proves structural readiness, not visual approval.

Existing deterministic collision proxies remain authoritative even after the authored render set is enabled.

## Human review required before activation

Before changing any of the nine P1 environment entries to `authoredPresent=true`, review in Unreal Editor must confirm:

- silhouette matches the intended family;
- scale/origin/orientation are correct;
- UV0 is usable and has no critical stretching;
- material assignment is correct;
- LOD transitions do not cause critical popping;
- normals/shading are intentional;
- no accidental collision has become gameplay-authoritative;
- visual cost remains compatible with V8 budgets.

Representative tablet evidence remains part of P5/device certification.

## Production boundary

P2 is source-complete when source geometry generation, deterministic hashes, LOD/material validation, Blender export path, Unreal import path, documentation and CI are green.

P2 is **not art-certified** until the native `.uasset` files exist and have passed the human/native review above. The repository must continue to report that distinction explicitly.

## Next phase

**P3 — Authored Character & Cosmetics**: produce the Child Explorer Skeletal Mesh, skinning/LOD source, initial hair/clothing slots and the first real avatar takeover candidate while preserving the procedural fallback and V5 animation read-model.
