# M1.8 — Visual Proof & Micro Vertical Slice

## Purpose

M1.8 gives World Makers a reproducible first visual composition layer without conflating source-controlled proxy geometry with production art. It exists to test scale, silhouette hierarchy, environmental density, camera readability and tablet-oriented scene structure while Issue #9 still blocks native UE certification.

## Micro slice

`AWMCaribbeanRainforestPrototype` creates a deterministic Caribbean Rainforest composition from Unreal Engine primitive meshes. It uses hierarchical instancing for repeated environment families and preserves a quiet central clearing for construction.

Environment archetypes:

1. broad build clearing / ground plane;
2. flattened terrain mounds;
3. tree clusters composed from trunk + canopy instances;
4. asymmetric rock clusters;
5. water-edge markers defining one strong scene boundary.

The default seed is `17062026` so screenshots, tests and later profiling can compare the same spatial composition unless a test intentionally changes the seed.

## Visual profile

`UWMVisualProfileSettings` stores:

- the `CaribbeanRainforestPrototype` profile name;
- prototype palette roles;
- Low / Mid / High quality tiers;
- target FPS and scene-density budgets.

The initial tier intent is:

- **Low:** conservative 30 FPS tablet composition proof;
- **Mid:** default 30 FPS tablet composition proof;
- **High:** 60 FPS desktop/high-device composition proof.

These counts are not final polygon, memory, texture, shader or draw-call budgets. Those must be measured on representative devices.

## Character proxy

The original cylinder + sphere avatar is replaced by a six-part neutral proxy: torso, head, two arms and two legs. The objective is to evaluate child-like silhouette, camera scale and environment proportion without introducing gender markers or pretending that skeletal animation/customization already exists.

## Cultural and environmental boundary

Selva Caribeña remains an ecosystem/visual proof. Cultural architecture, sacred motifs, community-specific symbols, Indigenous/Afro-Caribbean representation or historical reconstruction must not be inferred from generic references. Those additions require documented provenance and expert/community review.

## Production-art handoff

The proxy becomes a real visual slice only after Unreal Editor can author and version:

- a parameterized base material and material instances;
- authored vegetation/rock/terrain mesh families;
- LOD/HLOD/cull settings;
- texture/trim-sheet strategy;
- final lighting/scalability profiles;
- avatar skeletal mesh and animation set;
- an authored `.umap` certification level.

## Certification state

**VISUAL SOURCE PROOF / RUNTIME + ART CERTIFICATION BLOCKED**

Issue #9 remains the hard blocker for native UE 5.8.2 build/test evidence and authored-map certification. M1.8 additionally requires real-device visual/performance profiling before any tablet-quality claim is valid.
