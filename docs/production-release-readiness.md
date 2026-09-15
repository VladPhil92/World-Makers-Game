# Production Release Readiness

**Status: SOURCE READY / REAL RELEASE EVIDENCE REQUIRED**

This is the last pre-publication gate. It does **not** publish World Makers. It decides whether the exact Release Candidate payload may enter a controlled production rollout.

## What has to happen

1. Release Candidate must be `CERTIFIED` on the exact same commit.
2. Promote the RC payloads immutably. No rebuild is allowed after RC certification; Win64, Android and iPadOS SHA-256 values must remain identical.
3. Prepare the public semantic version (`x.y.z`) from the certified `x.y.z-rc.n` candidate.
4. Complete the production distribution evidence for the Windows production channel, Google Play production and Apple App Store production. Store credentials stay outside the repository.
5. Approve a staged rollout of **5% → 25% → 50% → 100%**, with minimum observation windows of **2h → 6h → 12h → 24h** and manual promotion between stages.
6. Verify health dashboards, alert routing, crash symbols, telemetry coverage, incident ownership, support escalation, store-withdrawal procedure and a real rollback drill.
7. Review privacy policy, terms, child-safety posture and store data-safety/privacy declarations. Open chat, child PII collection and real-money/token earning remain disabled.
8. Obtain explicit QA, Engineering, Child Safety, Product, Privacy/Legal and Release Operations launch approval.
9. Run `WorldMakers-ProductionRelease-Certify.cmd` from a clean current `main`.

## Health guardrails for the rollout

The rollout plan is pre-authorized only if its health policy includes at least 99.5% crash-free sessions, 99.9% fatal-error-free sessions, 98% install/launch success, 95% core-journey success, 100% save integrity, 98% mobile background/resume success and 99% telemetry coverage.

The execution phase must halt immediately for P0/P1 blockers, SLO breach, save-integrity failure, a confirmed child-safety incident or telemetry blindness.

## Evidence

Place real evidence under `artifacts/production-release/` using the committed templates:

- `release-manifest.json`
- `distribution.json`
- `rollout-plan.json`
- `operations.json`
- `release-decision.json`

The RC dependencies remain in `artifacts/release-candidate/`.

## Certification truth

Hosted CI validates only the source infrastructure and its fail-closed behavior. A real `CERTIFIED` result requires real store/channel readiness, rollback/monitoring evidence and human launch approval. `CERTIFIED` means *authorized for controlled staged rollout*, not *already released*.

## Next phase

After this gate is genuinely `CERTIFIED`, the next phase is **Production Release Execution**: perform the 5/25/50/100 rollout, collect live health evidence during each observation window, halt or rollback on trigger, and close the launch only after the final 24-hour observation window remains healthy.
