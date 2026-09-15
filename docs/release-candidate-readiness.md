# World Makers — Release Candidate Readiness

Release Candidate (RC) Readiness is the gate between a successful controlled **External Alpha** and final **Production Release** approval. It does not create evidence by declaration: it consumes an External Alpha `CERTIFIED` result on the exact same commit and requires a frozen, reproducible, regression-clean and operationally reversible candidate.

## Source state versus certification

The repository contract is intentionally `source-ready-release-evidence-required`. Hosted CI can prove that the RC machinery is coherent, fail-closed and syntactically valid. It cannot claim that real packages were independently rebuilt, signed, regression-tested or approved by release owners.

A real RC `CERTIFIED` result is written to:

`artifacts/release-candidate/release-candidate-readiness.json`

and only clean current `main` synchronized with `origin/main` may produce the certifying state.

## 1. Freeze

`freeze-manifest.json` binds the candidate to:

- a semantic prerelease such as `0.1.0-rc.1`;
- one full repository commit;
- Unreal Engine 5.8.2;
- frozen content, dependencies and configuration;
- Win64 reference, Android and iPadOS target payload digests;
- verified mobile signing state.

A candidate whose source, dependency or configuration state changes is a new candidate and must produce new evidence.

## 2. Reproducibility

`reproducibility.json` requires an independent rebuild for every canonical target. The normalized payload SHA-256 from the independent rebuild must equal both the canonical rebuild digest and the payload digest recorded by the freeze manifest.

The normalized payload boundary exists because signed containers can contain nondeterministic signing metadata. The gameplay/content payload must still be reproducible.

## 3. SBOM and supply-chain provenance

Run the RC certification command with `-GenerateSbom` to create:

`artifacts/release-candidate/payloads/worldmakers.spdx.json`

The generator emits SPDX 2.3 from the repository state without network resolution. It inventories the locked Unreal Engine version, enabled Unreal plugins and discoverable package-lock dependencies.

RC certification requires:

- SPDX 2.3 or CycloneDX 1.6;
- SHA-256-bound SBOM evidence;
- dependency provenance review;
- zero known Critical vulnerabilities;
- zero known High vulnerabilities.

The generated source SBOM is an input to review, not a substitute for platform/store dependency inspection when native packaging adds platform libraries.

## 4. Regression and save compatibility

`regression.json` must cover Win64, Android and iPadOS. Every target requires:

- fresh install;
- upgrade install;
- full critical journey;
- save upgrade compatibility;
- offline recovery;
- mobile background/resume on Android and iPadOS;
- zero crashes;
- zero fatal errors;
- zero data-loss events.

Open P0 and P1 issues are not allowed.

Save compatibility is a hard RC boundary. A candidate that cannot safely load and continue supported Alpha saves is not eligible for promotion unless an explicitly reviewed migration policy replaces the affected evidence in a future contract version.

## 5. Crash diagnostics and release operations

`operations.json` requires:

- crash symbols retained and uploaded;
- production telemetry route verified;
- Android and iPadOS distribution signing verified;
- release notes prepared and SHA-256-bound;
- rollback package prepared and SHA-256-bound;
- rollback procedure verified;
- child PII disabled;
- open chat disabled;
- real-money and token earning disabled.

These child-safety boundaries remain release blockers, not optional post-launch settings.

## 6. Human go/no-go

`release-decision.json` must be `approved` with `decision=PROMOTE` and must reference the exact RC version and commit. Explicit named approvals are required from:

- QA;
- Engineering;
- Child Safety;
- Product;
- Security / Supply Chain.

Known blocking issues must be empty.

## One-command gate

From clean current `main`:

```bat
WorldMakers-ReleaseCandidate-Certify.cmd -GenerateSbom
```

Before running it, populate real freeze, reproducibility, regression, operations and release-decision evidence. Generating the SBOM alone does not certify the candidate.

## Fail-closed behavior

The assessor self-test proves that a complete synthetic candidate certifies, while each of the following independently blocks promotion:

- payload reproducibility drift;
- save upgrade incompatibility;
- unsafe open chat.

Hash mismatches for the SBOM, release notes or rollback package also block certification.

## Gate boundary

A real RC `CERTIFIED` result advances only to **Production Release Readiness**. That final phase owns production-channel publication, store/release metadata, staged rollout, post-release observation, rollback authority and final launch approval. RC certification itself never means that World Makers has been publicly released.
