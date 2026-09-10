# Art Style Guide

## Direction

World Makers should feel like an original **premium stylized 3D world**: readable silhouettes, warm materials, expressive environments, and enough fidelity to feel richer than voxel construction games while remaining scalable to tablets.

References such as stylized Nintendo/open-world titles may guide quality discussions but must not be copied. Define an original shape language, palette, proportions, material treatment, UI language, and character identity.

## Visual pillars

- Friendly and legible at small tablet sizes.
- Strong silhouette hierarchy before surface detail.
- Soft stylization over photorealism.
- Modular construction pieces that still feel authored.
- Environmental storytelling without visual clutter.
- Inclusive avatar customization without a gender-default silhouette.
- Color should communicate biome, safety and gameplay state rather than simulate realism for its own sake.

## Naming

Use `PREFIX_Feature_Descriptor_Variant`.

Examples: `SM_Jungle_Palm_A`, `M_Building_Wood`, `T_Building_Wood_BaseColor`, `MI_Building_Wood_Warm`, `A_Avatar_Wave`.

## Source vs game-ready assets

- Source DCC files may live in version control through Git LFS when they are necessary for reproducibility.
- Imported Unreal assets live under `/Game/WorldMakers/...`.
- Do not mix marketplace/vendor source content with custom assets without documenting license/provenance.
- M1.8 Engine-primitives are composition proxies only. They are not production art and should be replaced asset-family by asset-family.

## Tablet optimization rules

Every production asset set must state:

- intended device tier;
- approximate triangle/vertex budget and LOD plan;
- texture sizes and compression intent;
- material slots and shader complexity;
- collision type;
- animation/skeleton implications;
- Nanite eligibility plus non-Nanite fallback where required;
- lighting assumptions and Lumen fallback where required.

### M1.8 prototype density budgets

These are **scene-density test budgets**, not final triangle or texture budgets. Representative hardware profiling must replace them with measured production limits.

| Tier | Target | Tree clusters | Rock clusters | Terrain mounds | Water markers | Purpose |
|---|---:|---:|---:|---:|---:|---|
| Low | 30 FPS | 16 | 8 | 4 | 6 | conservative tablet proof |
| Mid | 30 FPS | 28 | 14 | 6 | 8 | default tablet composition proof |
| High | 60 FPS | 42 | 20 | 8 | 10 | desktop/high-device composition proof |

Use HISM/ISM for repeated environment families. Production foliage must have explicit LOD/cull behavior and must not rely on high-cost translucency or uncontrolled overdraw.

## Prototype palette — Caribbean Rainforest

The first palette is intentionally compact and warm. Hex values are visual-direction anchors and may be tuned after accessibility and device-display testing.

- Canopy Deep — `#184F3A`
- Leaf Bright — `#3E8A57`
- Trunk Warm — `#80502E`
- Earth / Clay — `#B76E4B`
- Water — `#2F8F9D`
- Stone — `#6F7B72`
- Sky — `#8BC7D8`
- Sunlight — warm neutral, approximately `#FFE3B0`
- Build Neutral — warm sand / wood-neutral family
- Build Eco — restrained living green family
- Preview Valid — cyan-teal family, paired with independent stencil identity
- Preview Invalid — amber family, paired with independent stencil identity
- Magical Accent — violet family reserved for fantasy emphasis rather than generic UI decoration

Do not communicate valid/invalid gameplay state using biome color alone. Placement feedback must remain independently readable through shape, iconography, custom-depth/stencil identity and contrast.

## Surface language

V2 establishes one material grammar rather than one bespoke shader per asset family.

Production material families:

- `/Game/WorldMakers/Materials/M_WM_MasterSurface`
- `/Game/WorldMakers/Materials/M_WM_Water`

`M_WM_MasterSurface` is the default authored surface path for earth, terrain, bark, stone, construction and compatible fantasy accents. Foliage remains part of the same visual grammar but may use a masked branch/instance where two-sided shading and wind are needed.

`M_WM_Water` is a specialized water path. It should prioritize readable stylized depth/color and stable gameplay silhouettes before expensive refraction. Low tier may use an intentional opaque fallback until representative-device profiling validates a more expensive path.

The source contract sends material intent through stable Custom Primitive Data: base color, roughness, metallic, emissive strength, wind response, opacity intent and surface-role identity. Do not repurpose those indices casually; changes require a versioned migration.

Texture strategy:

- favor a small reusable set of stylized textures rather than unique 4K sets per object;
- baseline repeated architecture/props around 256 px/m;
- hero/story assets may rise toward 512 px/m only where the gain survives gameplay viewing distance;
- prefer packed masks and trim sheets/atlases for repeated construction families;
- keep material slots at two or fewer in the current tablet-first target;
- avoid parallax/height tricks, dense layered translucency and expensive microdetail as baseline identity.

Surface variation should support silhouette and composition, not fight them. Macro color/roughness variation is preferred to noisy high-frequency detail.

## Character proportions

The target avatar language is child-proportioned, expressive and customizable rather than anatomically realistic.

- approximately 4.5–5 heads tall for the eventual production character;
- slightly enlarged head and hands for tablet readability;
- compact torso and short limbs;
- neutral base body with no forced gender markers;
- gender expression, hair, clothing, skin tone and accessories belong to customization rather than the base silhouette;
- locomotion should emphasize readable anticipation and follow-through over realistic micro-motion.

M1.8 uses a six-part primitive proxy (head, torso, two arms and two legs) to test camera scale and silhouette only.

## Caribbean Rainforest micro vertical slice

### Composition language

The source-controlled M1.8 proof uses five environment archetypes:

1. broad build clearing / ground plane;
2. low terrain mounds framing the playable center;
3. tree clusters with narrow trunks and large rounded canopies;
4. asymmetric rock clusters;
5. a water-edge band that creates a strong boundary on one side of the scene.

The center must remain visually quiet enough for building. Density should increase toward the perimeter so the environment frames rather than obscures construction.

### Cultural boundary

The micro slice is an **environment/ecosystem proof**, not a historical reconstruction. Do not introduce Indigenous, Afro-Caribbean, colonial, vernacular or sacred architectural motifs until provenance is documented and cultural/community review has occurred.

### Production handoff

Before calling the visual slice production-ready, replace proxy families with authored meshes/materials and record:

- source/provenance;
- LOD/HLOD plan;
- texture and material budgets;
- collision policy;
- tablet profiling evidence;
- accessibility contrast review;
- cultural/environmental review where applicable.
