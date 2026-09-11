# P5 — Art Polish & Device Certification

P5 closes the authored visual-production track. It does not add another visual subsystem; it proves that the authored stack produced by P1–P4 is complete, polished and viable on representative tablets.

## Source-complete versus certified

P5 has two intentionally separate states. The source contract, assessor, native inventory collector and release workflow can be source-complete in CI. The game remains **BLOCKED** for visual certification until real Unreal assets, human review and device captures exist.

Source CI cannot self-certify P5.

## Native inventory requirement

Certification requires all **16 P1 targets** to be explicitly approved with `authoredPresent=true` and to exist as native `.uasset` files. The native Unreal collector additionally requires all **17 AnimSequence** assets from P4, all **17 Niagara** semantic targets, the final Animation Blueprint, IK Rig, Physics Asset, camera Data Asset, UI motion style and two Level Sequences.

Missing one asset blocks certification. P5 never mutates `authoredPresent` automatically.

## Final polish reviews

Materials and lighting must approve the master surface, water material, bounded exposure and shadow quality. Environment review covers LOD transitions, HLOD/culling, foliage overdraw, texture streaming, zero texture-pool over-budget allowance and no critical visible mip pop-in.

Character review requires approved deformation, IK, Physics Asset and cosmetic clipping with maximum observed foot sliding of 3 cm and zero critical deformation/clipping defects.

VFX review covers semantic parity, reduced motion, overdraw and profile-specific GPU p95 ceilings. Presentation review covers camera comfort, UI legibility, safe areas, reduced motion and confirms that reveals never lock input, force ViewTarget or alter global time scale.

Human review must provide four SHA-256 verified artifacts: a visual contact sheet, animation review capture, VFX overdraw evidence and camera/UI review evidence.

## Device evidence

P5 consumes the V8 four-scenario matrix but strengthens its completeness rule. It requires **six platform/profile packages** from one exact build commit:

- Android / Low
- Android / Medium
- Android / High
- iPadOS / Low
- iPadOS / Medium
- iPadOS / High

Every package still includes the V8 Rainforest Explore, Dense Build, Science/VFX Burst and Adventure Reveal scenarios with at least 1,800 measured frames per scenario and capture/screenshot integrity hashes.

## Fail-closed assessor

`scripts/assess-p5-art-polish-certification.py` checks repository-authored state, the Unreal-generated native inventory, human polish review and V8 device evidence. A green V8 run alone cannot certify P5. A complete art review without Android/iPadOS evidence also cannot certify P5.

The final result is either `CERTIFIED` or `BLOCKED`; there is no inferred or partial certification state.

## Native workflow

`.github/workflows/p5-art-polish-device-certification.yml` runs only on a self-hosted Windows Unreal runner. The operator supplies an evidence directory containing `p5-review.json`, review artifacts and the `v8/` evidence tree. Unreal generates `native-inventory.json` for the same `${{ github.sha }}` before the P5 assessor runs with `--require-certified`.

## Current boundary

At source-phase completion the certification is intentionally **BLOCKED** because final native asset approvals and representative-device evidence are not present in ordinary source CI. The next action is production/import/review on UE 5.8.2 and representative Android/iPadOS hardware, not another proxy phase.
