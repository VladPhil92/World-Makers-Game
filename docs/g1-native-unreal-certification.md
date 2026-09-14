# World Makers — G1 Native Unreal Certification

Status: **IMPLEMENTED / NATIVE EVIDENCE REQUIRED**  
Engine: **Unreal Engine 5.8.2**  
Canonical contract: `content/production/g1-native-certification-v1.json`

## Purpose

G1 proves that the World Makers source tree can execute as a real Unreal Engine project on the locked Windows toolchain. It deliberately separates native code/runtime readiness from authored-map production.

G1 answers one question: **does the exact current `main` compile `WorldMakersEditor` in UE 5.8.2 and successfully execute the complete `WorldMakers.*` Unreal Automation namespace?**

It does not require final art, the certification map, manual gameplay smoke testing, or representative-device profiling. Those belong to later gates.

## Gate separation

- **G0** — source architecture and repository governance.
- **G1** — UE 5.8.2 native build + complete native automation.
- **G2** — authored `WM_PrototypeCertification.umap`, stable asset paths and playable authored route.
- **G3** — production visual/animation/VFX quality.
- **G4** — representative-device performance certification.

This separation is intentional. A missing `.umap` must not prevent the engineering team from proving that C++ and Unreal automation are healthy.

## One-command workstation entry point

From the repository root on the Windows Unreal workstation:

```bat
WorldMakers-G1-Certify.cmd
```

The command uses the existing diagnostic-first infrastructure. It does not install Unreal, Visual Studio, SDKs, Python, Git or Git LFS.

The strict sequence is:

1. clear stale G1 evidence;
2. resolve and verify UE 5.8.2;
3. run the workstation doctor;
4. verify repository freshness, branch policy and clean worktree;
5. run Unreal source preflight without requiring the authored map;
6. compile `WorldMakersEditor Win64 Development`;
7. execute `Automation RunTests WorldMakers.`;
8. validate the native evidence;
9. write the canonical G1 result.

## Canonical result

The result is written to:

`artifacts/g1-native/g1-native-readiness.json`

Allowed states:

- `CERTIFIED` — complete native pass on clean current `main` matching `origin/main`;
- `NON_CERTIFYING_PASS` — native checks passed using an explicit development exception such as `-AllowNonMain` or `-AllowDirtyWorktree`;
- `BLOCKED` — one or more mandatory G1 conditions failed or evidence is incomplete.

A hosted Ubuntu validator can prove only that the G1 contract and orchestration are structurally correct. It cannot emit `CERTIFIED`.

## Required native evidence

G1 retains these artifacts under `artifacts/g1-native/`:

- `workstation-doctor.json`;
- `source-preflight.log`;
- `readiness-result.json`;
- `build-result.json` and `build.log`;
- `automation-result.json` and `automation.log`;
- `native-failure-summary.json` when native compilation fails;
- `g1-native-readiness.json` as the canonical assessment.

The native pass requires exact UE `5.8.2`, target `WorldMakersEditor`, `Win64`, `Development`, complete `WorldMakers.` automation, source preflight and no readiness blockers.

## Development-mode checks

A developer may diagnose a branch without generating certification:

```powershell
.\scripts\run-g1-native-certification.ps1 -AllowNonMain -AllowDirtyWorktree
```

If native checks pass in this mode the result is `NON_CERTIFYING_PASS`. Development exceptions can never be promoted into G1 certification evidence.

## Failure policy

G1 remains diagnostic-first and fail-closed. A native build failure does not imply that Unreal or Visual Studio should be reinstalled. Inspect the structured failure category and logs first.

Stale evidence is deleted before a new G1 attempt so an old successful build or automation run cannot certify a new commit.

## Exit condition

G1 is complete only when `g1-native-readiness.json` contains `status: CERTIFIED` and `certified: true` for clean current `main`.

After that, move to G2 and author `game/Content/WorldMakers/Maps/WM_PrototypeCertification.umap` plus the first production-path Unreal assets. Until a Windows UE 5.8.2 execution produces that G1 evidence, the repository remains source-ready but native certification is still pending.
