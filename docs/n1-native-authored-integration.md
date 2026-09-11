# N1 — Native Authored Integration Candidate

## Purpose

N1 is the first post-P5 native-integration phase. It does not add another visual proxy or another source-art format. It proves whether the authored content produced by P2, P3 and P4 can coexist in one Unreal checkout and whether that exact checkout is eligible to propose activation of the P1 registry.

The output is intentionally binary:

- `BLOCKED`
- `ACTIVATION_CANDIDATE`

An `ACTIVATION_CANDIDATE` is not a merge, release, certification or automatic activation. It is a generated review artifact.

## All-or-nothing activation

N1 uses an all-or-nothing policy. A partial P1 registry is not produced. The candidate exists only when all of the following pass in one native runner execution:

- P2 imports all 9 rainforest assets and every native import contract passes;
- P3 imports all 8 character/cosmetic modules, creates the shared Skeleton and passes every import contract;
- P4 imports all 17 AnimSequence targets and creates both Level Sequence containers;
- P5 native inventory sees all 16 P1 registry targets;
- P5 native inventory sees all 17 animation targets;
- P5 native inventory sees all 17 VFX targets;
- P5 native inventory sees all 7 presentation/rig targets;
- the inventory belongs to the exact GitHub commit being assessed.

If any requirement is missing, the result remains `BLOCKED`.

## Native runner orchestration

The manual workflow `N1 Native Authored Integration Candidate` executes the production chain in one self-hosted Windows + Unreal runner:

1. generate P2 source and export FBX LOD payloads with Blender;
2. import P2 rainforest assets into Unreal;
3. generate P3 character source and build the rigged FBX payload with Blender;
4. import P3 character/cosmetic assets into the same Unreal Content tree;
5. generate the canonical P4 source bundle and verify its SHA-256 against the P4 manifest;
6. export the 17 P4 animation FBXs;
7. import P4 AnimSequence assets and presentation containers;
8. collect the P5 native inventory from that same checkout;
9. run the `WorldMakers.*` automation suite;
10. assess N1 readiness.

This is intentionally stricter than running P2/P3/P4 workflows independently, because independent runs can describe different Content states.

## Activation candidate

When N1 reaches `ACTIVATION_CANDIDATE`, it writes:

`artifacts/n1/authored-assets-p1.activation-candidate.json`

The candidate is a deep copy of the current P1 registry. The only permitted field change is:

`authoredPresent: false -> true`

Every ID, object path, fallback policy, source path, LOD requirement, material-slot budget and collision policy must remain byte-equivalent at the data-model level.

N1 does not mutate `content/visual/authored/authored-assets-p1.json`, does not commit the candidate, does not open a merge automatically and does not remove procedural fallback.

## Human review

Human review is mandatory before applying the candidate. The reviewer should compare the generated candidate with the repository P1 manifest, inspect the N1 readiness report, confirm that the native inventory corresponds to the intended Unreal checkout and verify that the remaining P5 artistic review/device-certification work is understood.

Applying the candidate is therefore a deliberate repository change, not a side effect of import automation.

## Relationship to P5

N1 is not P5 certification. It only proves native integration readiness and generates a possible P1 activation patch. P5 still requires art-polish evidence plus the six Android/iPadOS × Low/Medium/High V8 evidence packages from the exact certified commit.

## Current source boundary

Source CI can validate the N1 contract and run the assessor self-test. It cannot create a real `ACTIVATION_CANDIDATE` because the normal GitHub-hosted runner does not contain Unreal-authored `.uasset` content. The native workflow remains fail-closed when assets such as final materials, AnimBP, Niagara, IK Rig, Physics Asset or camera/UI assets are absent.

## Next phase — N2

N2 is **Authored Activation & Release Candidate**. After a human approves the N1 candidate, N2 will validate the deliberately activated registry, exercise runtime takeover instead of procedural fallback, build a release-candidate package and hand that exact commit to the P5/V8 device-certification workflow.
