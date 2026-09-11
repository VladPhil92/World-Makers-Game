# P4 — Authored Animation, VFX & Presentation

## Purpose

P4 replaces the last major procedural presentation stand-ins with reproducible authored source. It is intentionally split from P3 because character geometry/skinning and motion/VFX/cinematics have different review and failure modes.

P4 does not redefine gameplay authority. `CharacterMovementComponent` still moves the player, V6 remains presentation-only, and V7 keeps player control during camera microbeats.

## Animation source

P4 authors **17 animation clips** against the exact P3/V4 19-joint Child Explorer Skeleton:

- 9 locomotion states: idle, start, walk, run, stop, turn-in-place, jump, fall and land;
- 8 interactions: build-place, build-remove, build-move, measure, observe, inspect, pickup and science-manipulate.

Every clip is 30 fps, in-place and root-motion disabled. Interaction duration and layer names are inherited directly from V5 so the authored motion cannot silently drift from the runtime state/read-model contract.

The generated source contains explicit per-bone key times, Euler rotations and bounded local translations. The Blender bridge bakes those curves on the P3 armature and exports one animation FBX per clip. The Unreal handoff imports those files as `AnimSequence` assets using `SKEL_WM_ChildExplorer`.

`ABP_WM_ChildExplorer` remains a native authored target. P4 defines its state/read-model plan but does not fabricate an empty Animation Blueprint merely to mark the asset complete.

## VFX source

P4 includes **17 VFX recipes**, one for every V6 semantic event. IDs, domains, shapes and Niagara target paths must match V6 exactly.

Each recipe defines a renderer intent, Low/Mid/High spawn ceilings, lifetime, motion model, semantic color role, `User.Intensity`, `User.Direction`, and a reduced-motion scale no greater than 0.35. Collision and light renderers remain disabled in the initial tablet-first recipes.

The recipes are presentation-only. They cannot grant rewards, create mission evidence or alter simulation outcomes. Niagara systems remain native authored targets and require Unreal Editor review; P4 does not create empty Niagara systems to make `authoredPresent` appear complete.

## Presentation source

P4 copies the six V7 camera profiles without changing their values and authors source timelines for **two microsequences**:

- `LS_WM_AdventureReveal` — 2.35 s;
- `LS_WM_ScienceReveal` — 1.95 s.

Both use `enter → hold → exit`, remain below the V7 2.5 s ceiling, never lock input, never change global time scale and never force a new ViewTarget.

The six V7 UI cues receive explicit opacity, translation and scale curves. Reduced-motion still removes translation/scale meaning and preserves communication through content/opacity.

The Unreal native bridge can create the Level Sequence containers and playback ranges. Camera tracks, final composition and shot polish remain human-authored native work.

## Determinism and handoff

`content/visual/authored/p4-motion-vfx-presentation.json` pins the SHA-256 of the complete generated source bundle. Repository Quality regenerates it twice and requires byte-for-byte determinism.

The manual native workflow executes:

1. P3 source generation;
2. P3 rig FBX build in Blender;
3. P4 source generation;
4. 17 animation FBX exports in Blender;
5. temporary P3 Skeletal Mesh/Skeleton import in Unreal;
6. P4 animation import and Level Sequence container creation;
7. World Makers automation tests;
8. artifact upload for native review.

The workflow reports Niagara/AnimBP targets but deliberately does not mutate `authoredPresent`.

## Procedural fallback

P4 does not remove V5 procedural motion, V6 procedural effects or V7 runtime framing. The **procedural fallback** remains available until the corresponding native assets exist and pass review.

The P1 flags for the character, Animation Blueprint, Niagara master and presentation assets remain false during the source phase.

## Production boundary

P4 is source-complete when the deterministic bundle, Blender bridge, Unreal bridge, documentation and CI gate pass.

P4 is **not** native/art-certified until all of the following have been reviewed in Unreal Engine 5.8.2 and on representative devices:

- imported AnimSequence timing, looping, foot sliding and deformation;
- authored `ABP_WM_ChildExplorer`, state machine, slots and additive aim/look;
- authored Niagara systems and V6 semantic parameter binding;
- camera Data Asset values and reduced-motion behavior;
- Level Sequence camera tracks and visual continuity;
- UI motion legibility and accessibility;
- performance against V8 limits.

No source-plan success may automatically flip an `authoredPresent` flag.

## Next

**P5** is Art Polish & Device Certification: final materials/lighting, LOD/HLOD/culling, animation deformation polish, VFX overdraw, camera/UI comfort and V8 certification on Android and iPadOS.
