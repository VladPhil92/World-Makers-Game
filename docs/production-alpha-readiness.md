# World Makers — Production Alpha Readiness

Status: **SOURCE READY / NATIVE PACKAGING AND PLAYTEST EVIDENCE REQUIRED**

Production Alpha Readiness is the first gate after G4. It does not replace G4 and does not declare the game ready for public release. Its purpose is to prove that a G4-certified build can be turned into traceable, installable alpha packages and operated safely enough for controlled external testing.

## Entry condition

Production Alpha certification requires a real `G4 CERTIFIED` result for the exact same repository commit. A source-only CI pass is not sufficient.

## Required alpha build lanes

| Target | Unreal platform | Configuration | Purpose |
| --- | --- | --- | --- |
| `windows-reference` | Win64 | Development | Internal reference, desktop smoke and debugging |
| `android-alpha` | Android / ASTC | Development | Representative Android tablet playtest |
| `ipados-alpha` | IOS | Development | Representative iPadOS playtest |

Win64 and Android packaging are assigned to a Windows UE 5.8.2 host. iPadOS packaging requires the macOS/Xcode Unreal lane. The repository deliberately does not pretend that a Windows runner can produce a valid iPadOS package.

## Native packaging

From a clean, current `main` checkout with UE 5.8.2 available:

```bat
WorldMakers-ProductionAlpha-Build.cmd -TargetId windows-reference
WorldMakers-ProductionAlpha-Build.cmd -TargetId android-alpha
```

Run the equivalent PowerShell Core script on the macOS Unreal build host for `ipados-alpha`:

```powershell
./scripts/build-production-alpha.ps1 -TargetId ipados-alpha
```

The packaging script uses Unreal Automation Tool `BuildCookRun` with build, cook, stage, pak, package and archive enabled. Android uses ASTC. Every successful package produces a canonical build manifest under `artifacts/production-alpha/builds/`.

A build manifest records the exact Git commit, UE version, target, configuration, host, archive, per-file size and SHA-256 plus an aggregate artifact SHA-256. Production Alpha certification re-hashes the actual package files; editing a manifest cannot conceal a changed binary.

Mobile builds intended for external distribution must have `signing.status=verified` and a non-empty signing identity. Credentials, certificates and provisioning material must never be committed to this repository.

## Smoke test contract

Each of the three build targets needs one matching smoke evidence file under `artifacts/production-alpha/smoke/` with status `passed` and the same `buildId` and repository commit as its build manifest.

Every target must demonstrate:

- install and launch;
- load `/Game/WorldMakers/Maps/WM_PrototypeCertification`;
- first-person control;
- observe/scan interaction;
- science interaction;
- build/place interaction;
- save and reload;
- clean exit;
- zero crashes, fatal errors and blocking issues.

Android and iPadOS must additionally demonstrate background/resume recovery.

## Crash telemetry and privacy

Controlled external alpha requires crash collection, symbol retention and correlation to build commit, build version and platform. The crash pipeline must collect the minimum technical context needed to diagnose failures without turning crash reporting into a child-data collection channel.

The gate explicitly requires:

- no child PII in crash telemetry;
- no free-text child data collection;
- build commit correlation;
- platform/version correlation;
- retained symbols for actionable crash signatures.

The implementation is provider-neutral: a crash provider can be selected later without weakening this contract.

## External playtest safety boundary

Production Alpha Readiness is intentionally stricter than ordinary internal QA because World Makers is intended for children. Before the gate can certify, the operations review must confirm:

- a guardian or otherwise authorized adult flow exists for child testing;
- open chat is disabled;
- real-money or token-earning mechanics are disabled for the alpha;
- feedback collection does not request child PII;
- a withdrawal/rollback process exists;
- release notes are prepared;
- there are zero known blocking issues;
- the controlled external tester target is at least five people.

This gate verifies readiness for the playtest; it does not fabricate five completed external playtests.

## Evidence layout

```text
artifacts/production-alpha/
├── builds/
│   ├── windows-reference.json
│   ├── android-alpha.json
│   └── ipados-alpha.json
├── smoke/
│   ├── windows-reference.json
│   ├── android-alpha.json
│   └── ipados-alpha.json
├── package/
│   ├── windows-reference/...
│   ├── android-alpha/...
│   └── ipados-alpha/...
├── alpha-ops-readiness.json
└── production-alpha-readiness.json
```

`artifacts/` remains local/CI evidence and is not source truth.

## Certification command

Once the build, smoke and operations evidence exists on the same clean current `main` commit:

```bat
WorldMakers-ProductionAlpha-Certify.cmd
```

Possible canonical states:

- `CERTIFIED`: complete evidence, G4 certified on the same commit and clean current `main`.
- `NON_CERTIFYING_PASS`: evidence passes but an explicit non-certifying context override was used.
- `BLOCKED`: any build, hash, smoke, G4, telemetry, safety or release-discipline requirement failed.

## CI boundary

Hosted CI validates the contract, Python tools, PowerShell syntax and the assessor's deterministic self-test. It cannot claim that Unreal packages were actually built, installed, signed or exercised on physical tablets.

## Exit condition

A real `CERTIFIED` result means **Production Alpha Readiness** has been reached: installable and traceable packages exist, smoke tests are clean, crash operations are ready and the product can move into a controlled external alpha playtest.

The next phase owns external playtest execution, issue intake/triage, release cohort management, crash-rate monitoring and promotion/rollback decisions. It is not yet public launch or store production release.
