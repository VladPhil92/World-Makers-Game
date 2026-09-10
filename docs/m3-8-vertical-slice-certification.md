# M3.8 — Caribbean Rainforest Vertical Slice Certification

## Purpose

M3.8 closes the source-side M3 development loop by defining a **fail-closed** certification system for the Caribbean Rainforest vertical slice. A source validator, hosted GitHub Actions job, native Unreal build, or isolated performance capture is not enough on its own. The slice is certified only when every required evidence domain agrees on the same repository commit and Unreal Engine baseline.

M3.8 certification infrastructure can be source-complete while runtime/device certification remains blocked. Issue #9 is still the explicit blocker for the locked Windows x64 Unreal Engine 5.8.2 runner and authored certification map.

## Certification matrix

| Domain | Required evidence | Pass condition |
| --- | --- | --- |
| Source | M1–M3.8 validators | all source gates pass |
| Native Unreal | preflight, build, `WorldMakers.*` automation | exact UE 5.8.2, authored map, build and complete automation pass |
| Manual smoke | `manual-smoke.json` | all baseline movement/build/save checks pass |
| M3 integrated route | `vertical-slice-route.json` | every M3.1–M3.7 experience check passes on the assessed commit |
| Tablet coverage | `devices/*.json` | at least two representative tablet records, including iPadOS and Android |
| Performance | referenced M3.7 capture JSON | each capture has >= 1,800 frame samples and passes the canonical selected profile budget |
| Integrity | SHA-256 + commit/version coherence | referenced capture hash and all evidence metadata match |
| Privacy | evidence schema/key scan | no PII, child/account identifiers, free text, behavioral analytics, commerce or hardware serial identifiers |

The output is always one of two states:

```text
CERTIFIED
BLOCKED
```

There is no partial state that may be marketed or treated as certification.

## Integrated vertical-slice route

The route evidence must cover the complete child experience rather than isolated unit tests:

1. Caribbean Rainforest biome loads.
2. Semantic exploration works across the authored/prototype route.
3. Deliberate observation interactions can be completed.
4. The rainforest science mission can be completed from gameplay evidence.
5. Reactive ecosystem consequences are visibly produced.
6. An ecological building intervention can be applied.
7. A creative unlock is granted deterministically.
8. My Adventures reflects trusted progress without exposing technical IDs.
9. The child can stop without losing a streak, reward, creation or progress.
10. Resume restores the persistent journey state.
11. The route exposes no commercial pressure or manipulative urgency.

Canonical template:

`docs/templates/m3-vertical-slice-route.example.json`

Runtime/operator evidence is copied into:

`artifacts/certification/vertical-slice-route.json`

## Representative tablet evidence

Certification requires **at least two** valid device evidence records and must include both:

- representative iPadOS tablet evidence;
- representative Android tablet evidence.

The contract deliberately records only engineering metadata: platform, non-unique device model/class, OS version, assessed commit, UE version, selected performance profile and capture hash. Serial number, device advertising ID, account ID, child ID and user identity are prohibited.

Each device record references a performance capture under `performance/`. The assessor does not trust the capture's `withinBudget` boolean alone. It reloads `content/performance/tablet-performance-profiles.json` and independently verifies:

- profile ID;
- minimum frame sample count;
- p95 frame time;
- world actor count;
- placed building-piece count;
- active interactable count;
- individual budget booleans;
- aggregate budget boolean.

The capture file SHA-256 must match the hash declared by the device evidence wrapper.

Canonical template:

`docs/templates/m3-tablet-device-evidence.example.json`

## Evidence inputs on the certification runner

The M3 certification workflow expects external evidence paths to be configured as repository variables when those artifacts are not already under the build evidence directory:

- `M3_MANUAL_SMOKE_EVIDENCE_FILE` — path to completed `manual-smoke.json`;
- `M3_VERTICAL_SLICE_EVIDENCE_FILE` — path to completed `vertical-slice-route.json`;
- `M3_DEVICE_EVIDENCE_ROOT` — directory containing `devices/*.json` plus referenced performance captures available to the collector.

These files are operator/device evidence, not source fixtures. The example templates remain `pending` and cannot create a certified result.

## Fail-closed assessor

Run:

```bash
python scripts/assess-m3-8-certification.py --evidence-dir artifacts/certification
```

This always writes:

`artifacts/certification/m3-vertical-slice-assessment.json`

For an actual release/certification decision, use:

```bash
python scripts/assess-m3-8-certification.py --evidence-dir artifacts/certification --require-certified
```

`--require-certified` exits non-zero unless every certification domain passes.

The assessor also has an offline deterministic self-test:

```bash
python scripts/assess-m3-8-certification.py --self-test
```

The self-test proves three paths: missing evidence is BLOCKED, a coherent synthetic evidence set is CERTIFIED, and a tampered capture hash returns BLOCKED.

## GitHub Actions separation

### Repository Quality / Unreal CI

Ordinary PR/push CI validates the **source contract**. Unreal CI may additionally emit a non-certifying assessment artifact when native evidence exists. These workflows cannot label the product certified merely because hosted checks are green.

### M3 Vertical Slice Certification

`.github/workflows/m3-certification.yml` is a separate `workflow_dispatch` release gate. It requires:

- `UNREAL_SELF_HOSTED_ENABLED=true`;
- a self-hosted Windows x64 runner labeled `unreal`;
- `UNREAL_ENGINE_ROOT` pointing to exact UE 5.8.2;
- the authored certification map;
- native build and `WorldMakers.*` automation;
- manual smoke evidence;
- integrated route evidence;
- representative iPadOS + Android evidence;
- passing M3.7 performance captures.

The workflow invokes the assessor with `--require-certified`; missing external evidence therefore fails the certification run rather than silently skipping it.

## Privacy boundary

M3.8 evidence is engineering certification data, not child behavioral analytics. No PII is permitted. Evidence must not contain child name/profile identifiers, account/user IDs, email, advertising IDs, physical/precise location, chat/free text, mission-answer text, learning-response text, purchase/commerce fields, payment tokens or hardware serial numbers.

Device model and OS version are allowed only as non-unique engineering compatibility metadata.

## Current state

After the M3.8 source phase merges:

```text
M3.1–M3.8 source architecture     SOURCE-COMPLETE
M3 native UE 5.8.2 certification BLOCKED BY ISSUE #9
M3 authored map smoke             BLOCKED/PENDING
M3 representative tablets         PENDING EXTERNAL EVIDENCE
M3 final status                    BLOCKED
```

No documentation or UI should represent M3 as runtime-certified until `m3-vertical-slice-assessment.json` contains `"certified": true` from the release certification workflow.

## Next source phase

With M3 source work closed, the next independent development stream is **M4 — Parent Portal MVP**. M4 source development can proceed while Issue #9 and representative-device certification are completed in parallel; it does not waive the M3 release gate.
