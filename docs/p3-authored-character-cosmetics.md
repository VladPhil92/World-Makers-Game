# P3 — Authored Character & Modular Cosmetics

## Purpose

P3 creates the first reproducible authored source candidate for the World Makers Child Explorer. It advances the V4 procedural character handoff into real skinned source geometry while preserving every gameplay authority already established by the runtime.

P3 is deliberately source-complete before it is native/art-certified. The source bundle, rig, weights, LODs and DCC/import automation can be proven in CI without pretending that final Unreal binary assets have already passed deformation and device review.

## Character contract

The authored candidate preserves the V4 silhouette exactly:

- 158 cm base body envelope;
- five-head stylized child proportion;
- neutral-child base silhouette;
- +Z up and +X forward source coordinates;
- `ACharacter.GetMesh()` remains the authored body destination;
- `ACharacter.CapsuleComponent` remains gameplay collision authority;
- root motion remains disabled by default.

The source generator creates a low-poly child body with readable head, hands and feet rather than a cube/block avatar. It is designed for tablet-distance legibility and for later art refinement without changing the rig contract.

## 19 joints

The rig preserves the 19 joints defined by V4:

`root`, `pelvis`, `spine`, `chest`, `neck`, `head`, `jaw`, `upperarm_l`, `lowerarm_l`, `hand_l`, `upperarm_r`, `lowerarm_r`, `hand_r`, `thigh_l`, `calf_l`, `foot_l`, `thigh_r`, `calf_r`, `foot_r`.

The generated source stores explicit rest positions and normalized skin influences. Every skinned source vertex uses between one and four influences, with four remaining the hard ceiling.

## Eight customization slots

P3 implements all eight customization slots from V4:

- `body`;
- `hair`;
- `top`;
- `bottom`;
- `footwear`;
- `head-accessory`;
- `back-accessory`;
- `hand-prop`.

`body` is the primary Skeletal Mesh. Hair, top, bottom and footwear are authored as skinned modular followers intended to use a shared Skeleton and leader pose. Head/back/hand accessories are independent Static Mesh payloads bound to the stable V4 sockets.

The first authored cosmetic family is intentionally small: one hairstyle, one top, one bottom, one footwear set, one head accessory, one back accessory and one hand prop. P3 proves the production system, not a store catalogue.

## Real source geometry

`scripts/generate-p3-character-source.py` produces `WM_ChildExplorer_P3.source.json`.

This is not an empty manifest. The generated bundle contains:

- vertex positions;
- triangle indices;
- UV0;
- source normals;
- stable material roles;
- the full joint hierarchy and rest locations;
- normalized per-vertex skin weights;
- authored LOD geometry for every module.

The canonical SHA-256 in `content/visual/authored/character-p3-source-pack.json` pins the complete generated bundle. Repository Quality regenerates it twice and requires byte-identical output.

## LOD strategy

The body, top, bottom and footwear each provide four authored source LODs. Hair provides three. Small socket accessories provide two.

LOD geometry is generated independently and must strictly reduce triangle count at every level. The base body remains far below the V4 high ceiling of 20,000 triangles and the lowest source LOD remains below the 6,500-triangle low-tier ceiling.

The purpose is not to maximize polygon count. It is to establish a stable deformation-ready silhouette with enough geometric hierarchy for later art polish.

## Blender bridge

`scripts/blender/build-p3-character-fbx.py` converts the source bundle into a native DCC scene:

1. creates the 19-bone armature;
2. recreates every module and authored LOD;
3. creates UV0 and material assignments;
4. creates Blender vertex groups from the canonical skin weights;
5. binds skinned modules to the common armature;
6. exports per-module, per-LOD FBX payloads without animation.

The Blender export is a conversion step. The deterministic JSON source remains the canonical P3 geometry contract.

## Unreal import bridge

`scripts/unreal/import-p3-character-assets.py` is executed inside Unreal Editor.

The intended native path is:

`P3 source → Blender armature/meshes → FBX per LOD → Unreal Skeletal/Static Mesh import → native report → human deformation review`.

The body is imported first so Unreal creates the production Skeleton. Skinned cosmetic modules then reuse that Skeleton. Small accessories are imported as Static Meshes. The native report records imported LOD contracts and explicitly records that `authoredPresent` was not mutated.

The native importer may create a Physics Asset during body import, but Physics Asset quality, socket alignment, IK Rig construction/validation, foot placement, deformation at joints and material/shading quality remain human/native review items.

## Runtime modular contract

V4 already gives the production body a stable takeover path through `ACharacter::GetMesh()`. P3 does not move collision or movement authority into the visual mesh.

The modular authored plan uses:

- body: `GetMesh()`;
- hair/top/bottom/footwear: shared Skeleton + leader pose;
- head accessory: `socket_head`;
- back accessory: `socket_back`;
- hand prop: `socket_hand_r`.

Until native assets are approved, the procedural fallback remains authoritative for the visible character.

## Privacy and child safety

P3 uses stable cosmetic IDs and palette intent only. It does not introduce:

- biometric capture;
- child photo avatar generation;
- face training;
- voice training;
- identity reconstruction.

The source avatar is an original stylized child design and is not derived from a real child's photograph or biometric template.

## Native `.uasset` boundary

P3 does **not** claim that final native `.uasset` files already exist or are production-certified.

The P1 `character.player.child-explorer` entry must remain `authoredPresent=false` until all of the following have passed on the native Unreal 5.8.2 workflow:

- body and modular imports;
- Skeleton hierarchy review;
- skin deformation review at shoulder, elbow, wrist, hip, knee, ankle, neck and jaw;
- Physics Asset review;
- IK Rig / retarget-chain review;
- socket alignment for head/back/hands;
- material and UV inspection;
- animation compatibility with V5;
- representative-device review.

Only after that evidence exists may the authored body takeover be deliberately activated.

## Acceptance boundary

P3 source-complete means:

- deterministic real source geometry exists;
- all 19 joints exist with stable hierarchy;
- all eight customization slots exist;
- skin weights are normalized and bounded to four influences;
- authored LODs satisfy V4/P1 budgets;
- Blender and Unreal conversion paths exist;
- Repository Quality passes;
- privacy and procedural fallback contracts remain intact.

It does not mean native/art certification has passed.

## Next phase — P4

P4 owns authored animation, VFX and presentation. For the character specifically, P4 will bind the production `ABP_WM_ChildExplorer`, locomotion clips and interaction montages to the P3 Skeleton, then validate the authored character under the V5 read-model and the later device-certification path.
