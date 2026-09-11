# P1 — Authored Asset Production Pipeline

## Status

**Source-complete pipeline target; not art-certified.** P1 turns the V1–V8 visual contracts into a real DCC-to-Unreal replacement path. It does not claim that final binary art exists.

The canonical manifest is `content/visual/authored/authored-assets-p1.json`. Its staged runtime copy is `game/Content/WorldMakers/Visual/Authored/authored-assets-p1.json`.

## Why P1 exists

V1–V8 established visual language, procedural stand-ins, character/animation/VFX/presentation contracts and device-certification infrastructure. Those systems deliberately avoided pretending that source-generated geometry was equivalent to final art.

P1 is the bridge into production assets. It defines how Blender/Substance/other DCC work becomes a World Makers Unreal asset, how that asset is validated, and how the runtime adopts it without changing gameplay authority.

## Coordinate and scale contract

All geometry handoffs use:

- **centimeters** as the Unreal-world unit;
- **+Z up**;
- **+X forward**;
- frozen/applied object transforms before export;
- meaningful pivots rather than arbitrary scene origins;
- no negative production scale;
- render meshes separated from gameplay collision whenever `collisionPolicy=proxy`.

Environment families using the P1 HISM bridge must be authored around the same normalized placement basis used by the V3 collision proxies. The bridge copies deterministic transforms; the art asset changes silhouette and surface fidelity, not world layout or collision authority.

## Naming and namespace

All production objects live under `/Game/WorldMakers/`.

Required prefixes:

| Kind | Prefix |
|---|---|
| Static Mesh | `SM_` |
| Skeletal Mesh | `SK_` |
| Material | `M_` |
| Niagara System | `NS_` |
| Animation Blueprint | `ABP_` |
| Level Sequence | `LS_` |
| Data Asset | `DA_` |

Engine BasicShapes, Starter Content and third-party sample namespaces cannot be marked as authored World Makers production art.

Source work is referenced under `SourceArt/WorldMakers/`. That directory is the DCC/source-art namespace; it is not a gameplay content folder and does not need to be staged into the packaged game.

## `authoredPresent`

Every manifest entry has an explicit `authoredPresent` flag.

`false` means the path is reserved but the asset must not be treated as production-ready. The runtime will not attempt to load it.

`true` means the team is asserting that the corresponding `.uasset` exists. Repository Quality checks file presence. Native runtime loading then checks the declared asset kind and, for meshes, the LOD/material contract. A missing or non-conforming asset is rejected and the existing fallback remains authoritative.

A reserved path is therefore never equivalent to a finished asset.

## Initial production queue

P1 defines 16 production targets spanning:

- rainforest ground and terrain;
- three general tree silhouettes;
- understory;
- rocks;
- water edge;
- hero ceiba;
- `SK_WM_ChildExplorer`;
- master surface and water material targets;
- player Animation Blueprint target;
- science Niagara master target;
- adventure reveal Level Sequence target;
- camera presentation Data Asset target.

Run:

```bash
python scripts/report-p1-authored-assets.py
```

for a human-readable queue, or add `--json` for automation.

## Environment activation: all-or-nothing

The rainforest replacement path is intentionally **all-or-nothing** in P1.

Ground, terrain, Tree A/B/C, understory, rock, water-edge and hero-ceiba must all be marked present and must load successfully before the procedural V3 environment is hidden. This prevents half-authored scenes where some families use final assets while neighboring families still visibly use source proxies.

The bridge creates render-only HISM components and copies the deterministic placement transforms from the existing biome. Existing collision proxy components remain untouched. If any family cannot load or violates its contract, authored components remain hidden and V3 stays visible.

This first authored environment bridge is intentionally conservative. Later production phases can replace the all-or-nothing set with streamed biome chunks after authored composition is visually approved.

## Avatar activation

The player can migrate independently from the environment.

When `character.player.child-explorer` becomes present and passes the P1 loader contract, `UWMAuthoredVisualBridgeSubsystem` assigns the asset to the existing `ACharacter::GetMesh()` Skeletal Mesh destination and calls `RefreshAvatarVisualPath()`.

The gameplay capsule and `CharacterMovementComponent` remain authoritative. The procedural V4 character remains the fallback until the Skeletal Mesh is actually loadable.

P1 does not claim that the final skeleton, skin weights, facial strategy, IK Rig, Physics Asset or Animation Blueprint have already been authored. Those require Unreal Editor/DCC production and native review.

## LOD contract

