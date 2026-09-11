# First-Person Activation Commit & Runtime Takeover Certification v1

## Purpose

This phase closes the gap between an approved native-review candidate and a production build that actually uses the authored first-person assets.

A successful native import is not activation. An activation manifest is not runtime proof. `TAKEOVER_CERTIFIED` requires both a controlled activation commit and runtime evidence from the packaged build.

## Why the reviewed source commit and activation commit are different

A Git commit cannot safely contain the SHA of itself: changing a file to insert that SHA changes the commit SHA again. The previous exact-build interpretation therefore created a self-reference problem.

World Makers now uses two identities:

1. **Reviewed source commit** — the immutable commit that Blender/Unreal import, deformation/contact review and device review actually approved.
2. **Activation commit / build commit** — a later commit allowed to change only the four first-person activation manifests.

The activation manifest keeps `targetCommitSha` as the **reviewed source commit**. At packaging time a non-versioned build provenance file binds that reviewed source SHA to the real activation/build SHA.

## Activation commit boundary

`scripts/verify-first-person-activation-commit.py` compares the reviewed source commit to the activation head.

Exactly four paths may differ:

- `content/visual/first-person/first-person-authored-pack-v1.json`
- `game/Content/WorldMakers/Visual/first-person-authored-pack-v1.json`
- `content/visual/first-person/first-person-native-activation-v1.json`
- `game/Content/WorldMakers/Visual/first-person-native-activation-v1.json`

The authored pack must contain exactly five assets and nine animations, all with `authoredPresent=true`. The activation manifest must be `activated`, must target the reviewed source commit and must retain all native/human/device approvals.

Any runtime, source-art, importer, workflow, gameplay or unrelated file change between review and activation invalidates the activation commit.

## Build provenance

The repository contains an intentionally `unbound` template:

`game/Content/WorldMakers/Visual/first-person-build-provenance-v1.json`

After checkout, `scripts/generate-first-person-build-provenance.py` replaces the workspace copy with a bound package-time record containing:

- actual `buildCommitSha`;
- `reviewedSourceCommitSha`;
- `activationCandidateSha256`;
- SHA-256 of the packaged activation manifest.

This generated provenance is packaging evidence, not a committed attempt to self-reference the Git commit.

## Runtime production authorization

`FWMFirstPersonNativeActivationRuntime` only authorizes production takeover when:

- activation manifest is structurally approved;
- build provenance is bound;
- activation `targetCommitSha` equals provenance `reviewedSourceCommitSha`;
- candidate fingerprints match.

`-WMBuildCommit=<sha>` may additionally be supplied by certification tooling. When supplied, it must equal the packaged provenance build SHA.

The review-only flag `-WMEnableFirstPersonAuthored` remains separate and cannot produce a production certification.

## Runtime takeover report

`-WMFirstPersonTakeoverReport=<path>` asks the authored bridge to emit `visual.first-person-runtime-takeover.v1` evidence.

A positive report must show:

- production activation approved;
- review bypass disabled;
- complete five-asset / nine-animation authored set;
- first-person interaction active;
- authored takeover active;
- authored arms visible;
- both hand proxies hidden;
- active tool points to the authored tool when required;
- wrist device points to the authored asset.

`-WMFirstPersonTakeoverCertificationMode` activates a presentation-only Scan state so a native runner can exercise takeover without changing mission, building, science or reward authority.

## Proxy rollback proof

Certification performs a second launch with a deliberately wrong `WMBuildCommit`.

The required result is fail-closed:

- `productionActivationApproved=false`;
- `takeoverActive=false`;
- authored arms hidden;
- proxy hands restored;
- report status `BLOCKED_OR_FALLBACK`.

This proves rollback rather than merely asserting that fallback code exists.

## Final assessor

`scripts/assess-first-person-runtime-takeover.py` combines:

- activated manifest;
- bound build provenance;
- runtime takeover report;
- expected build SHA.

Only a consistent, bypass-free authored state can emit:

`TAKEOVER_CERTIFIED`

The output fingerprints the activation manifest, build provenance and runtime report using SHA-256.

## Current boundary

The source infrastructure can be validated in ordinary CI. Real `TAKEOVER_CERTIFIED` evidence still requires:

1. a real `ACTIVATION_CANDIDATE` from native human/device review;
2. a deliberate activation commit containing only the four allowed manifest changes;
3. a self-hosted Unreal + Blender runner;
4. successful authored takeover and wrong-build proxy rollback evidence from that build.

Until those conditions exist, the repository remains in the blocked/all-off state and the source-proxy path remains authoritative.
