# G3 — Visual Fidelity & Presentation Readiness

## Status

**Source infrastructure implemented. Native visual certification remains intentionally blocked until G2 is certified and real UE 5.8.2 evidence exists.**

G3 is the quality gate between the authored playable vertical slice from G2 and the representative-device performance gate in G4. Its purpose is to answer a narrower but critical question:

> Does the G2 certification route now look, animate and present itself like a production-quality game rather than a technically functional prototype?

G3 does not certify Android/iPadOS frame-time, thermal stability or memory budgets. Those remain G4 responsibilities.

## Dependencies

G3 builds directly on:

- **G2** — the real `/Game/WorldMakers/Maps/WM_PrototypeCertification` map and complete playable route;
- **P1/P2/P3/P4** — authored environment, character, animation, VFX and presentation asset contracts;
- **P5** — the existing final polish thresholds and the native inventory collector.

The gate deliberately reuses `scripts/unreal/collect-p5-native-inventory.py`. There must be one native source of truth for asset existence; G3 does not introduce a competing registry.

## Quality bar

### Materials and lighting

The certification route must prove:

- the authored master surface is present and visually approved;
- the water material is present and visually approved;
- exposure is deliberately bounded rather than left to uncontrolled adaptation;
- the rainforest owns exactly one primary directional sun;
- Sky Light, Sky Atmosphere and Exponential Height Fog are present exactly once on the canonical rainforest actor;
- no critical shadow artifacts remain.

The native map inspector verifies the structural lighting components. Human review owns artistic quality, exposure and shadow judgment.

### Environment

G3 requires the complete authored Caribbean Rainforest takeover, not a mixture of approved art and visible prototype stand-ins.

Human visual QA must approve:

- LOD transitions;
- HLOD/culling behavior;
- foliage overdraw behavior;
- texture streaming;
- zero texture-pool over-budget allowance at the review target;
- no critical visible mip pop-in;
- no visible placeholder/prototype geometry in the certification route.

### Character and animation

G3 inherits the P4/P5 requirement for all 17 animation clips plus the authored Animation Blueprint, IK Rig and Physics Asset.

The review must report:

- authored character approval;
- deformation approval;
- IK approval;
- Physics Asset approval;
- cosmetic clipping approval;
- maximum observed foot sliding of **3 cm or less**;
- zero critical deformation defects;
- zero critical cosmetic clipping defects.

This is an animation-quality gate, not merely an asset-existence gate.

### VFX

All 17 P4 Niagara effects must exist natively and preserve their gameplay/science semantics.

G3 requires human approval for:

- semantic parity;
- Reduced Motion behavior;
- overdraw behavior;
- presence of every required effect.

Representative-device GPU budgets are intentionally deferred to G4.

### Camera, cinematics and UI motion

The final presentation layer must include:

- camera presentation Data Asset;
- both required Level Sequences;
- authored UI motion style;
- camera comfort approval;
- UI legibility and safe-area approval;
- Reduced Motion approval;
- reveal duration no longer than **2.5 seconds**;
- no input lock;
- no forced ViewTarget takeover;
- no global time-scale manipulation.

The goal is cinematic polish without taking agency away from the player.

## Evidence contract

A real G3 review must provide exactly four evidence artifact kinds:

1. `visual-contact-sheet` — representative route frames proving environment, materials and lighting;
2. `animation-review` — locomotion + interaction review evidence;
3. `vfx-overdraw` — VFX semantic and overdraw review evidence;
4. `camera-ui-review` — camera, reveal, UI legibility, safe-area and Reduced Motion evidence.

Each artifact is referenced from `g3-review.json` and bound by SHA-256. The review itself is bound to the exact 40-character repository commit.

`content/production/g3-visual-review-template.json` is intentionally `pending` and contains no fabricated hashes, filenames or passed checks.

## Native inspection

`scripts/unreal/inspect-g3-visual-map.py` loads the real G2 map in Unreal Editor and verifies:

- exact certification map path;
- exact `WM_G2_Rainforest` authored actor;
- UE 5.8.2;
- one Directional Light component;
- one Sky Light component;
- one Sky Atmosphere component;
- one Exponential Height Fog component;
- evidence bound to `WM_BUILD_COMMIT`.

This is structural evidence only. It does not claim that lighting looks good.

## Certification flow

Run from a **clean, current `main`** after the real G2 route has already been authored and its route-review evidence is available:

```bat
WorldMakers-G3-Certify.cmd -G2RouteReviewFile artifacts\g2-authored\g2-route-review.json
```

The orchestrator then:

1. validates the G3 source contract;
2. resolves UE 5.8.2;
3. verifies clean current `main`;
4. runs G2 as a prerequisite and requires `CERTIFIED` on the same commit;
5. regenerates the deterministic P4 source bundle;
6. collects the existing P5 native authored-asset inventory in Unreal;
7. inspects the G3 certification map in Unreal;
8. loads `artifacts/g3-visual/g3-review.json`;
9. validates all review thresholds and SHA-256 evidence;
10. returns `CERTIFIED` only when every requirement passes.

The canonical result is:

`artifacts/g3-visual/g3-visual-fidelity.json`

## CI behavior

`.github/workflows/g3-visual-fidelity.yml` has two intentionally different jobs.

### Hosted source validation

Always available. It verifies contracts, thresholds, scripts and Python syntax. It **cannot certify G3**.

### Native visual inspection

Runs only when repository variable `G3_NATIVE_VISUAL_ENABLED=true` and an authorized self-hosted Windows/X64/Unreal runner is available. It collects native inventory and inspects the map, but it still does not manufacture human review evidence.

## Production truth

A green hosted G3 workflow means:

**G3 SOURCE INFRASTRUCTURE READY**

It does not mean:

**G3 VISUAL FIDELITY CERTIFIED**

Certification requires real authored binaries, the real G2 `.umap`, a clean current-main commit, Unreal 5.8.2 inspection and human visual evidence.

## Next gate — G4

After G3 is genuinely `CERTIFIED`, G4 will consume the same polished build and move to representative Android/iPadOS device certification: frame-time, memory, thermal behavior, VFX GPU budgets and the existing V8 scenario matrix.
