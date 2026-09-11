# World Makers — First-Person Authored Asset Production v1

## Purpose

This phase converts the source-proxy First-Person Interaction Kit into a production-art pipeline without granting presentation code gameplay authority.

The production target is a coherent authored set:

- `SK_WM_FirstPersonArms`;
- `SM_WM_Scanner`;
- `SM_WM_BuildTool`;
- `SM_WM_MeasureTool`;
- `SM_WM_WristDevice`;
- nine first-person animation clips.

The authored path is **all-or-proxy**. Partial takeover is intentionally rejected so the player never sees a hybrid such as authored arms with a basic-shape scanner.

## Source art

`scripts/generate-first-person-authored-source.py` produces a deterministic DCC-neutral JSON bundle containing:

- explicit vertices and triangle indices;
- UV0 coordinates;
- explicit normals;
- skin weights for a seven-bone first-person arm rig;
- three authored LOD source meshes for every visual asset;
- material intent;
- nine 30 fps animation definitions with bounded hand/root keys.

Coordinates use centimeters with `+X` forward, `+Y` right and `+Z` up.

The seven stable bones are:

`root`, `upperarm_l`, `lowerarm_l`, `hand_l`, `upperarm_r`, `lowerarm_r`, `hand_r`.

The first-person rig is deliberately separate from the full third-person Child Explorer skeleton. It exists to optimize camera-space arms and tool interactions while preserving the third-person avatar contract.

## DCC handoff

`scripts/blender-build-first-person-authored.py` consumes the generated bundle and exports:

- `SK_WM_FirstPersonArms_LOD0..2.fbx`;
- three FBX LODs for scanner/build/measure/wrist assets;
- one animation FBX for every stable first-person action ID.

The generated geometry is a production starting point, not a claim that final sculpt/material polish is complete. An artist can refine silhouettes, topology and surface treatment while preserving the stable skeleton, asset paths, slot IDs and animation IDs.

## Unreal import

`scripts/unreal-import-first-person-authored.py` imports the authored set to the stable paths already reserved by the interaction kit. It writes `Build/FirstPersonAuthored/native-import-report.json`.

The importer intentionally does **not** mutate `authoredPresent`. Import success and product approval are separate states.

Native review must still verify:

- all three LODs import correctly;
- no critical wrist/elbow deformation;
- no tool/hand interpenetration in the nine clips;
- no first-person clipping through the near plane;
- scanner/build/measure silhouettes remain legible on tablet screens;
- owner-only visibility and collision-free render behavior;
- Reduced Motion presentation remains comfortable;
- V8/P5 device budgets remain satisfied.

## Runtime takeover

`UWMFirstPersonAuthoredBridgeSubsystem` watches the existing first-person interaction component. The bridge never changes movement, build validity, science, mission progress or rewards.

Authored takeover requires both:

1. all five visual assets + all nine animations loaded successfully; and
2. explicit launch with `-WMEnableFirstPersonAuthored`.

If either condition is false, the source-proxy kit remains visible.

This explicit launch switch makes native art review reversible and prevents freshly imported assets from silently becoming the shipping presentation path.

## Animation behavior

The authored animation set preserves the stable vocabulary introduced by the source-proxy phase:

`tool-raise`, `tool-lower`, `scan-anticipate`, `scan-hold`, `scan-settle`, `build-point`, `build-confirm`, `measure-focus`, `observe-focus`.

`scan-hold` is the only looping action. Root motion remains disabled. Character locomotion continues to be controlled by `CharacterMovementComponent`.

## Certification boundary

CI can validate deterministic source geometry, UVs, normals, weights, LOD structure, animation IDs/durations, stable Unreal paths, all-or-proxy behavior and the presence of the DCC/native pipeline.

CI without Blender + Unreal cannot certify final `.uasset` quality. The manual workflow `First-Person Authored Native Integration` requires a self-hosted Windows/X64 Unreal runner with `BLENDER_EXE` and `UE_EDITOR_CMD` configured.

A successful native import report is still **not** final art certification. Human deformation/camera review and representative tablet evidence remain mandatory before `authoredPresent` may be changed to true in a reviewed activation commit.
