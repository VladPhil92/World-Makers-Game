# World Makers — Visual Production Roadmap

## Purpose

This track moves World Makers from source-controlled visual proxies to an original, production-quality stylized 3D identity. It runs alongside gameplay and learning-system development and has its own acceptance criteria.

The target remains: **premium stylized 3D, readable on tablets, expressive rather than photoreal, richer than a voxel sandbox, and recognizably World Makers.**

A source-complete phase is not automatically art-certified. Binary Unreal assets, authored maps, final capture, and representative-device performance require Unreal Editor/DCC production and measured evidence.

## V1 — Visual Production Foundation / Look Development

Status: **source-complete in PR #49; native visual certification pending.**

- reproducible atmosphere, sunlight, skylight, fog and camera FOV;
- deterministic temporary avatar motion;
- V1–V8 production sequence and source gate.

Exit: source CI passes and the prototype has a reproducible atmosphere + motion layer.

## V2 — Materials & Surface Language

Status: **source-complete in PR #50; native authored-material certification pending.**

- stable material roles and Custom Primitive Data layout;
- semantic proxy coloration and build-placement feedback;
- tablet-first material/texture budgets;
- master-surface and water handoff contracts.

Exit: representative proxy and first authored meshes can share a coherent surface system without bespoke shaders per asset.

## V3 — Environment Art / Biome Production

V3 status: source-complete procedural art pass.

Status: **source-complete in PR #51; authored DCC mesh replacement and native/device art certification pending.**

- procedural render-only art for ground, terrain, bark/roots, foliage, stone and water;
- irregular ground, faceted terrain, multiple tree/canopy silhouettes, buttress roots, understory, boulders, river and hero ceiba;
- hidden Engine primitives retained only for collision/fallback;
- quality-tier tessellation, environment-family manifest and V3 gate.

Exit: no major visible rainforest family depends on `/Engine/BasicShapes/*` in the active V3 render path.

## V4 — Character Art & Rig

Status: **source-complete implementation on `feat/v4-character-art-rig`; authored Skeletal Mesh/skin/IK certification pending.**

Goal: replace the six-part visible primitive child proxy with an original, animation-ready avatar foundation while preserving gameplay movement and collision authority.

Implemented source/runtime contract:

- `ChildExplorerV1` default silhouette at 158 cm and exactly 5 heads tall;
- neutral child base with no gender-default marker;
- original procedural torso, faceted head, clustered hair, segmented arms/hands, legs and readable feet;
- 19 stable rig joints from `root` through head, arms and legs;
- V1 gait redirected through the V4 rig pivots for source-visible motion proof;
- 8 stable customization slots: body, hair, top, bottom, footwear, head accessory, back accessory and hand prop;
- stable head/back/left-hand/right-hand socket intent;
- `ACharacter::GetMesh()` reserved as the production Skeletal Mesh destination;
- automatic visual-path selection between procedural V4 art, authored Skeletal Mesh and legacy six-piece fallback;
- Low/Mid/High triangle, bone, influence and material-slot budgets;
- canonical/staged avatar-rig manifest with authored asset targets;
- explicit privacy boundary: no biometric capture, photo-avatar generation or child face/voice training;
- Unreal Automation tests for proportions, rig hierarchy and procedural geometry budgets;
- dedicated Repository Quality V4 gate.

Authored production target:

- `/Game/WorldMakers/Characters/Player/SK_WM_ChildExplorer`;
- `/Game/WorldMakers/Characters/Player/SKEL_WM_ChildExplorer`;
- production UVs, skin weights and modular clothing/hair meshes;
- IK Rig and Physics Asset matching the 19-joint semantic chains;
- facial strategy appropriate to the tablet performance tier;
- measured LOD, skinning and draw-call evidence;
- preserve `CharacterMovementComponent` and gameplay capsule authority.

Exit: the source avatar has a stable character silhouette, rig vocabulary, customization/attachment contract and skeletal handoff without changing gameplay collision semantics. Final character-art certification still requires authored binary assets and native Unreal review.

See `docs/v4-character-art-rig.md` and `content/visual/character/avatar-rig-v4.json`.

## V5 — Character & Interaction Animation

V5 status: source-complete animation-runtime pass.

Status: **source-complete on `feat/v5-character-interaction-animation`; authored clips/AnimBP/IK and native-device animation certification pending.**

Goal: make the character feel responsive now while defining an asset-agnostic animation state/read-model that production clips can consume later.

Implemented source/runtime contract:

