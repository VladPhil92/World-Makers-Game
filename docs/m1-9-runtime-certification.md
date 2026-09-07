# M1.9 — Runtime Certification Infrastructure

> **SOURCE GREEN IS NOT RUNTIME CERTIFIED.**

This phase turns the Unreal certification blocker into an auditable pipeline. It does not manufacture the missing Unreal Editor environment or the binary certification map.

## Required runner

Use a dedicated Windows x64 self-hosted GitHub Actions runner with labels:

- `self-hosted`
- `Windows`
- `X64`
- `unreal`

Install the exact engine version locked by `game/UNREAL_ENGINE_VERSION` (**5.8.2** for the current baseline), Git, and Git LFS.

Configure repository variables:

- `UNREAL_SELF_HOSTED_ENABLED=true`
- `UNREAL_ENGINE_ROOT=<absolute UE 5.8.2 installation root>`

The runner must not hold production secrets or child/user data. It is a build machine only.

## Authored certification map

The runtime gate requires this exact Git LFS asset:

`game/Content/WorldMakers/Maps/WM_PrototypeCertification.umap`

Create it in Unreal Editor 5.8.2. The map should contain:

- a deterministic PlayerStart;
- static authored ground/collision independent of the runtime bootstrap;
- sufficient open space for placement, overlap rejection, move/edit, remove and save/load smoke tests;
- no marketplace/vendor assets unless provenance and license are documented.

`validate-m1-certification.py --require-authored-map` intentionally rejects a different `.umap` name.

## Native pipeline

The `unreal-build-and-test` job performs:

1. runner preflight;
2. exact certification-map validation;
3. `WorldMakersEditor` Win64 Development build;
4. `UnrealEditor-Cmd.exe` automation execution with the complete `WorldMakers.*` test namespace;
5. certification evidence collection;
6. evidence upload with 30-day retention;
7. fail-closed native-evidence assertion.

The full `WorldMakers.*` namespace is required so Building, Input/Touch and Visual/Profile tests participate in native certification.

## Evidence contract

The native job writes under `artifacts/certification/`:

- `runner-preflight.json`
- `build.log`
- `build-result.json`
- `automation.log`
- `automation-result.json`
- `certification-manifest.json`
- optional `manual-smoke.json`

The manifest includes the Git commit, exact Unreal version, SHA-256 of the authored map, evidence hashes and certification state.

### Certification states

- `blocked`: required native evidence is missing or failed.
- `native-pass-manual-smoke-pending`: UE 5.8.2 preflight/build/tests passed, but manual smoke evidence is absent.
- `certified`: native pass plus `manual-smoke.json` with `status: passed`.

The regular Ubuntu source-validation job can never produce the latter two states because it does not run Unreal.

## Manual smoke

After a successful native run, launch the certification map in Unreal Editor/standalone and verify at minimum:

- player spawns at the intended start;
- keyboard/gamepad movement and third-person camera operate;
- touch drag rotates camera on a representative touch device when available;
- valid build placement succeeds;
- invalid overlap is rejected;
- piece cycling and rotation work;
- move/edit can be confirmed and cancelled;
- remove works;
- undo and redo cover place/remove/move;
- save and load reconstruct the world;
- the child-facing build HUD reflects valid/invalid and move/cancel states.

Record the result using `docs/templates/manual-smoke.example.json` as the shape. Do not include child names, accounts, recordings or other PII in certification evidence.

## Closing Issue #9

Issue #9 may close only when all of the following exist:

- the exact `.umap` is committed through LFS;
- the self-hosted runner reports UE 5.8.2;
- `WorldMakersEditor` compiles;
- `WorldMakers.*` completes successfully;
- the uploaded manifest reports `native-pass-manual-smoke-pending` or later;
- manual smoke is completed and evidence is attached/referenced;
- the final manifest can report `certified`.

Until then the product remains **SOURCE COMPLETE / RUNTIME CERTIFICATION BLOCKED**.
