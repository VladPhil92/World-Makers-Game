# V6 — VFX & Fantastic/Scientific Feedback

## Status

**Source-complete VFX runtime target.** Native Niagara authoring, final material effects and representative-device visual certification remain separate production evidence.

## Principle

**Visual effects explain causality; they do not invent it.**

V6 is presentation-only. A VFX event cannot grant a reward, mutate a science result, complete learning evidence, move a gameplay actor, or substitute for an authoritative simulation outcome. The system only receives stable semantic IDs plus bounded location/direction/intensity/duration values.

## Runtime layers

1. `FWMVFXRuntime` validates semantic IDs and applies rate, active-count, intensity and duration budgets.
2. `UWMVFXSubsystem` observes neutral world read-models and accepts explicit events from trusted producers.
3. `AWMProceduralVFXActor` is the current source-visible fallback renderer.
4. `FWMScienceVFXAdapter` converts accepted M5.3 science results into visual events without changing the simulation.
5. Future Niagara systems consume the same semantic IDs and magnitudes.

The procedural renderer uses original ring, halo, burst and directional-chevron meshes. It is intentionally simple enough to remain a useful Low-tier fallback after authored Niagara effects exist.

## Stable feedback vocabulary

Gameplay/world:

- `gameplay.build.place`
- `gameplay.build.remove`
- `gameplay.build.move`
- `mission.measure.reveal`
- `world.observe.reveal`

Chemistry:

- `science.chemistry.dissolution`
- `science.chemistry.saturation`
- `science.chemistry.filtration`
- `science.chemistry.reaction`

Physics:

- `science.physics.force`
- `science.physics.circuit-flow`

Biology/ecology:

- `science.biology.cell-energy`
- `science.biology.plant-growth`
- `science.ecology.recovery`
- `science.ecology.stress`

Fantasy:

- `fantasy.portal.open`
- `fantasy.rune.activate`

Unknown semantic IDs are rejected rather than rendered with a generic spectacle effect.

## Real producers

### Construction

`UWMVFXSubsystem` subscribes to `UWMBuildWorldStateSubsystem::OnBuildWorldChanged`. It compares the previous/current neutral snapshots to classify additions, removals and moves. This keeps Building independent from VFX and gives feedback at the piece's actual world position.

### Ecology

V6 subscribes to `UWMEnvironmentStateSubsystem::OnEnvironmentStateChanged`. Signed changes across vegetation, water flow, soil protection and shade select either recovery or stress feedback. The visual state remains downstream of the authoritative ecosystem model.

### Science

`FWMScienceVFXAdapter` accepts only already-accepted simulation results:

- dissolution intensity derives from dissolved/undissolved mass and preserves saturation as a distinct semantic;
- filtration derives from retained-solid fraction and points downward to communicate separation direction;
- reaction intensity scales with bounded reaction extent;
- force preserves the force vector direction and scales against a caller-provided reference force;
- circuit feedback scales with power while retaining current/power as upstream authoritative values;
- cell feedback derives from produced-energy units;
- plant feedback derives from positive growth units;
- ecosystem delta maps positive net change to recovery and negative net change to stress.

The adapter never exposes a real-world chemistry procedure. It only visualizes results of the existing virtual simulation.

## Accessibility and child-safety

V6 does not require flashing, camera shake, strobing, purchase celebration or manipulative reward loops. `SetReducedMotion(true)` preserves semantic feedback but clamps travel motion to at most 35% of the normal path.

Effects are short-lived and rate-limited. Low/Mid/High tiers cap both active effects and events per second. The visual system contains no child-authored text, voice, face, biometric or behavioral-profile payload.

## Production Niagara handoff

The canonical manifest reserves `/Game/WorldMakers/VFX/NS_WM_*` targets for each semantic effect. Authored systems should consume the same event identity, position, direction and normalized intensity rather than re-deriving gameplay state.

Niagara production should preserve these requirements:

- no effect may imply a science outcome different from the simulation result;
- directional phenomena retain meaningful direction;
- saturation remains visually distinguishable from ordinary dissolution;
- recovery/stress remain legible without relying exclusively on hue;
- reduced-motion mode must preserve meaning with less travel/rotation;
- Low tier must have a bounded fallback and avoid uncontrolled translucent overdraw;
- effects do not become reward, retention or commerce pressure mechanisms.

The manifest currently states `authoredNiagaraAssetsPresent=false`. V6 therefore **does not claim** that Niagara systems, final shaders, audio-visual synchronization or tablet GPU profiling have already been produced.

## Quality budgets

Source planning ceilings:

| Tier | Active proxy effects | Events/s | Max duration | Max intensity | Niagara particles/effect target |
|---|---:|---:|---:|---:|---:|
| Low | 8 | 8 | 1.15 s | 0.85 | 48 |
| Mid | 16 | 16 | 1.50 s | 1.00 | 96 |
| High | 28 | 28 | 2.00 s | 1.00 | 180 |

These are ceilings for art production, not measured device performance. V8 must replace assumptions with GPU/frame-time evidence.

## Certification boundary

Source CI can validate IDs, budgets, causality adapters, procedural geometry, privacy constraints and world subscriptions. It cannot certify final Niagara rendering or perceived quality. Native Unreal visual review and representative Android/iPad profiling remain required.

## Next phase

**V7 — Camera, Cinematics & UI Motion** owns contextual framing, camera transitions, adventure reveals, motion-sickness-safe camera behavior and UI transition rhythm. V6 should provide the world feedback that those presentation systems frame, not embed camera choreography into each effect.
