# V8 — Visual Optimization & Device Certification

## Status

**Source-complete certification infrastructure target.** Actual visual/device certification remains blocked until representative iPadOS and Android captures exist and the manual native workflow returns `CERTIFIED`.

## Purpose

V8 turns the visual-production track into a measurable release contract. It does not add spectacle. It asks whether V1–V7 remain readable, comfortable and performant on representative tablet hardware.

The certification system is **fail-closed**: source CI may prove that budgets, schemas, tests and release gates exist, but source CI cannot self-certify the visual product.

## Required scenarios

Every representative device evidence file must contain exactly these four scenarios:

1. `visual.rainforest.explore` — traverse the Caribbean rainforest with the production visual tier active.
2. `visual.build.dense` — exercise a dense but valid construction scene near the tier's practical world budget.
3. `visual.science.vfx-burst` — exercise scientific/environment feedback with the tier's bounded VFX concurrency.
4. `visual.adventure.reveal` — exercise V7 contextual camera + UI motion during an adventure reveal.

Each scenario requires at least **1,800 frame samples**.

## Metrics

Certification evaluates:

- p95 total frame time;
- p95 game-thread time;
- p95 render-thread time;
- p95 GPU time;
- peak draw calls;
- peak visible triangles;
- peak resident texture memory;
- peak active VFX.

The canonical thresholds live in `content/visual/certification/visual-certification-v8.json` and are aligned to the existing Tablet Low / Medium / High performance profiles.

### Planning ceilings

| Profile | FPS | p95 frame | game/render p95 | GPU p95 | draw calls | visible triangles | texture MB | active VFX |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Tablet Low | 30 | 33.34 ms | 24 / 24 ms | 30 ms | 700 | 750k | 512 | 8 |
| Tablet Medium | 30 | 33.34 ms | 23 / 23 ms | 29 ms | 950 | 1.2M | 768 | 16 |
| Tablet High | 60 | 16.67 ms | 13.5 / 13.5 ms | 15.5 ms | 1250 | 1.8M | 1024 | 28 |

These are release thresholds to validate, not measured claims about current hardware.

## Evidence integrity

A device evidence JSON cannot certify by itself. It must reference both a native performance capture and a screenshot stored under the evidence root. `assess-v8-visual-certification.py` recalculates SHA-256 for both files and blocks certification if either payload is missing, escapes the evidence root or does not match its declared hash.

Evidence contains a generic device class/model and OS version but must not contain serial number, advertising ID, account ID, child profile, email, biometric data or other hardware/user identifiers.

## Required device coverage

Certification requires at least one valid evidence package for each platform:

- representative **iPadOS** tablet;
- representative **Android** tablet.

A single platform can never produce `CERTIFIED`.

## Human visual review

Performance alone is insufficient. Every evidence package must explicitly pass:

- scene readability;
- UI legibility;
- camera comfort;
- absence of critical rendering artifacts.

The review role may be visual QA, tech art, art director or product owner. This is intentionally small and auditable; V8 does not collect child behavioral telemetry to infer visual quality.

## Runtime/source contract

`FWMVisualCertificationEvaluator` provides a deterministic C++ budget verdict for measured samples. It fails if the sample count is below the configured floor or any frame/thread/GPU/render-resource ceiling is exceeded.

This evaluator does not collect GPU metrics itself. Native Unreal tools such as Unreal Insights, CSV Profiler and ProfileGPU remain the measurement authority.

## CI model

Ordinary PR/push CI runs `scripts/validate-v8-visual-certification.py`. It validates source structure and executes the assessor self-test, but it cannot return a production certification.

Actual certification uses `.github/workflows/v8-visual-certification.yml`, which is manual (`workflow_dispatch`) and requires a self-hosted Windows/X64/Unreal runner plus:

- `UNREAL_SELF_HOSTED_ENABLED=true`;
- `UNREAL_ENGINE_ROOT`;
- `V8_DEVICE_EVIDENCE_ROOT` containing the real iPadOS and Android evidence packages and referenced captures/screenshots.

The workflow runs `WorldMakers.*` automation tests and then invokes the assessor with `--require-certified`. Missing evidence or failed budgets make the job fail.

## Accessibility / camera comfort

V7 reduced-motion constraints remain part of the visual release surface. The `visual.adventure.reveal` scenario must therefore be reviewed for camera comfort and UI legibility. V8 does not relax V7's bounded FOV/distance/offset behavior to obtain marketing-style captures.

## Optimization strategy

When a device misses budget, optimize in this order before lowering conceptual readability:

1. eliminate avoidable draw calls/material slots and transparent overdraw;
2. reduce foliage density/distance and shadow cost using the existing bounded performance profiles;
3. reduce VFX concurrency/particle complexity while preserving semantic shape;
4. reduce texture residency and author proper LOD/HLOD/culling;
5. reduce visible triangle density where silhouette is unaffected;
6. reduce screen percentage only within the existing profile contract.

Do not remove scientific/interaction feedback or UI meaning merely to hit a number.

## Production boundary

V8 can be **source-complete** while actual certification remains `BLOCKED`. Full visual completion requires authored assets from earlier visual phases, a native Unreal runner, real device captures and human review. Until those exist, the correct release state is not `CERTIFIED`.

## After V8

Once representative evidence passes, the V1–V8 visual-production track can be considered device-certified for the captured profiles and build commit. Future visual changes must either preserve the evidence contract or trigger new captures when they materially affect render cost or camera/UI comfort.
