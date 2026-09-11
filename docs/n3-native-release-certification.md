# N3 — Native Release Candidate Certification

N3 is the final fail-closed composition gate for the World Makers authored visual release chain. It does not replace N2, P5 or V8. It requires all three to have passed for the **same Git commit** and then verifies the native packages that were actually installed and exercised on representative Android and iPadOS devices.

## Release equation

A release can become `RELEASE_CERTIFIED` only when all of the following are true:

`N2 RELEASE_CANDIDATE + P5 CERTIFIED + V8 CERTIFIED + six native package proofs + human release review`

Source CI cannot self-certify N3.

## Exact six-pair matrix

N3 requires exactly one evidence package for each pair:

- Android / `performance.tablet.low`
- Android / `performance.tablet.medium`
- Android / `performance.tablet.high`
- iPadOS / `performance.tablet.low`
- iPadOS / `performance.tablet.medium`
- iPadOS / `performance.tablet.high`

Five of six is `BLOCKED`. Duplicate evidence for one pair is also `BLOCKED`. All evidence must refer to one identical 40-character release commit and Unreal Engine 5.8.2.

N3 also hardens the V8 assessor so V8 itself now requires the same six unique platform/profile pairs rather than merely observing both platforms and all three profiles somewhere in the evidence set.

## Native package proof

Each package evidence record is closed-schema and must prove:

- package type is valid for the platform (`apk`/`aab` on Android, `ipa` on iPadOS);
- package SHA-256 and byte size match the actual payload;
- package signature was verified by the native/device runner;
- install succeeded;
- launch succeeded;
- World Makers automation passed;
- N2 authored runtime takeover was observed;
- crash count is exactly zero;
- the smoke session lasted at least 120 seconds;
- every required smoke action passed;
- install log, runtime log and screenshot exist and match their SHA-256 fingerprints;
- the linked V8 evidence exists, matches its hash and belongs to the same platform/profile/commit.

The package-evidence schema intentionally excludes serial numbers, advertising IDs, account IDs, child profile IDs, emails and biometric identifiers. Broad device class and OS information remains in the existing V8 evidence contract, not N3 package proof.

## Smoke path

Every device/profile pair must exercise the same minimal gameplay path:

1. boot to a playable world;
2. move and control the camera;
3. place a build piece;
4. observe the world;
5. trigger science VFX;
6. change mission;
7. return to normal exploration.

This is a release smoke path, not a substitute for deeper gameplay QA.

## Evidence root

The final assessor expects one root containing:

- `n2-release-assessment.json`
- `p5-assessment.json`
- `v8-assessment.json`
- `release-review.json`
- `packages/*.json`
- package payloads and their referenced logs/screenshots/V8 evidence beneath the same root

All relative paths are constrained to that root; traversal and absolute paths are rejected.

## Human release review

`release-review.json` must be `approved` for the exact release commit, include the roles `release-qa`, `tech-art` and `product-owner`, and explicitly approve:

- the N2 release candidate;
- P5 art polish;
- the V8 device matrix;
- the child-safety regression review;
- absence of critical defects;
- release notes.

The repository ships only a `pending` example. No review is pre-approved.

## Automation

`collect-n3-package-evidence.py` creates one evidence record from real package/log/screenshot/V8 payloads. It never infers success: verification flags must be supplied explicitly, and a missing flag produces `status=failed`.

`assess-n3-native-release-certification.py` recomputes payload hashes, validates the dependency chain, enforces the exact six-pair matrix and emits either:

- `RELEASE_CERTIFIED`, or
- `BLOCKED` with explicit blockers.

The N3 GitHub workflow is manual and requires a self-hosted release-coordinator environment with `N3_RELEASE_EVIDENCE_ROOT`. The workflow can validate a prepared native evidence bundle but cannot manufacture representative device evidence.

## Certification boundary

N3 is **source-complete certification infrastructure** when its repository gate is green. That does **not** mean the game itself is release-certified.

Real `RELEASE_CERTIFIED` status remains blocked until:

1. N2 produces a real exact-build `RELEASE_CANDIDATE`;
2. P5 produces real `CERTIFIED` art-polish evidence;
3. V8 produces all six representative Android/iPadOS profile records;
4. the six native packages pass installation/runtime smoke checks;
5. the human release review is approved.
