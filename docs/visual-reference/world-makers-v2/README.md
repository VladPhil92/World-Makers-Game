# World Makers — Approved Visual Reference Set v2

This directory supersedes `docs/visual-reference/world-makers-v1` as the current approved visual-direction reference for World Makers. It contains the user-approved image-generation outputs at their **original resolution and format**, committed byte-for-byte with no recompression, re-encoding, upscaling, downscaling or crop.

These files are **reference images only**. They are not runtime Unreal assets, are not evidence that equivalent native assets exist in `/Game/WorldMakers/...`, and must not be copied into the packaged build without authored production, provenance review, optimization and native approval. See `SourceArt/WorldMakers/README.md` for the source-art / runtime-asset boundary.

## Why v2 exists

`world-makers-v1` stored **documentation-optimized WebP derivatives** (deliberately downsized, e.g. 120–360px wide) so approved direction could be versioned without treating concept imagery as production art. `world-makers-v2` is a different, newer approved generation batch, delivered directly at full resolution. Because the website (`worldmakers.ctgone.com`, repository `ctg_one_website`) is contractually required to present World Makers concept art **without any lossy compression or Next.js image re-optimization**, these masters are committed here unmodified and served to the site via `raw.githubusercontent.com`.

`v1` is kept for historical traceability; it is not deleted. Do not mix asset identities across the two sets — `v1` and `v2` are separate approved batches, not revisions of the same images.

## Reference index

| File | Content | Pixel dimensions | SHA-256 |
|---|---|---:|---|
| `logo-primary.png` | World Makers wordmark lockup, globe mark, primary public logo (transparent background) | 1774×887 | `acc31c94f351ad0af8204447c85ae03fd4b1f48280bfadc7d6a9c2dd4917b706` |
| `hero-banner.png` | Spanish marketing hero: "Crea, explora y aprende en un mundo vivo", primary CTAs, main explorer character | 1672×941 | `54e2bba30f01009e6e2acdbb0ef9be1faf6fd1dc9eaec4147e4c6aaee6ddda0d` |
| `gameplay-overview.png` | Four-panel gameplay composite: first-person exploration, building mode, science mode, mission screen | 1536×1024 | `096f46abcff8c9af44f96c6b6cd51607e72c9b4e3967bd099e0f1229652de9c3` |
| `gameplay-science.png` | First-person HUD screenshot: River Renewal Project mission with Chemistry / Physics / Biology / Botany panels | 1672×941 | `6e8aa993796dc241db52bf706f3b1e1b959ce8e691c410c1a62e628ba8eae1e0` |
| `gameplay-build.png` | First-person build-mode screenshot: Eco-Research Module, material inventory, snap-to-connector UI | 1672×941 | `31e6bb072af775081602cb304d8ed5a4356167539aae8dec09cea473af0d4181` |
| `universe-overview.png` | Explorer + companion overlooking the World Makers universe, HUD/quest/build-category overlay | 1024×1536 | `e4aed835fc09b721634a1a32b06cd6c01a5a331d468807b414a32bbdaca4a5f7` |
| `visual-identity-guide.png` | Brand guide sheet: color palette, shape language, design-style summary | 1024×1536 | `dc9a7f162903d5fd44989d9e060ffc9143fde2b0223e097f84840962522b4f73` |
| `forms-style-policy.png` | Shape/style policy sheet: do/don't boundary vs. voxel-block aesthetics, character style, silhouette references | 1312×1199 | `8082c4f66bb199e9b9b002aba80e42b4b3ab9f031292e442410c9d24ffdcdd75` |
| `character-01-curious-explorer.png` | Main explorer archetype — yellow jacket, compass badge, warm orange/blue equipment language | 1086×1448 | `8a65096c86db99516e4a24d3eab28a506f2a2f9521a038d0b9a12856094dfebb` |
| `character-02-scientist-inventor.png` | Scientist/inventor archetype — cyan/blue tech gear, field tablet, ponytail | 1024×1536 | `a2741007b01455e823fe4a0c6c55b6cc17284e649d5a9603fc181780807399a7` |
| `character-03-nature-guardian.png` | Nature-guardian archetype — green eco gear, leaf motifs, ecology accent language | 1086×1448 | `38a645775b6ba8269815dcd0cbe67df5ebeb81cb3d261fcb5ba2dd2a3930044a` |
| `character-04-luna-explorer.png` | Complementary explorer archetype — purple/lavender jacket, aviator goggles, adventure backpack | 1086×1448 | `301a9fcbf880b6b92b2e799a834a090c49bbd57d0b1b600274e4756ad6dbc285` |

## Storage note

`.gitattributes` in this directory unsets the repository-wide Git LFS filter for `*.png` so these files are stored as plain Git blobs. This is intentional: `raw.githubusercontent.com` serves LFS-tracked paths as pointer files, not image bytes, which would break direct consumption by the website's live visual proxy. Do not remove this override without re-wiring the consuming API route (`ctg_one_website` repository, `src/app/api/worldmakers/visuals/[asset]/route.ts`) to resolve LFS objects first.

## Use rule

Treat the images as a **north-star visual specification** for direction, palette, UI language, mission structure and character archetypes. Production art and UI must preserve the principles while generating original meshes, materials, silhouettes, icons, animation curves, HUD components and compositions — these PNGs are marketing/concept references, not shippable interface assets.
