# World Makers — Pre-Unreal Readiness Audit

Status date: 2026-09-14

## Purpose

World Makers has accumulated substantial source-level gameplay, learning, persistence, web, identity, visual-production and certification infrastructure. Before moving active production into Unreal Editor, the repository must pass a hardening gate that separates three different claims:

1. **source-complete** — contracts and code exist and hosted validators pass;
2. **native-build-ready** — UE 5.8.2 compiles the project with the locked Windows toolchain;
3. **authored-runtime-ready** — committed Unreal maps/assets exist and the intended route runs in-editor and in automation.

Hosted source CI must never be treated as evidence for the latter two claims.

## Audit findings

### P0 — Native compile blockers discovered by the first real UE 5.8.2 build

The first workstation build exposed defects that hosted source validators did not detect:

- module-root-qualified includes such as `Adventure/...` and `Mission/...` were not resolvable because the runtime module did not expose `ModuleDirectory` explicitly;
- `WMChildJourneyWidget.cpp` declared local UMG variables named `Slot`, which collide with inherited `UWidget::Slot` under the locked compiler and produce C4458;
- `WMAuthoredVisualBridgeSubsystem.cpp` used `Components/ProceduralMeshComponent.h`; UE 5.8 expects the plugin-root include `ProceduralMeshComponent.h` when the `ProceduralMeshComponent` module is declared.

This hardening branch fixes all three defects and adds a hosted static regression gate.

### P0 — Native CI remains conditional, not continuous

`.github/workflows/unreal-ci.yml` runs extensive source-contract validation on GitHub-hosted Ubuntu runners, but the actual `WorldMakersEditor` build and `WorldMakers.*` automation suite execute only when `UNREAL_SELF_HOSTED_ENABLED == 'true'` on a Windows UE runner. Therefore a green `Unreal CI / project-validation` check is not proof that the C++ project compiles.

Issue #9 remains the infrastructure blocker: provision the UE 5.8.2 self-hosted Windows runner, configure `UNREAL_ENGINE_ROOT`, enable the repository variable, and retain native evidence artifacts.

### P0 — Authored certification map is absent

The native preflight and M1.5 validator both require:

`game/Content/WorldMakers/Maps/WM_PrototypeCertification.umap`

That map is not currently tracked. The repository intentionally supports procedural/proxy world generation, but certification requires a deterministic authored level committed through Git LFS. This is the first unavoidable Unreal Editor deliverable after source hardening.

### P0 — Repository currently contains source/data, not committed Unreal authored binaries

The current tree contains no tracked `.umap` or `.uasset` files. That is acceptable for source prototyping but not for a production-ready Unreal project. It means final maps, Blueprint assets, materials, skeletal meshes, animation blueprints, Niagara systems and authored UI assets are not yet reproducible from Git alone.

The next Unreal phase must create the certification map first, then add authored assets incrementally under stable `/Game/WorldMakers/...` paths with Git LFS.

### P1 — LFS policy is too broad for web chrome and already caused checkout noise

`.gitattributes` routes every PNG through Git LFS, including the Player Dashboard logo. The logo was committed as an ordinary Git blob, producing `Encountered 1 file that should have been a pointer` during checkout. This branch explicitly exempts that small web asset while retaining LFS for Unreal/DCC binaries and general production media.

### P1 — Project identity was left at the zero GUID

`DefaultGame.ini` used `ProjectID=00000000000000000000000000000000`. This branch assigns a stable non-zero project ID before the repository begins accumulating editor-authored binary content.

### P1 — Documentation and issue state drift behind implementation

The canonical roadmap still describes M5.6B as the next phase while the repository has already advanced through later M5.6 work and multiple visual/authored-integration phases. Open issues also mix completed source work with genuinely blocked native/runtime work. Before the Unreal production phase, roadmap and issue hygiene should be reconciled so `source-complete`, `native-certified`, `device-certified` and `production-art-complete` are never conflated.

### P1 — Certification contracts are stronger than the implementation evidence

There is a large number of deterministic token/source validators. They are useful governance, but several are structural checks rather than executable gameplay tests. The M5.6G failure caused by an equivalent ternary expression illustrates the brittleness of token-based validation. Future gates should prefer behavioral tests where feasible and reserve string-token checks for architectural invariants that cannot be tested otherwise.

### P2 — UE 5.8 deprecation backlog

The first native build also surfaced API deprecation warnings, including direct access to UMG focus state and engine APIs scheduled for removal in a future release. These warnings do not currently block UE 5.8.2, but they should be burned down before adopting a later engine baseline.

### P2 — Input stack is internally consistent but legacy

Player input currently uses the classic `InputComponent` binding model. It is acceptable for the locked vertical slice and should not be migrated before native build stability. Enhanced Input can be evaluated after the first certified route to avoid expanding the hardening scope.

## Pre-Unreal exit gate

Do **not** begin broad Unreal content production until all source-side items below are true:

- hosted Repository Quality and Unreal source checks are green;
- `validate-unreal-source-preflight.py` is green;
- local UE 5.8.2 `WorldMakersEditor Win64 Development` build returns `Result: Succeeded`;
- no known fatal compiler errors remain;
- LFS checkout is clean;
- the hardening changes are merged to `main`;
- roadmap/issues have a single, explicit distinction between source completeness and native/device certification.

The authored certification map is intentionally the **first Unreal Editor task**, because a `.umap` cannot be correctly produced by ordinary source tooling. Once created and committed through LFS, native automation and manual smoke testing become the next gate.

## Unreal entry sequence after this gate

1. Pull the hardened `main` and run Git LFS checkout.
2. Open the project in UE 5.8.2 only after a clean command-line build.
3. Author `WM_PrototypeCertification.umap` at the exact path required by the preflight contract.
4. Add deterministic PlayerStart and minimal authored geometry; keep procedural systems available as fallback.
5. Run `WorldMakers.Building.*`, then the full `WorldMakers.*` automation namespace.
6. Commit the map/assets through Git LFS and enable the self-hosted native CI runner.
7. Only then begin broad authored environment, character, animation, VFX and presentation production.

## Decision

World Makers is **not ready for broad Unreal content production yet**, but it is close to the correct transition point. The immediate work is repository hardening and native compile closure, not additional feature expansion. After the hardening gate passes, Unreal Editor becomes the required production environment for maps/assets rather than an optional visualization step.
