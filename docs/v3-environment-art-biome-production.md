# V3 — Environment Art / Biome Production

## Status

**Source-complete procedural environment art pass.** Authored DCC/static-mesh replacement, native Unreal render approval and representative-device certification remain pending.

## Purpose

V3 moves the Caribbean Rainforest from visible Engine primitives toward an original World Makers environment language. The source-controlled implementation deliberately separates **what the player sees** from **what gameplay collides with** so art iteration cannot silently change building, navigation or interaction semantics.

## Visible V3 render path

`AWMCaribbeanRainforestPrototype` now owns six render-only `UProceduralMeshComponent` families:

- `GroundArt` — irregular low-poly ground silhouette;
- `TerrainArt` — faceted terrain mounds;
- `BarkAndRootsArt` — tapered trunks and buttress roots;
- `FoliageArt` — three canopy silhouettes plus radial understory leaf clusters;
- `StoneArt` — asymmetric faceted boulders;
- `WaterArt` — continuous sinuous river-edge strip.

A deterministic hero ceiba provides a landmark silhouette using a wide tapered trunk, six buttress roots and three crown volumes. It is an ecological/visual landmark, not a cultural reconstruction.

## Geometry vocabulary

`FWMProceduralEnvironmentGeometry` builds triangles directly and does not wrap `/Engine/BasicShapes/*` assets. The reusable vocabulary is:

- irregular ground disc;
- tapered low-poly trunk;
- faceted ellipsoid;
- buttress-root wedge;
- radial leaf cluster;
- sinuous river strip.

The geometry is intentionally faceted and stylized. It is a source-controlled art-development bridge, not the final DCC asset library.

## Tree variation

Trees no longer share one visible sphere-on-cylinder silhouette. The procedural art pass cycles three crown families:

1. broad single crown;
2. split offset crown;
3. flattened umbrella crown with a secondary upper mass.

Trunk height, width, lean, root presence and canopy proportions vary deterministically from the biome seed. Every second cluster can add understory foliage; every third tree can expose buttress roots.

## Collision/render separation

The previous HISM Engine primitives remain in the actor as **hidden collision/fallback proxies**. When `bUseProceduralEnvironmentArt=True`:

- procedural meshes are visible and collision-free;
- legacy HISM meshes are hidden from rendering;
- existing collision semantics remain available;
- disabling the V3 flag restores the prior visible fallback path.

This is intentional. V3 must improve appearance without changing gameplay authority.

## Surface integration

V3 consumes the V2 surface language rather than defining bespoke materials per family:

- ground → `ground-earth`;
- terrain → `terrain`;
- trunks/roots → `bark`;
- canopies/understory → `foliage`;
- boulders → `stone`;
- river → `water`.

The temporary `BasicShapeMaterial` is used only as a color-capable fallback material while production `M_WM_MasterSurface` and `M_WM_Water` assets remain unauthored. Geometry identity is independent from that fallback.

## Quality scaling

The V3 geometry vocabulary scales structural tessellation by visual tier:

| Tier | Facet segments | Ground segments | River segments | Leaves/cluster |
|---|---:|---:|---:|---:|
| Low | 5 | 18 | 10 | 4 |
| Mid | 6 | 26 | 16 | 5 |
| High | 8 | 34 | 22 | 7 |

Existing V1/V2 scene-density and material budgets remain authoritative. These segment ceilings are source budgets, not measured GPU proof.

## Machine-readable handoff

`content/visual/environment/caribbean-rainforest-v3.json` is canonical and must remain byte-equivalent to `game/Content/WorldMakers/Visual/caribbean-rainforest-v3.json`.

The manifest records family IDs, surface roles, procedural geometry vocabulary, variant counts, quality-tier complexity and intended `/Game/WorldMakers/Environment/CaribbeanRainforest/...` authored destinations.

## Authored-asset handoff

The next art-production pass may replace procedural geometry family-by-family with static meshes authored in Blender or another DCC tool. Replacement must preserve:

- V2 surface-role semantics;
- gameplay collision behavior;
- quiet central build clearing;
- increasing perimeter density;
- one-sided water boundary;
- hero-landmark readability;
- tablet material/texture budgets.

Each authored family must add LOD/HLOD or equivalent fallback strategy, cull distances, provenance/source notes and measured representative-device evidence before it can be called production-certified.

## What V3 does not claim

V3 does **not** claim that:

- final `.uasset` environment meshes exist;
- UVs/textures have received final art polish;
- LOD/HLOD has been authored and measured;
- foliage overdraw has been certified on tablets;
- the river is a full fluid simulation;
- native UE 5.8.2 rendered screenshots have been approved.

Those require Unreal Editor/DCC work and the native certification infrastructure tracked by Issue #9 and later V8 evidence.

## Acceptance boundary

V3 is source-complete when the default visible rainforest path no longer depends on Engine BasicShapes for its major visual families, the fallback/collision path remains reversible, procedural geometry is deterministic and tested, V2 surface semantics are preserved, canonical/staged manifests match, and Repository Quality passes the V3 source gate.
