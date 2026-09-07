# Contributing to World Makers

## Workflow

1. Update local `main` and create a short-lived branch.
2. Use `feat/`, `fix/`, `art/`, `content/`, `docs/`, or `chore/` prefixes.
3. Keep each PR focused on one change domain when practical.
4. Run relevant validation before requesting review.
5. Never commit credentials, private child data, production exports, or consent records.

## Commit convention

Use Conventional Commits: `type(scope): imperative summary`.

Examples:

- `feat(building): add grid snap preview`
- `content(math): add measurement mission draft`
- `art(jungle): import optimized palm set`
- `fix(parent-portal): mask child profile identifiers`

## C++

- Follow Epic/Unreal naming and coding conventions.
- Prefer Unreal reflection types and lifecycle patterns where engine integration is required.
- Keep gameplay systems modular and testable; avoid monolithic Actor classes.
- Document security/privacy-relevant network RPCs and authority assumptions.

## Blueprints

- Prefix assets by type and feature.
- Keep event graphs small; extract reusable logic into functions/components.
- Avoid hard object references across unrelated systems where soft references or data assets are appropriate.
- Blueprint-only gameplay logic that becomes performance-critical should be profiled before moving to C++.

## Asset naming

Pattern: `PREFIX_Feature_Descriptor_Variant`.

Common prefixes: `BP_`, `WBP_`, `SM_`, `SK_`, `M_`, `MI_`, `T_`, `A_`, `SFX_`, `MUS_`, `DA_`, `DT_`, `LV_`.

Examples: `SM_Jungle_Palm_A`, `T_Jungle_Palm_BaseColor`, `BP_Building_PlaceableBlock`, `DA_Mission_Math_Measure01`.

Keep Unreal content under `/Game/WorldMakers/<Domain>/...` and avoid dumping assets directly at the project root.

## Art optimization

Tablet performance is a first-class constraint. Every imported art set must define expected LOD strategy, texture resolution budget, material complexity, collision approach, and device quality tier. Nanite/Lumen usage must always have scalable fallbacks where target hardware requires them.

## Pedagogical content

Mission source definitions under `content/` must include learning objectives, target age band, evidence of learning, difficulty, localization keys, safety review status, and pedagogy review status. Cultural content requires provenance and sensitivity review.

## Reviews

- Gameplay/code: engineering review.
- Art/content imports: art/technical-art review.
- Pedagogical missions: pedagogy/content review.
- Child-data, auth, telemetry, multiplayer permissions: security/privacy review.
