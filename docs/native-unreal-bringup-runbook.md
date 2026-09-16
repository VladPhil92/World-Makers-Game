# Native Unreal Bring-Up

## Goal

The immediate production objective is no longer another source-only milestone. It is to obtain the first reproducible native Unreal result from the locked Windows + Unreal Engine 5.8.2 workstation:

1. compile `WorldMakersEditor Win64 Development`;
2. run the complete `WorldMakers.*` automation namespace through G1;
3. if G1 passes, let Unreal create or reconcile the real `WM_PrototypeCertification.umap`;
4. if anything fails, surface the first classified native blocker and preserve the complete evidence bundle.

This bring-up is deliberately non-certifying while the Final Pre-Unreal external handoff remains incomplete. It exists to advance native engineering without misrepresenting readiness.

## Required runner

GitHub must have an online Windows x64 self-hosted runner with these labels:

- `self-hosted`
- `Windows`
- `X64`
- `unreal`

Set repository variable `UNREAL_SELF_HOSTED_ENABLED=true` before attempting the native job.

`UNREAL_ENGINE_ROOT` is recommended but optional. When omitted, the repository engine resolver attempts deterministic discovery and still rejects any Unreal version other than 5.8.2.

## Run workflow

In GitHub Actions open **Native Unreal Materialization Entry**, choose **Run workflow**, keep `run_native_bringup=true`, and start the run on the current branch/commit being investigated.

The hosted `validate-controlled-entry` job verifies source wiring. The `native-bringup` job runs only for an explicit workflow dispatch and only when `UNREAL_SELF_HOSTED_ENABLED=true`; this prevents duplicate Unreal builds from consuming runner time on every push.

The native job executes the existing controlled entry with:

- `-AllowPreUnrealBlockedForDevelopment` — allows native diagnosis before external HTTPS/CTG One closure, but never certifies the handoff;
- `-AllowNonMain` — required because Actions may use a detached checkout for the selected commit;
- `-StopBlockingProcesses` — prevents a stale Unreal Editor/Live Coding process from hiding the actual build result.

## Evidence

The workflow always uploads `native-unreal-bringup-<commit>` containing available evidence from:

```text
artifacts/native-materialization/
```

The most useful files are:

```text
native-materialization-entry.json
g1/g1-native-readiness.json
g1/build-result.json
g1/build.log
g1/native-failure-summary.json
g1/automation-result.json
g1/automation.log
g2/g2-author-report.json
native-bringup-summary.json
```

If Unreal successfully authors the map, the workflow also uploads:

```text
game/Content/WorldMakers/Maps/WM_PrototypeCertification.umap
```

The job summary shows the first actionable blocker. `native-bringup-summary.json` is the compact machine-readable diagnosis.

## Failure loop

When the native job fails, use only the first classified blocker as the next repair target. Typical categories include C++/UHT/compiler defects, Live Coding/editor process interference, toolchain problems, engine-version mismatch, linker failures, or runtime automation failures.

Do not reinstall Unreal or Visual Studio merely because UnrealBuildTool returns a generic failure code. The repository classifier and bounded excerpts exist specifically to distinguish source-actionable failures from workstation-state problems.

Repair the blocker on a focused branch, validate hosted source checks, merge it, and rerun **Native Unreal Materialization Entry**. Repeat until G1 produces a native pass.

## Successful G1 and map authoring

After G1 succeeds, the same workflow invokes the existing G2 Unreal Python authoring path. Only Unreal Engine may create the `.umap`; source scripts must never fabricate a placeholder binary.

A successful development bring-up should leave the map present and the entry result at `AUTHORING_COMPLETE_COMMIT_REQUIRED` or an equivalent development-pass state. That is still not G2 certification.

Open `WM_PrototypeCertification` in Unreal Engine 5.8.2 and inspect it before committing anything.

## After the first real map exists

The next native tasks are:

1. verify player spawn, first-person camera, collision and movement;
2. verify the rainforest and mission-geometry authored actors;
3. import/review the nine Caribbean Rainforest P2 asset families;
4. set each `authoredPresent=true` only after the corresponding native asset is genuinely loadable and approved;
5. run the complete G2 route: observe/scan → collect → science → craft/transform → build/place → visible ecosystem consequence → mission evidence → save/reload;
6. review `git lfs status`;
7. commit the real `.umap` and `.uasset` files through Git LFS;
8. produce passed route-review evidence bound to that exact commit;
9. run `WorldMakers-G2-Certify.cmd` from clean current `main`.

Only then should visual G3 work become the main development priority.

## Truth boundary

A green native bring-up proves that the selected Windows/UE 5.8.2 workstation can compile and execute the current native path and, when successful, that Unreal authored the canonical certification map. It does not by itself certify the Final Pre-Unreal handoff, G2 route quality, G3 visual fidelity, G4 device performance, packaging, external alpha, or production release.