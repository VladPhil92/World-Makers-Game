# M5.6E — Native Epic Continuity Certification

Status: **certification harness implemented; native execution blocked until Issue #9 infrastructure is available**.

## Goal

M5.6E converts the M5.6D source-level continuity promise into a native Unreal evidence contract. The phase proves, on the locked Unreal toolchain, that chapter-only epic persistence survives the engine's real SaveGame serialization path and that the persistence automation runs against the same commit that produced the evidence bundle.

M5.6E does not widen the child data model. The persisted boundary remains the versioned `FWMEpicCheckpoint`: epic identity, chapter identity/index/count and completion state only.

## Native automation scope

The dedicated native filter is:

`WorldMakers.Epic.Persistence.`

It must execute and expose all of the following tests in the Unreal automation log:

- `WorldMakers.Epic.Persistence.ChapterCheckpointDropsPartialEvidence`;
- `WorldMakers.Epic.Persistence.ResumeIndexFailClosed`;
- `WorldMakers.Epic.Persistence.NativeSaveGameRoundTrip`.

`NativeSaveGameRoundTrip` uses a unique automation-only slot, calls Unreal's real `SaveGameToSlot`, `LoadGameFromSlot` and `DeleteGameInSlot` path, verifies every checkpoint field, then removes the slot. It must never reuse the production `WM_EpicJourney_v1` slot.

## Locked native environment

The native job is allowed to run only when repository infrastructure enables the self-hosted Unreal runner. The runner contract remains:

- labels: `self-hosted`, `Windows`, `X64`, `unreal`;
- exact engine version from `game/UNREAL_ENGINE_VERSION` (currently UE 5.8.2);
- `UNREAL_SELF_HOSTED_ENABLED=true`;
- `UNREAL_ENGINE_ROOT` points to the certified engine root;
- Git LFS checkout enabled;
- authored certification map present at `game/Content/WorldMakers/Maps/WM_PrototypeCertification.umap`.

These requirements intentionally align with Issue #9 rather than bypassing it.

## Evidence contract

The workflow writes evidence under `artifacts/m5-6e-epic` and retains it as a GitHub Actions artifact. The bundle contains, when available:

- `runner-preflight.json`;
- `build-result.json` and `build.log`;
- `automation-result.json` and `automation.log`;
- `m5-6e-epic-certification-manifest.json`;
- `m5-6e-epic-assessment.json`.

The collector records:

- repository commit;
- expected and actual Unreal versions;
- exact automation filter;
- expected, observed and missing persistence tests;
- authored certification map presence and SHA-256;
- hashes for native evidence files;
- the privacy-minimized persistence boundary.

The independent Python assessor re-hashes evidence files and fails closed on commit mismatch, engine-version mismatch, missing authored map, missing test names, wrong filter, absent SaveGame round-trip, privacy-boundary drift or evidence-integrity failure.

## Privacy and data minimization

M5.6E must not add or certify persistence for:

- partial evidence counters;
- answers or free text;
- hint history;
- moral-choice content;
- personality or ideology labels;
- raw child interaction telemetry;
- Garden session-only mathematics/philosophy mastery gates.

A resumed chapter still starts with clean evidence/world-state counters.

## Certification semantics

A green source-contract job means only that the M5.6E harness is correctly wired.

A native M5.6E pass requires all of the following on the same commit:

1. locked-runner preflight passes;
2. authored certification map exists;
3. `WorldMakersEditor` build passes;
4. exact `WorldMakers.Epic.Persistence.` automation passes;
5. all three required tests are present in the automation log;
6. native SaveGame round-trip is observed;
7. evidence hashes verify;
8. the independent assessor returns `PASSED`.

Until the Issue #9 runner/map boundary is satisfied, the correct status is **SOURCE HARNESS READY / NATIVE EXECUTION BLOCKED**. M5.6E must not be described as packaged-device certification, production telemetry certification, representative-device performance certification or full game runtime certification.

## Next boundary

After M5.6E obtains real native evidence, the next continuity phase should connect the signed dashboard `epicResume` handoff to production native launch/bootstrap and prove end-to-end synchronization back to the durable player profile without expanding the child telemetry model.