- deterministic `FWMCharacterAnimationRuntime` independent from animation assets;
- `UWMCharacterAnimationComponent` bridge from `CharacterMovementComponent`, controller view and successful gameplay interactions;
- nine stable locomotion states: idle, start, walk, run, stop, turn-in-place, jump, fall and land;
- speed-normalized gait phase with explicit start/stop transitions;
- jump/fall separation from vertical velocity and bounded landing compression;
- additive head look from controller yaw/pitch divergence;
- bounded source pose across root, pelvis, chest, head, arms, forearms, hands, thighs, calves and feet;
- eight stable interaction actions: build-place, build-remove, build-move, measure, observe, inspect, pickup and science-manipulate;
- successful gameplay actions trigger upper-body interaction layers without taking movement authority;
- explicit Jump press/release binding; animation still derives airborne state from `CharacterMovementComponent`;
- compact AnimBP read-model: locomotion state, speed, airborne, state age, aim yaw/pitch, interaction action and interaction alpha;
- canonical/staged `character-animation-v5.json` production manifest;
- reserved `ABP_WM_ChildExplorer`, animation-clip and interaction-montage asset paths;
- Low/Mid/High pose-rate/layer planning budgets;
- no camera-shake dependency, commerce animation hook, biometric mocap requirement or manipulative celebration loop;
- five Unreal Automation tests plus dedicated Repository Quality V5 gate;
- V1 `FWMPrototypeMotionStyle` retained only for the explicit legacy six-piece rollback path required by earlier contracts.

Authored production target:

- create locomotion clips and transitions for the nine-state vocabulary;
- create `ABP_WM_ChildExplorer` consuming the V5 component read-model;
- use upper-body slots/montages for build, measure, observe, pickup and science actions;
- preserve root-motion-off gameplay authority unless a future isolated cinematic explicitly opts in;
- add foot placement/retargeting through `IKR_WM_ChildExplorer`;
- implement reduced-motion scaling without changing gameplay success or timing;
- profile update rate, blend cost, skinning and foot sliding on representative devices.

Exit: source-visible character motion is governed by the V5 state machine and semantic action layers, while production animation has a stable handoff contract. Final animation certification still requires authored binary clips, AnimBP/IK review, native Unreal execution and tablet evidence.

See `docs/v5-character-interaction-animation.md` and `content/visual/character/character-animation-v5.json`.

## V6 — VFX & Fantastic/Scientific Feedback

V6 status: source-complete causal VFX pass.

Status: **source-complete on `feat/v6-vfx-scientific-fantastic-feedback`; authored Niagara systems and native/device VFX certification pending.**

Goal: communicate causality and wonder visually without allowing spectacle to become a substitute for gameplay or scientific truth.

Implemented source/runtime contract:

- `FWMVFXRuntime` with stable semantics, style resolution and Low/Mid/High acceptance budgets;
- 17 semantic effects across gameplay, chemistry, physics, biology, ecology and fantasy;
- procedural source fallback using original ring, halo, burst and directional-chevron geometry;
- collision-free short-lived `AWMProceduralVFXActor` instances;
- `UWMVFXSubsystem` with active-effect and events-per-second limits;
- reduced-motion mode that preserves meaning while clamping effect travel to at most 35%;
- neutral subscription to `UWMBuildWorldStateSubsystem::OnBuildWorldChanged` and snapshot classification for place/remove/move feedback;
- neutral subscription to `UWMEnvironmentStateSubsystem::OnEnvironmentStateChanged` for recovery/stress feedback;
- `FWMScienceVFXAdapter` mapping accepted dissolution, filtration, reaction, force, circuit, cell, plant and environment results into bounded visual events;
- scientific intensity/direction derived from upstream quantities instead of arbitrary spectacle;
- distinct saturation semantics rather than treating all dissolution identically;
- explicit semantic hooks for measurement/observation and fantasy portal/rune feedback;
- canonical/staged `vfx-feedback-v6.json` manifest with reserved `/Game/WorldMakers/VFX/NS_WM_*` authored targets;
- no reward/progression authority, no evidence authority, no child-authored payload and no real-world chemistry procedure;
- no required flash, camera shake or commerce feedback loops;
- five Unreal Automation tests plus dedicated Repository Quality V6 gate.

Authored production target:

- create Niagara systems for the 17 stable semantic IDs without changing their causal meaning;
- preserve normalized intensity and direction as inputs from the V6 runtime;
- keep saturation visually distinct from ordinary dissolution;
- provide shape/contrast redundancy for ecology recovery/stress rather than hue alone;
- keep a lightweight procedural or Niagara fallback for Low tier;
- cap translucent overdraw and particle counts by visual tier;
- implement reduced-motion variants before accessibility certification;
- profile GPU cost, overdraw and concurrent effect bursts on representative tablets.

Exit: important construction/ecology events already have source-visible causal feedback, science systems have a deterministic adapter into the same vocabulary, and Niagara production has a stable handoff contract. Final VFX-art certification still requires authored binary systems, native Unreal review and representative-device evidence.

See `docs/v6-vfx-scientific-fantastic-feedback.md` and `content/visual/vfx/vfx-feedback-v6.json`.

## V7 — Camera, Cinematics & UI Motion

V7 status: source-complete presentation-runtime pass.

Status: **source-complete on `feat/v7-camera-cinematics-ui-motion`; authored Level Sequences, final UI motion art and native/device comfort certification pending.**

Goal: establish one presentation grammar that frames exploration, building, science and adventure without taking control away from the player.

Implemented source/runtime contract:

