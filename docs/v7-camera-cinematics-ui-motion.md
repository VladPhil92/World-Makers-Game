# V7 — Camera, Cinematics & UI Motion

## Status

**Source-complete presentation-runtime target.** Authored Level Sequences, final UI art, native camera review and representative-device motion-sickness validation remain production evidence.

## Principle

**Presentation frames the player's action; it does not take ownership of it.**

V7 coordinates camera distance/FOV/offset and lightweight UI motion downstream of gameplay and V6 semantic feedback. It does not complete missions, grant rewards, alter scientific outcomes or own input authority.

## Camera language

Six stable camera modes define the source presentation grammar:

- `Explore` — default third-person framing;
- `Build` — slightly wider, higher framing so nearby structure remains legible;
- `Observe` — tighter framing for environmental inspection;
- `Science` — closer analytical framing for simulation feedback;
- `Dialogue` — reserved conversational distance without forcing a view target;
- `AdventureReveal` — wider/high reveal used for short adventure beats.

`UWMPresentationSubsystem` discovers the local pawn's existing `USpringArmComponent` and `UCameraComponent`, keeps spring-arm collision enabled and interpolates only `TargetArmLength`, `TargetOffset` and FOV. It explicitly disables camera lag/rotation lag in this source path to avoid secondary oscillation.

V7 does **not** set controller yaw/pitch, call `SetViewTarget`, freeze movement, change global time scale or require camera shake.

## V6 integration

V6 now exposes `OnVFXAccepted` after an event has passed its own semantic and budget validation. V7 subscribes downstream:

- `gameplay.build.*` -> Build framing + build confirmation cue;
- `world.observe.reveal` -> Observe framing;
- `mission.measure.reveal` and `science.*` -> Science framing;
- `science.ecology.*` -> Science framing + living-world cue;
- `fantasy.*` -> AdventureReveal framing.

The event location is validated but V7 does not force the player to look at it. The player remains free to steer the camera.

## Mission reveals

The presentation subsystem watches the authoritative active mission ID. A change from one active mission to another triggers a short `AdventureReveal` pulse and `presentation.mission.changed` UI cue.

This is a **microbeat**, not a cutscene takeover. Source beats are bounded to 2.5 seconds maximum and contain the conceptual phases `enter -> hold -> exit`. They do not lock input, alter gameplay time scale or force a new camera actor.

## UI motion

`UWMPresentationOverlayWidget` provides a source-visible motion proof for six semantic cues:

- `presentation.build.confirm`;
- `presentation.observe.focus`;
- `presentation.science.focus`;
- `presentation.adventure.reveal`;
- `presentation.mission.changed`;
- `presentation.ecology.changed`.

Standard mode uses a small <=18 px entrance translation and 0.98 -> 1.0 scale settle together with opacity. Reduced-motion mode retains the same text/opacity communication with **zero UI translation and scale motion**.

Motion is supplementary. Meaning must remain understandable from wording, hierarchy and state even if animation is disabled.

## Reduced motion

Reduced-motion is a shared presentation setting and is forwarded to V6 VFX. For contextual camera modes it enforces these mathematical ceilings relative to Explore:

- arm-length delta <= 80 cm;
- FOV delta <= 3 degrees;
- vertical target-offset delta <= 16 cm;
- blend <= 0.18 s;
- contextual hold <= 0.70 s;
- UI translation = 0 px;
- UI scale = 1.0.

This is not a promise that motion sickness has been clinically or device-certified; it is a source-level safety constraint pending representative review.

## Authored production handoff

The manifest reserves these future assets without claiming they exist:

- `/Game/WorldMakers/Presentation/DA_WM_CameraPresentation`;
- `/Game/WorldMakers/Presentation/Sequences/LS_WM_AdventureReveal`;
- `/Game/WorldMakers/Presentation/Sequences/LS_WM_ScienceReveal`;
- `/Game/WorldMakers/UI/Presentation/DA_WM_UIMotionStyle`.

Authored Level Sequences must preserve the V7 authority boundary: optional short staging may enhance a reveal, but input locking, forced rotation and gameplay/time authority remain disallowed for normal child-facing adventure beats.

The manifest explicitly records `authoredSequenceAssetsPresent=false` and `authoredLevelSequencesPresent=false`. V7 therefore **does not claim** final cinematic assets or capture-ready trailer sequences.

## Tests and CI

Automation coverage protects:

1. all six camera profiles and source bounds;
2. reduced-motion camera deltas;
3. semantic-event camera routing;
4. UI meaning with zero movement in reduced-motion;
5. stable cue vocabulary and <=2.5 s microbeat duration.

Repository Quality additionally validates canonical/staged manifest parity, presentation-only authority, V6 delegate integration and the absence of input locking/forced view-target behavior in the V7 subsystem.

## Certification boundary

Source CI can validate state mappings, bounds, manifests and authority separation. It cannot validate perceived camera comfort, final cinematography, Level Sequence composition, UI polish or frame pacing on tablets. Those remain V8/native-production evidence.

## Next phase

**V8 — Visual Optimization & Device Certification** measures the visual stack rather than extending it speculatively: frame time, GPU/CPU cost, material/texture pressure, VFX overdraw, animation cost, camera/UI behavior and representative iPadOS/Android evidence.
