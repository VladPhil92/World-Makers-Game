# V2 — Materials & Surface Language

## Purpose

V2 establishes the material grammar that makes World Makers read as one authored stylized world before V3 replaces the remaining primitive environment meshes.

The key rule is: **one visual language, few master materials, many controlled instances**. Asset families must not grow bespoke shaders simply because they are authored by different people or produced in different DCC tools.

V2 is designed for a tablet-first rendering floor. Higher tiers may add fidelity only when the same scene remains readable and the lower tier still feels intentional.

## What is executable now

The current Caribbean Rainforest still uses `/Engine/BasicShapes/*`, but V2 no longer renders every proxy as the same default grey material.

`UWMStylizedSurfaceLibrary` resolves stable surface roles into:

- base color;
- roughness;
- metallic intent;
- emissive strength;
- wind response;
- opacity intent;
- two-sided intent;
- translucency intent.

During the proxy phase, `/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial` is used as a visible fallback and receives the resolved `Color` parameter. At the same time, the complete surface state is written to Custom Primitive Data so production materials can consume the same contract without changing gameplay or environment code.

## Stable surface roles

The first material vocabulary is intentionally small:

1. `ground-earth`
2. `terrain`
3. `bark`
4. `foliage`
5. `stone`
6. `water`
7. `build-neutral`
8. `build-eco`
9. `preview-valid`
10. `preview-invalid`
11. `magical-accent`

A new material should normally be an instance/role variation, not a new shader family.

## Custom Primitive Data contract

The production master material contract is versioned in `content/visual/materials/surface-language-v2.json`.

| CPD index | Meaning |
|---:|---|
| 0–3 | BaseColor RGBA |
| 4 | Roughness |
| 5 | Metallic |
| 6 | EmissiveStrength |
| 7 | WindResponse |
| 8 | OpacityIntent |
| 9 | SurfaceRoleNormalized |

These indices are a compatibility contract. Reordering them requires a versioned migration rather than an opportunistic material edit.

## Master material family

### `M_WM_MasterSurface`

Expected production path:

`/Game/WorldMakers/Materials/M_WM_MasterSurface`

Intended uses: earth, terrain, bark, stone, foliage variants, construction pieces and fantasy accents.

The authored graph should support:

- CPD-driven tint and surface response;
- optional stylized BaseColor texture;
- optional restrained normal detail;
- packed mask texture;
- macro color variation that does not create visual noise;
- vertex/per-instance variation where useful;
- masked foliage path rather than uncontrolled translucent leaf cards;
- optional emissive accent without bloom-dependent readability.

The Low tier should prefer opaque or masked shading. Expensive effects must not be required for the art direction to work.

### `M_WM_Water`

Expected production path:

`/Game/WorldMakers/Materials/M_WM_Water`

The water language should emphasize readable shape and color before realistic refraction. It must support shallow-edge readability, a calm river state and later scientific/fantastic state variation.

Until representative hardware proves a translucent implementation, **an intentional opaque stylized fallback is valid for Low tier**. Water must not be the reason the base visual profile misses its frame budget.

## Foliage

Foliage receives explicit wind and two-sided intent through the surface role. Production foliage should use authored silhouettes and restrained alpha coverage; dense overlapping transparent cards are not acceptable as the primary richness strategy.

Wind should be readable at canopy level without making the scene visually unstable. Small leaf micro-motion is secondary to the large-form silhouette.

## Construction materials and placement feedback

Placed pieces use `build-neutral` or `build-eco` according to their semantic category.

Placement preview uses `preview-valid` and `preview-invalid`, but color is supplemental. The existing custom-depth stencil remains authoritative:

- valid = stencil `1`;
- invalid = stencil `2`.

This keeps placement state independently representable through outline/icon/shape treatment and avoids a color-only interaction contract.

## Texture and shader budgets

The visual profile now carries per-tier limits.

| Tier | Max material slots | Max texture edge | Max sampled textures/material |
|---|---:|---:|---:|
| Low | 2 | 1024 px | 6 |
| Mid | 2 | 2048 px | 8 |
| High | 2 | 2048 px | 10 |

These are source-side ceilings, not evidence of measured GPU cost. V8 must replace assumptions with representative-device captures.

### Texture strategy

Baseline reusable architecture/prop texel density: approximately **256 px/m**.

Hero/story assets may reach **512 px/m** only where the visual gain survives tablet viewing distance and the texture budget remains acceptable.

Preferred packed masks:

- R = ambient occlusion;
- G = roughness;
- B = material/variation mask;
- A = reserved.

Trim sheets and atlases should be favored for repeated architectural families rather than one unique texture set per build piece.

## What V2 does not claim

This phase does **not** claim that `M_WM_MasterSurface.uasset` or `M_WM_Water.uasset` has been authored and render-certified. Binary Unreal material assets require Unreal Editor/native asset production. The source contract and proxy fallback are complete enough for V3 code/mesh integration, but art certification remains separate.

It also does not claim:

- final foliage opacity/overdraw performance;
- final water refraction/translucency performance;
- production texture compression measurements;
- final material instance library;
- final authored environment meshes.

Those require the native Unreal/DCC production path and representative-device evidence.

## V2 exit condition

V2 is source-complete when:

- stable surface roles exist in code and a machine-readable handoff contract;
- rainforest proxies visibly receive semantic surface colors;
- material response data is emitted through stable Custom Primitive Data indices;
- building and Eco pieces receive different surface identities;
- valid/invalid placement feedback retains non-color stencil identity;
- water and foliage carry explicit specialized intent;
- material/texture budgets are profile-controlled and tested;
- Repository Quality enforces the V2 contract.

Native art approval remains blocked until the authored master materials are created and visually certified in Unreal.