- `FWMPresentationRuntime` with six camera modes: Explore, Build, Observe, Science, Dialogue and AdventureReveal;
- bounded camera profiles for arm length, FOV, target offset, blend and hold duration;
- `UWMPresentationSubsystem` as a tickable world-level coordinator for the local spring arm/camera;
- spring-arm collision preserved; camera lag and rotation lag disabled in the source path to avoid secondary oscillation;
- semantic downstream integration through V6 `OnVFXAccepted`, with no V6 -> V7 dependency;
- build, observation, science/ecology and fantasy events select contextual framing without forcing controller rotation;
- active-mission changes trigger short AdventureReveal presentation beats;
- source microbeats limited to `enter -> hold -> exit` and <=2.5 seconds;
- no `SetViewTarget`, input lock, forced yaw/pitch, gameplay time dilation or camera-shake requirement;
- `UWMPresentationOverlayWidget` with six stable semantic UI cues;
- source UI motion uses opacity plus <=18 px translation and 0.98 -> 1.0 scale settle;
- reduced-motion removes UI translation/scale motion while preserving text and opacity communication;
- reduced-motion camera limits of <=80 cm arm delta, <=3 degree FOV delta, <=16 cm offset delta, <=0.18 s blend and <=0.70 s contextual hold;
- reduced-motion preference is forwarded to the V6 VFX subsystem;
- canonical/staged `presentation-camera-ui-v7.json` manifest;
- reserved camera data, Level Sequence and UI motion authored targets without claiming binary assets exist;
- five Unreal Automation tests plus dedicated Repository Quality V7 gate.

Authored production target:

- create `/Game/WorldMakers/Presentation/DA_WM_CameraPresentation` using the six-mode contract;
- author short adventure/science Level Sequences without normal-play input takeover;
- create final UI presentation motion from the six semantic cue IDs;
- preserve readable no-motion states for every cue;
- add screenshot/trailer capture presets only after final authored environments/characters exist;
- perform native camera collision, framing and motion-sickness review;
- verify UI animation/frame pacing and camera transitions on representative tablets.

Exit: gameplay, science, VFX, mission changes and UI share a source-controlled presentation language while child control remains authoritative. Final cinematic/UI certification still requires authored binary sequences, native Unreal review and representative-device comfort evidence.

See `docs/v7-camera-cinematics-ui-motion.md` and `content/visual/presentation/presentation-camera-ui-v7.json`.

## V8 — Visual Optimization & Device Certification

V8 status: source-complete certification infrastructure.

Status: **source-complete on `feat/v8-visual-optimization-device-certification`; actual device certification remains blocked until representative evidence passes.**

Goal: turn V1–V7 into a measurable visual release contract rather than assuming desktop/editor quality translates to tablets.

Implemented source/certification contract:

- canonical/staged Low/Mid/High visual budgets aligned with 30/30/60 FPS targets;
- p95 total frame, game-thread, render-thread and GPU ceilings;
- peak draw-call, visible-triangle, resident-texture and active-VFX ceilings;
- four mandatory stress routes: rainforest exploration, dense construction, science/VFX burst and adventure reveal;
- minimum 1,800 frame samples per scenario;
- deterministic `FWMVisualCertificationEvaluator` with a fail-closed aggregate verdict;
- closed device-evidence schema with no serial number, device ID, advertising ID, account ID, child profile or biometric fields;
- capture and screenshot SHA-256 integrity verification;
- explicit human review for scene readability, UI legibility, camera comfort and critical rendering artifacts;
- certification requires at least one representative iPadOS package and one representative Android package;
- source CI cannot self-certify;
- `assess-v8-visual-certification.py` with empty-evidence, one-platform, passing dual-platform and over-budget self-tests;
- manual self-hosted Unreal certification workflow using `--require-certified`;
- three Unreal Automation tests covering passing sample, minimum-sample failure and single-metric failure;
- dedicated Repository Quality V8 source gate.

Optimization order:

- reduce avoidable draw calls/material slots and translucent overdraw first;
- use bounded foliage/shadow scalability from M3.7;
- reduce VFX concurrency/complexity without losing semantic feedback;
- reduce texture residency and author proper LOD/HLOD/culling;
- reduce visible triangles only where silhouette remains intact;
- reduce screen percentage only within the existing tablet profile contract.

Exit for source phase: budgets, evaluator, evidence integrity, dual-platform requirement, self-tests and manual certification workflow are versioned and source CI passes.

Exit for actual certification: the manual V8 workflow returns `CERTIFIED` for real, hash-verified representative iPadOS and Android evidence on the captured build commit. Until that happens, visual/device certification remains blocked.

See `docs/v8-visual-optimization-device-certification.md`, `content/visual/certification/visual-certification-v8.json` and `scripts/assess-v8-visual-certification.py`.

## Production principle

**Silhouette → composition → motion → material → effects → detail.**

Surface fidelity must not be used to compensate for weak silhouettes, unreadable scenes or inert movement. World Makers should feel alive at the lowest viable visual tier before higher-end rendering features are layered on top.
