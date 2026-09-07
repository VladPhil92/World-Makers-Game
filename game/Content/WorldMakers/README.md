# Unreal Content Layout

Create Unreal assets under `/Game/WorldMakers` using domain folders such as:

```text
Characters/
Building/
Biomes/
Missions/
UI/
Audio/
Materials/
FX/
Data/
Developer/
```

Use `PREFIX_Feature_Descriptor_Variant` naming. Track `.uasset` and `.umap` through Git LFS. Never place real child/user data, credentials, production exports, or legal consent artifacts in Unreal Content.

Recommended common prefixes: `BP_`, `WBP_`, `SM_`, `SK_`, `M_`, `MI_`, `T_`, `A_`, `SFX_`, `MUS_`, `DA_`, `DT_`, `LV_`.
