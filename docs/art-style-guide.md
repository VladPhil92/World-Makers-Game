# Art Style Guide

## Direction

World Makers should feel like an original **premium stylized 3D world**: readable silhouettes, warm materials, expressive environments, and enough fidelity to feel richer than voxel construction games while remaining scalable to tablets.

References such as stylized Nintendo/open-world titles may guide quality discussions but must not be copied. Define an original shape language, palette, proportions, material treatment, UI language, and character identity.

## Visual pillars

- Friendly and legible at small tablet sizes.
- Strong silhouette hierarchy.
- Soft stylization over photorealism.
- Modular construction pieces that still feel authored.
- Environmental storytelling without visual clutter.
- Inclusive avatar customization.

## Naming

Use `PREFIX_Feature_Descriptor_Variant`.

Examples: `SM_Jungle_Palm_A`, `M_Building_Wood`, `T_Building_Wood_BaseColor`, `MI_Building_Wood_Warm`, `A_Avatar_Wave`.

## Source vs game-ready assets

- Source DCC files may live in version control through Git LFS when they are necessary for reproducibility.
- Imported Unreal assets live under `/Game/WorldMakers/...`.
- Do not mix marketplace/vendor source content with custom assets without documenting license/provenance.

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

### Initial placeholder budgets

Do **not** treat these as final production limits. Profile representative devices first.

- Favor 1–2 material slots for common static props.
- Use texture atlases/trim sheets where they improve batching and memory.
- Reserve high-resolution textures for hero assets with measurable benefit.
- Build explicit LOD/HLOD strategy even when high-tier Nanite is enabled.
- Avoid uncontrolled translucent/overdraw-heavy foliage and VFX.

## Palette

[PLACEHOLDER: primary palette, biome palettes, accessibility contrast rules]

## Character proportions

[PLACEHOLDER: age-neutral stylized proportions, face system, skin/hair/clothing system, animation language]

## Caribbean Rainforest vertical slice

[PLACEHOLDER: mood board, vegetation families, architecture/material references, water/sky treatment, cultural/environmental source list]
