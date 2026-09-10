# V4 — Character Art & Rig

## Purpose

V4 moves the player from a six-piece Engine-primitive silhouette to a source-visible, original stylized child avatar with a stable rig contract. It prepares the production Skeletal Mesh and V5 authored-animation path without changing gameplay collision or movement authority.

## Visual direction

The default `ChildExplorerV1` profile is exactly 5 heads tall at 158 cm. The silhouette uses a compact torso, enlarged head/hands and readable feet so body language remains legible on tablets. The base silhouette has no gender marker; expression comes from hair, clothing, color and optional accessories.

The source-visible avatar is built from original low-poly procedural geometry: tapered torso, faceted head, clustered hair, segmented upper/lower arms, hands, thighs, calves and feet. It does not use `/Engine/BasicShapes/*` for visible character geometry. `BasicShapeMaterial` remains only a temporary colorable material fallback.

## Rig contract

The V4 contract exposes 19 stable joints:

`root`, `pelvis`, `spine`, `chest`, `neck`, `head`, `jaw`, `upperarm_l`, `lowerarm_l`, `hand_l`, `upperarm_r`, `lowerarm_r`, `hand_r`, `thigh_l`, `calf_l`, `foot_l`, `thigh_r`, `calf_r`, `foot_r`.

The procedural art hierarchy mirrors these major transforms. V5 may animate the source hierarchy for fallback proof, but production animation is expected to target an authored Skeleton using the same semantic chains.

## Customization slots

Stable slots are `body`, `hair`, `top`, `bottom`, `footwear`, `head-accessory`, `back-accessory` and `hand-prop`. Socket intent is defined for head, back and both hands. Cosmetic choices must not change the `ACharacter` gameplay capsule.

V4 stores appearance intent as stable cosmetic IDs and palette data. It does not introduce biometric capture, child-photo avatar generation, face training or voice training.

## Visual-path selection

`ACharacter::GetMesh()` remains the production `USkeletalMeshComponent` destination. The source-controlled default uses procedural avatar art while the authored Skeletal Mesh is absent. Once a production mesh is assigned and the procedural preference is disabled, `RefreshAvatarVisualPath()` selects the Skeletal Mesh path without changing `CharacterMovementComponent`, the capsule, missions, building or interaction systems.

The legacy six-piece V1 avatar remains hidden as a fail-safe only.

## Animation handoff

V1's bounded procedural gait now drives V4 rig pivots when the procedural avatar is active: root lean/bob, head bob, opposing upper-arm swing and thigh swing. This is still a motion proof, not V5 production animation.

V5 owns authored locomotion, start/stop, turn, jump/fall/land, interaction montages, additive look/aim, secondary motion and the production Animation Blueprint/state machine.

Root motion is disabled by default because gameplay movement authority remains `CharacterMovementComponent`.

## Authored asset targets

The machine-readable handoff reserves:

- `/Game/WorldMakers/Characters/Player/SK_WM_ChildExplorer`
- `/Game/WorldMakers/Characters/Player/SKEL_WM_ChildExplorer`
- `/Game/WorldMakers/Characters/Player/ABP_WM_ChildExplorer`
- `/Game/WorldMakers/Characters/Player/IKR_WM_ChildExplorer`
- `/Game/WorldMakers/Characters/Player/PHYS_WM_ChildExplorer`

These paths are contracts, not claims that binary `.uasset` files already exist.

## Budgets

Low targets at most 6,500 triangles, 48 bones, 4 skin influences and 3 material slots. Mid targets 12,000 triangles, 64 bones, 4 influences and 3 slots. High targets 20,000 triangles, 96 bones, 4 influences and 4 slots. The procedural fallback is intentionally far below these ceilings.

These are source production ceilings. V8 must replace assumptions with measured GPU/CPU, memory, skinning and draw-call evidence on representative devices.

## Acceptance boundary

V4 is source-complete when the procedural avatar, 19-joint contract, customization/socket manifest, quality budgets, visual-path selection, automation tests and Repository Quality gate pass.

V4 does **not** claim that the final DCC-authored Skeletal Mesh, UVs, skin weights, facial shapes, modular clothing assets, physics asset or IK rig have been created or visually approved. Those binary/native deliverables still require Blender/Maya and Unreal Editor production plus native certification.
