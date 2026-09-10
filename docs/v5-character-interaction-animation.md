# V5 — Character & Interaction Animation

## Status

**Source-complete animation-runtime pass; authored animation clips, Animation Blueprint graph, IK assets and native/device visual certification remain pending.**

V5 replaces the V1 gait proof as the active motion system for the V4 procedural avatar. It does not pretend that source-authored transforms are final character animation. Instead, it establishes the deterministic state/read-model, semantic action vocabulary, layer boundaries and authored-asset paths that the production animation graph must consume.

## Design rule

**CharacterMovementComponent owns movement. Animation explains movement.**

V5 does not use animation root motion to drive exploration or building locomotion. `CharacterMovementComponent` and the gameplay capsule remain authoritative for translation, jump physics and collision. This prevents cosmetic animation changes from changing gameplay reach, speed or hitboxes.

## Runtime architecture

`FWMCharacterAnimationRuntime` is an asset-agnostic deterministic state model. `UWMCharacterAnimationComponent` samples the owning `AWMPlayerCharacter`, updates that runtime and exposes a compact AnimBP read-model.

The production graph can read:

- `locomotionStateId`;
- `speedAlpha`;
- `isAirborne`;
- `stateAgeSeconds`;
- `aimYawDegrees`;
- `aimPitchDegrees`;
- `interactionActionId`;
- `interactionAlpha`.

The V4 procedural hierarchy consumes the same state through a bounded source pose, so source-visible builds already demonstrate the intended motion grammar without requiring binary assets.

## Locomotion state machine

Nine explicit states are stable:

1. `locomotion.idle`
2. `locomotion.start`
3. `locomotion.walk`
4. `locomotion.run`
5. `locomotion.stop`
6. `locomotion.turn-in-place`
7. `locomotion.jump`
8. `locomotion.fall`
9. `locomotion.land`

Starts and stops are short transitional states rather than instantaneous switches. Jump and fall are separated by vertical velocity. Landing has a short bounded compression phase. Turn-in-place derives from view/character yaw divergence while stationary.

The source pose adds readable gait opposition, calf/foot follow-through, restrained body lean, head look and landing compression. All transforms are hard-bounded before application.

## Interaction animation vocabulary

V5 defines eight stable semantic actions:

- `interaction.build-place`
- `interaction.build-remove`
- `interaction.build-move`
- `interaction.measure`
- `interaction.observe`
- `interaction.inspect`
- `interaction.pickup`
- `interaction.science-manipulate`

The first five are wired to existing gameplay paths. Build, measurement and observation gestures trigger only when the corresponding gameplay operation succeeds. The remaining actions are available through `TriggerActionById` for later pickup/inspection/science producers.

Interaction gestures are upper-body layers. They do not own movement and do not pause the locomotion state machine. This is important in an exploration/building game: a child can continue moving while a short contextual gesture completes unless a future gameplay mechanic deliberately imposes a movement constraint.

## Jump input

V5 adds explicit `Jump` input and binds press/release to `ACharacter::Jump` / `StopJumping`. The animation state is still derived from `CharacterMovementComponent::IsFalling()` and vertical velocity rather than trusting the button press itself. That keeps animation correct when the character falls without jumping or a jump request is rejected by gameplay physics.

## Layering contract

The authored Animation Blueprint should implement four conceptual layers:

1. **Base locomotion** — idle/start/walk/run/stop/turn/jump/fall/land.
2. **Look additive** — bounded head/chest aim from yaw/pitch read-model values.
3. **Upper-body interaction** — semantic build/measure/observe/inspect/pickup/science actions.
4. **Landing response** — short compression/recovery layered over the grounded transition.

`ABP_WM_ChildExplorer` is the reserved production target. Root motion remains disabled by default. Interaction montages must not silently alter capsule translation.

## Authored production targets

The V5 manifest reserves:

- `/Game/WorldMakers/Characters/Player/ABP_WM_ChildExplorer`
- `/Game/WorldMakers/Characters/Player/Animations/A_WM_Child_Idle`
- `/Game/WorldMakers/Characters/Player/Animations/A_WM_Child_Walk`
- `/Game/WorldMakers/Characters/Player/Animations/A_WM_Child_Run`
- `/Game/WorldMakers/Characters/Player/Animations/A_WM_Child_Jump`
- `/Game/WorldMakers/Characters/Player/Animations/A_WM_Child_Fall`
- `/Game/WorldMakers/Characters/Player/Animations/A_WM_Child_Land`
- `/Game/WorldMakers/Characters/Player/Animations/Interactions/*`
- `/Game/WorldMakers/Characters/Player/IKR_WM_ChildExplorer`

These paths are handoff contracts. V5 does **not** claim that the corresponding `.uasset` files exist yet.

## Motion quality target

The final authored animation should feel deliberate, playful and physically readable without exaggerated hyperactivity. Use anticipation, weight transfer and follow-through, but avoid constant idle fidgeting, reward-dance spam or motion whose main purpose is to hold attention.

Build/place interactions should read as intentional reaching and placement. Measurement should read as careful framing/inspection. Observation should direct attention to the world rather than to UI spectacle. Science manipulation should emphasize cause and effect.

## Accessibility and child-safety boundary

The V5 contract does not require camera shake, flashing feedback, biometric motion capture or face/voice capture. Reduced-motion support is a production requirement: authored clips and additive layers must permit amplitude/rate reduction without changing gameplay state or success conditions.

There are no commerce animation hooks and no manipulative celebration loop in the animation runtime.

## Performance handoff

The manifest defines pose/update targets rather than claiming measured performance:

| Tier | Target pose rate | Concurrent additive layers | Finger animation |
|---|---:|---:|---|
| Low | 30 Hz | 2 | No |
| Mid | 45 Hz | 3 | No |
| High | 60 Hz | 4 | Allowed |

V8 must replace these planning ceilings with native Unreal and representative-device evidence.

## Validation

`WMCharacterAnimationRuntimeTests.cpp` covers:

- deterministic locomotion transitions;
- explicit jump/fall/land sequencing;
- interaction layering that does not own locomotion;
- look and final-pose bounds;
- semantic interaction ID round-trip stability.

`scripts/validate-v5-character-animation.py` verifies canonical/staged manifest parity, runtime/read-model wiring, gameplay-success action hooks, jump input, tests, documentation and Repository Quality integration.

## Production boundary

V5 is **source-complete**, not animation-art certified. Final certification still requires authored animation clips, a real `ABP_WM_ChildExplorer`, retarget/IK review, root-motion audit, blend/foot-sliding review, native Unreal execution and representative tablet profiling.

The old `FWMPrototypeMotionStyle` remains only for the explicit six-piece legacy rollback path required by earlier certification contracts.

## Next phase

**V6 — VFX & Fantastic/Scientific Feedback** should connect visible effects to real simulation/gameplay outcomes: construction, discovery, chemistry, forces, circuits, biology, ecology and fantastic adventure transformations, with accessibility-safe flash/overdraw budgets.
