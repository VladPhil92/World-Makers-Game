# World Makers — Native Unreal Readiness Gate

Status: pre-Unreal hardening contract
Baseline: Unreal Engine 5.8.2 / Win64 / Development

## Purpose

This gate is the controlled handoff between repository/source hardening and authored Unreal Editor production. It exists so that opening the project in Unreal is not treated as a speculative debugging step.

A workstation is **pre-editor native ready** only when the repository is current and clean, Git LFS is functioning, the locked engine version matches, hosted-equivalent source preflight passes, and `WorldMakersEditor Win64 Development` compiles successfully.

A workstation is **authored-map certification ready** only when the same checks pass and `WM_PrototypeCertification.umap` exists at the canonical path. Full automation can then be executed as an explicit second gate.

These states do not imply representative-device certification.

## One-command entry point

From the repository root on Windows PowerShell:

```powershell
$UE = "C:\Program Files\Epic Games\UE_5.8"
.\scripts\run-unreal-readiness-gate.ps1 -EngineRoot $UE -CleanIntermediate
```

The default mode is deliberately strict:

- current branch must be `main`;
- local `HEAD` must equal freshly fetched `origin/main`;
- the worktree must be clean;
- Git and Git LFS must be available;
- `git lfs pull` must succeed;
- repository baseline must remain UE 5.8.2;
- the installed engine must report UE 5.8.2;
- `validate-unreal-source-preflight.py` must pass;
- `WorldMakersEditor Win64 Development` must compile successfully.

Temporary diagnostic exceptions exist through `-AllowNonMain` and `-AllowDirtyWorktree`, but they must not be used as certification evidence.

## Post-map certification mode

After the first Unreal-authored certification map has been created and committed through Git LFS:

```powershell
$UE = "C:\Program Files\Epic Games\UE_5.8"
.\scripts\run-unreal-readiness-gate.ps1 `
  -EngineRoot $UE `
  -RequireAuthoredMap `
  -RunAutomation `
  -CleanIntermediate
```

This requires the exact file:

`game/Content/WorldMakers/Maps/WM_PrototypeCertification.umap`

and then executes the complete `WorldMakers.` automation namespace through the existing native test script.

## Evidence

The gate writes evidence under:

`artifacts/unreal-readiness/`

The primary record is `readiness-result.json`. It contains only engineering state:

- repository commit and branch;
- fetched `origin/main` commit;
- expected and actual Unreal versions;
- source-preflight state;
- native build state;
- optional automation state;
- blocking reasons.

It must not contain player/guardian identifiers, child-authored text, learning evidence, account data, precise location, commerce state, or behavioral telemetry.

The existing `build-unreal.ps1` and `test-unreal.ps1` logs/results remain the authoritative lower-level native records.

## Fail-closed behavior

The gate blocks rather than guessing when any of the following cannot be established:

- source freshness;
- clean working state;
- LFS integrity;
- exact engine version;
- source preflight;
- native compilation;
- authored-map presence when requested;
- native automation when requested.

A `ready` result therefore means only that every requested gate passed. It does **not** mean the game is production-ready or device-certified.

## First Unreal Editor task

Once pre-editor native readiness is green, the first editor-only deliverable is not broad environment production. It is the deterministic certification level:

`/Game/WorldMakers/Maps/WM_PrototypeCertification`

Minimum content:

1. deterministic `PlayerStart`;
2. minimal authored collision/ground geometry;
3. project `WMGameMode` active;
4. enough authored geometry to test build placement without relying exclusively on runtime-generated ground;
5. stable save path and repeatable smoke route.

The map must be committed as `.umap` through Git LFS before it can satisfy the certification-mode gate.

## Native smoke sequence after the map exists

The first manual/native route should cover:

1. project load with no fatal asset/class errors;
2. movement and camera;
3. valid building placement;
4. invalid overlap rejection;
5. rotate/move/remove;
6. undo/redo;
7. save/load;
8. rainforest observation/interaction route;
9. child journey UI open/close/continue;
10. one representative mission/epic route without spoofed progression.

Only after this route and `WorldMakers.*` automation are clean should broad authored character, environment, animation, Niagara/VFX and cinematic production become the primary development track.

## Relationship to repository gates

- `Unreal Source Preflight`: hosted static regression gate; no engine execution.
- `run-unreal-readiness-gate.ps1`: local/runner orchestration of source freshness + native compile and optional automation.
- `Unreal CI / unreal-build-and-test`: continuous native evidence once the self-hosted UE 5.8.2 runner is provisioned.
- M3/M5 certification assessors: higher-level route/device evidence, not substitutes for compilation.

See also `docs/unreal-readiness-audit.md`, Issue #9, Issue #106 and Issue #108.