`minLods` is a minimum production floor, not a request to generate duplicate LODs automatically.

The initial expectations are intentionally stronger for large or persistent objects:

- regular rainforest environment meshes: generally at least 2–3 LODs;
- hero ceiba: at least 4;
- player Skeletal Mesh: at least 4.

When a mesh is marked present, `UWMAuthoredAssetSubsystem` checks the native mesh's LOD count before accepting it. Failure returns `nullptr`, preserving fallback art.

Actual screen-size thresholds, reduction percentages, impostors/HLOD and foliage cull distances require visual profiling in Unreal Editor and are subsequently verified by V8 device evidence.

## Material slots and UVs

The P1 manifest carries a hard `maxMaterialSlots` ceiling of 1–4 depending on the asset. The runtime rejects a present mesh whose material count exceeds its entry.

Production art should normally use:

- UV0 for authored surface coordinates;
- UV1/lightmap channel only where the rendering path actually requires it;
- packed masks/atlases consistent with V2;
- reusable `M_WM_MasterSurface` instances rather than bespoke shaders per prop;
- masked foliage instead of unnecessary translucent leaf surfaces;
- `M_WM_Water` for the dedicated water language.

Material slots are a performance budget, not an organizational convenience.

## Collision

P1 supports three collision policies:

- `none` — presentation-only asset;
- `proxy` — gameplay collision remains owned by the existing deterministic proxy;
- `authored-simple` — a future explicitly reviewed simple authored collision body may be used.

The first rainforest bridge uses render-only authored components even for entries marked `proxy`. It never changes the existing biome collision proxies.

The player Skeletal Mesh remains collision-free; the character capsule is authoritative.

## Runtime architecture

`FWMAuthoredVisualAssetCatalog`
: validates stable IDs, World Makers object/source namespaces and bounded production metadata.

`UWMAuthoredAssetSubsystem`
: loads the staged catalog once per Game Instance and attempts native object loading only for entries marked present. Mesh loads are rejected when LOD or material slots violate the manifest.

`UWMAuthoredVisualBridgeSubsystem`
: applies accepted authored art downstream of gameplay. It can retry briefly during world startup so spawned pawns/biomes are found, and exposes `RefreshAuthoredVisuals()` for explicit refresh after asset/profile work.

Neither subsystem can complete missions, grant rewards, create commerce state or submit learning evidence.

## DCC → Unreal handoff checklist

Before changing an asset to `authoredPresent=true`:

1. Work at the agreed centimeters / +Z up / +X forward basis and apply transforms.
2. Use the exact source-art and Unreal naming contract.
3. Validate silhouette at gameplay distance before adding microdetail.
4. Prepare the required LOD floor; do not submit identical duplicate LODs as a compliance shortcut.
5. Keep material slots within the manifest ceiling and use the V2 material language.
6. Use the declared collision policy; never silently replace a proxy with complex render collision.
7. Import into the exact `/Game/WorldMakers/...` object path.
8. Run native Unreal validation and inspect pivots, normals/tangents, UVs, bounds, LOD transitions, material assignment and collision.
9. Set `authoredPresent=true` only after the `.uasset` exists at that path.
10. Run `python scripts/validate-p1-authored-assets.py` and the normal Repository Quality suite.

## Privacy / source-art boundary

Character source art must not include child photographs, face scans, biometric templates, private voice recordings or identity-derived reference packs. The neutral avatar system defined in V4 remains the design basis.

SourceArt should contain only production work that the project has the right to use and redistribute under its intended asset workflow.

## What P1 automates vs. what still needs production work

P1 automates:

- stable object/source paths;
- manifest parsing;
- honest presence flags;
- native load/fallback selection;
- mesh LOD and material-slot rejection;
- environment set completeness;
- deterministic authored HISM placement;
- player Skeletal Mesh adoption;
- CI naming/namespace/budget validation;
- art-production queue reporting.

P1 cannot author the binary meshes, textures, skin weights, animations, Niagara graphs or Level Sequences by itself. Those require DCC and Unreal Editor work followed by visual review.

## Exit criterion

P1 is **source-complete** when the manifest/schema, runtime loader/bridge, tests, production queue report, documentation and CI gate pass without weakening V1–V8.

P1 remains **not art-certified** until actual authored assets are imported and reviewed in Unreal Editor. The next production phase is P2 — Authored Rainforest Asset Pack, which fills the nine-environment-family set and performs the first real procedural-to-authored biome takeover.
