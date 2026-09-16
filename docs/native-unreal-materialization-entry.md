# Native Unreal Materialization Entry

## Purpose

This is the operational bridge from the completed Pre-Unreal architecture into the first real Unreal-authored binary deliverable. It does not introduce another gameplay milestone. It composes the existing Final Pre-Unreal, G1 and G2 gates so the transition into Unreal Engine is deterministic and fail-closed.

The official sequence is:

`PRE_UNREAL_READY -> G1 CERTIFIED -> Unreal authors WM_PrototypeCertification.umap -> native/human review -> Git LFS commit -> G2 certification`

## One-command entry

From a Windows workstation with the locked Unreal Engine 5.8.2 toolchain:

```bat
WorldMakers-NativeMaterialize.cmd
```

The launcher delegates to `scripts/enter-native-unreal-materialization.ps1` and performs the following sequence:

1. fetch `origin/main` and inspect the current repository state;
2. require `artifacts/pre-unreal-handoff/final-pre-unreal-handoff.json` to say `PRE_UNREAL_READY` for the exact current commit;
3. require clean current `main` matching `origin/main` for the official path;
4. execute the existing G1 native certification runner, including real `WorldMakersEditor` build and `WorldMakers.` automation;
5. require G1 `CERTIFIED` on the official path;
6. invoke the existing G2 `-AuthorMap` flow in a child PowerShell process;
7. require Unreal to create/reconcile the real `game/Content/WorldMakers/Maps/WM_PrototypeCertification.umap`;
8. verify that the map is covered by Git LFS;
9. write `artifacts/native-materialization/native-materialization-entry.json`.

The successful entry state is deliberately named:

`AUTHORING_COMPLETE_COMMIT_REQUIRED`

That state is not G2 certification. It means Unreal successfully created or reconciled the real certification map after an acceptable G1 native pass and the binary now requires native review and a deliberate Git LFS commit.

## Why the runner uses child PowerShell processes

The existing G2 authoring runner exits after successful `-AuthorMap` execution. The materialization entry therefore invokes G1 and G2 through isolated `powershell.exe` child processes so their exit codes remain authoritative without terminating the parent orchestration before evidence is written.

## Current external blocker and development override

The official path remains fail-closed on the Final Pre-Unreal handoff. If custom-domain HTTPS or authenticated CTG One E2E evidence is still pending, the runner will stop before native materialization.

Native engineering may still proceed deliberately in parallel for local development with:

```bat
WorldMakers-NativeMaterialize.cmd -AllowPreUnrealBlockedForDevelopment
```

This override is explicitly non-certifying. It does not create `PRE_UNREAL_READY`, does not certify G2 and must not be presented as release evidence. Normal G1 branch/worktree rules still apply; use the existing `-AllowNonMain` or `-AllowDirtyWorktree` switches only when intentionally diagnosing local development state.

The override exists so an external DNS/TLS propagation delay does not prevent engineers from discovering native compiler/runtime problems. Official authored handoff still requires the real Final Pre-Unreal result.

## What Unreal authors

The existing G2 Unreal Python authoring script creates or reconciles:

- `/Game/WorldMakers/Maps/WM_PrototypeCertification`;
- `WM_G2_PlayerStart` using `/Script/Engine.PlayerStart`;
- `WM_G2_Rainforest` using `/Script/WorldMakers.WMCaribbeanRainforestPrototype`;
- `WM_G2_MissionGeometry` using `/Script/WorldMakers.WMMissionGeometryActor`;
- WorldSettings default GameMode `/Script/WorldMakers.WMGameMode`.

The `.umap` is produced and saved by Unreal Engine. Source tooling must never manufacture a text or placeholder file with a `.umap` extension.

## Required work after AUTHORING_COMPLETE_COMMIT_REQUIRED

Open `WM_PrototypeCertification` in UE 5.8.2 and review the actual route before committing it. The next work is deliberately asset/runtime work rather than new architecture:

1. verify spawn, first-person movement/camera and collision;
2. inspect the authored rainforest/mission geometry actors;
3. import and visually review the nine P2 Caribbean Rainforest asset families;
4. set `authoredPresent=true` only after each required asset is genuinely loadable and approved;
5. verify `git lfs status` and commit the real `.umap`/`.uasset` binaries through Git LFS;
6. exercise the G2 route: spawn/control -> observe/scan -> collect -> science -> craft/transform -> build/place -> visible ecosystem consequence -> mission evidence -> save/reload;
7. produce the passed G2 route-review evidence bound to the committed HEAD;
8. return to clean current `main` and run `WorldMakers-G2-Certify.cmd`.

Only after G2 is actually `CERTIFIED` should G3 visual fidelity/presentation become the primary production gate.

## Evidence and failure diagnosis

Primary entry result:

```text
artifacts/native-materialization/native-materialization-entry.json
```

Nested evidence is retained under:

```text
artifacts/native-materialization/g1/
artifacts/native-materialization/g2/
```

A blocked result records the immediate reason while preserving the existing G1/G2 evidence files for deeper diagnosis. The normal diagnostic-first rule remains in force: a compiler or Unreal failure is a code/toolchain/runtime-state problem to classify, not an instruction to reinstall Unreal or Visual Studio by default.

## Truth boundary

This entry runner can establish that the project passed an acceptable native G1 run and that Unreal created the canonical G2 map under the required Git LFS policy. It cannot establish final rainforest art quality, manual gameplay-route success, G2 certification, G3 presentation quality, representative-device performance, packaging, external-alpha readiness or production release readiness.
