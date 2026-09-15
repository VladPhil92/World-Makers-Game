# World Makers — Pre-Unreal Content & Platform Readiness

This gate closes the remaining source-side gaps before native Unreal materialization. It is intentionally narrower than G1/G2: it proves that the project has complete source contracts and runtime boundaries for the domains that Unreal must materialize, while refusing to claim native assets, maps, builds or device evidence that do not yet exist.

## What this phase adds

### Audio

World Makers now has a semantic audio runtime boundary and a Caribbean Rainforest sound-design contract. Gameplay emits stable cue IDs rather than depending on final SoundWave/MetaSound asset paths. The initial catalog covers rainforest ambience, discovery/build music, observe/collect/build/science/ecosystem feedback and UI cues. Voice capture, open voice chat and behavioral audio profiling remain prohibited.

### Input

Enhanced Input is enabled and linked while legacy mappings remain available during migration. A stable semantic action catalog covers movement, look, jump, interaction, observation, building, undo/redo, measurement, mission cycling and pause. A landscape-tablet touch layout is defined for movement, look, interaction, jump, observation, build and pause. Native InputAction/InputMappingContext assets are still created by Unreal later; they are not fabricated in source control.

### UI

The production UI contract defines first-person HUD, build HUD, observation, mission, pause/settings, presentation and touch-control surfaces. It requires safe-area awareness, controller/keyboard/touch navigation, visible focus, minimum 48dp touch targets, reduced motion and no critical meaning conveyed by color alone.

### Accessibility and localization

The baseline supports English and Latin American Spanish (`es-419`) with bilingual core UI copy. Subtitles are on by default. Reduced motion, high contrast, text scaling, camera-shake scaling, hold/toggle choice, accessible labels and deterministic focus order are source requirements. Dark patterns, child-PII prompts, open chat and real-money prompts are explicitly prohibited.

### Platform profiles

The source contract covers Win64 reference, Android tablet arm64/ASTC and iPadOS arm64. Low/Mid/High mobile device-profile baselines are committed, but representative-device profiling remains the authority for final CVars and certification. Core vertical-slice play remains offline-first and cloud sync must degrade gracefully.

## Source gate

Run:

```bat
WorldMakers-PreUnreal-Validate.cmd
```

or directly:

```powershell
python scripts/validate-pre-unreal-content-platform.py
python scripts/validate-pre-unreal-content-platform.py --self-test
```

The self-test proves fail-closed behavior for four regressions:

- enabling child voice capture;
- removing required touch input;
- removing `es-419` localization;
- removing iPadOS from the target matrix.

On a clean current `main`, the PowerShell runner can emit:

```text
artifacts/pre-unreal/pre-unreal-readiness.json
status = READY_FOR_NATIVE_MATERIALIZATION
```

On another branch or with explicit non-certifying overrides it emits `NON_CERTIFYING_PASS`.

## Truth boundary

`READY_FOR_NATIVE_MATERIALIZATION` means the source-side handoff surface is complete enough to enter native authoring without inventing missing architecture while inside Unreal.

It does **not** mean:

- Unreal Engine 5.8.2 has compiled the project;
- real `.uasset` or `.umap` files have been created;
- authored rainforest/character/audio/UI assets have passed human review;
- Android or iPadOS device performance has passed;
- store packages are ready.

Those claims remain behind G1–G4 and later native/device gates.

## Native materialization inventory prepared by this phase

Unreal will be responsible for creating or importing, from these stable source contracts:

- `WM_PrototypeCertification.umap`;
- rainforest authored meshes/materials;
- Child Explorer skeletal mesh, skeleton, IK and animation assets;
- Niagara systems and presentation sequences;
- SoundWave/MetaSound/SoundMix/attenuation assets for the semantic audio catalog;
- Enhanced Input actions and mapping contexts;
- UMG widgets/styles/touch controls;
- localization target/resources for `en` and `es-419`;
- platform-specific cooked builds and representative-device evidence.

The source gate must stay green while that native work proceeds.
