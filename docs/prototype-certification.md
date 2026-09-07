# M1.5 — Prototype Certification & Hardening

## Purpose

M1.5 converts the M1 source prototype into a prototype that can be certified by a real Unreal Engine toolchain. A green lightweight GitHub job is not equivalent to runtime certification.

## Locked engine baseline

- Unreal Engine family: **5.8** (`EngineAssociation`).
- Certification patch baseline: **5.8.2** (`game/UNREAL_ENGINE_VERSION`).
- The self-hosted runner must expose an Engine root whose `Engine/Build/Build.version` matches the lock exactly.

## Source hardening gates

- placement must target sufficiently upward-facing surfaces;
- X/Y snapping derives from the traced surface and Z rests on the surface rather than snapping the surface height itself;
- placement performs an overlap query before spawning a piece;
- local save/load rejects unsafe transforms, invalid IDs, unsupported format versions, and excessive piece counts;
- deterministic grid and surface-grid automation tests exist;
- source-level validators run on ordinary GitHub-hosted runners.

## Runtime certification gates

All items below are required before M1.5 can be marked `CERTIFIED`:

1. At least one authored `.umap` exists under `game/Content/WorldMakers/` and is tracked through Git LFS.
2. A Windows x64 self-hosted runner labeled `unreal` is connected.
3. Repository variable `UNREAL_SELF_HOSTED_ENABLED=true` is set.
4. Repository variable `UNREAL_ENGINE_ROOT` points to the certified Unreal Engine 5.8.2 installation.
5. `WorldMakersEditor` compiles successfully with UnrealBuildTool/UnrealHeaderTool.
6. `UnrealEditor-Cmd.exe` runs `WorldMakers.Building.*` automation tests successfully.
7. A manual smoke test confirms player spawn, camera, placement, invalid-overlap rejection, remove, undo/redo, save and load.
8. Representative tablet profiling is captured before production art scope expands.

## Current certification state

**SOURCE HARDENED / RUNTIME CERTIFICATION BLOCKED**

The repository can enforce all source gates today. Full certification remains blocked until an Unreal Editor environment creates the first authored map and a UE 5.8.2 self-hosted runner executes the native build/test chain.

## Governance blocker

`main` must be protected with pull-request-only changes and required CI checks. The current GitHub connector used for this work does not expose branch-protection mutation endpoints, so repository administration must enable that rule separately. This is a release-governance requirement, not an optional recommendation.
