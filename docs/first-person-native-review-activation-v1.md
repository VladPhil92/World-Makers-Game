# First-Person Native Review & Activation v1

## Purpose

This phase converts the first-person authored source pack into a reviewable production activation without confusing successful import with artistic approval.

The source-proxy interaction kit remains the rollback path. Native authored art may enter production only after exact-commit import, automated tests, deformation/contact review, camera comfort review, Reduced Motion review and representative-device approval.

## Two takeover paths

### Review takeover

`-WMEnableFirstPersonAuthored`

This flag is deliberately temporary. It allows a reviewer to inspect the complete authored asset set even while the packaged activation manifest is still blocked. It does not mean that the pack is production-approved.

### Production takeover

Production does **not** require the review flag. Instead, the packaged `first-person-native-activation-v1.json` must be `activated`, structurally valid and approved for the exact SHA supplied through:

`-WMBuildCommit=<40-character-sha>`

If the manifest is missing, blocked, malformed or belongs to another commit, the source-proxy kit remains active.

## Required review scenarios

The native evidence package must cover all six:

1. Explore — tool raise/lower and return to quiet framing.
2. Scan — anticipate, hold and settle with scanner contact.
3. Build — point and confirm while keeping reticle readable.
4. Measure — focus pose with stable hand/tool contact.
5. Observe — close framing without near-plane clipping.
6. Reduced Motion — equivalent information state without nonessential bob/lag.

## Quantified review thresholds

The source contract currently requires:

- maximum hand-to-tool contact gap: **1.5 cm**;
- maximum visible tool penetration: **0.5 cm**;
- maximum elbow stretch ratio: **1.10**;
- maximum wrist twist: **75 degrees**;
- near-plane clipping: **0 frames** in the reviewed capture sequence;
- reticle obstruction: **0 frames**;
- maximum tool screen fraction: **0.22**;
- arms: maximum **1 material slot**;
- each scanner/build/measure tool: maximum **3 material slots**;
- wrist device: maximum **2 material slots**.

These thresholds are release gates, not suggestions.

## Evidence integrity

`first-person-native-review-evidence.template.json` defines the review payload. Every capture is referenced by a relative path and SHA-256. The assessor recalculates every capture hash, rejects paths escaping the evidence directory and verifies that the evidence references the exact native import report by SHA-256.

The native import report itself contains `commitSha`. The review evidence and import report must both match the requested target commit.

## Activation candidate

`scripts/assess-first-person-native-review.py` emits `ACTIVATION_CANDIDATE` only when:

- 5/5 visual assets imported;
- 9/9 animation assets imported;
- all six capture scenarios exist and hash correctly;
- all numeric thresholds pass;
- first-person automation tests passed;
- camera comfort is approved;
- Reduced Motion is approved;
- contact/deformation review is approved;
- human review is approved;
- representative-device review is approved;
- commit identity is exact.

Any failed condition produces `BLOCKED` and no candidate.

## Preparing activation

`scripts/prepare-first-person-native-activation.py` takes a valid candidate and creates **copies** of:

- the canonical/staged first-person authored pack with all 5 assets and 9 animations switched to `authoredPresent=true`;
- the canonical/staged activation manifest switched to `activated=true` and pinned to the candidate/import/review hashes.

The script never edits repository files directly. The generated manifests must be reviewed and committed as a separate, explicit activation change.

This separation makes activation auditable and keeps CI from silently approving art.

## Native workflow

`First-Person Native Review Activation` runs on a self-hosted Windows/X64/Unreal runner. It rebuilds the source pack from the selected commit, exports through Blender, reimports into Unreal with the exact commit SHA, runs `WorldMakers.Visual.FirstPerson*`, consumes review evidence and publishes activation-candidate artifacts.

The review evidence can live outside the checkout so `actions/checkout` may remain clean/reproducible.

## Certification boundary

This phase can source-certify the review/activation machinery and fail-closed runtime behavior. It cannot manufacture human visual judgment or representative-device evidence. Until a real native run satisfies the contract, the committed activation manifest must remain `blocked` and `authoredPresent` must remain false.
