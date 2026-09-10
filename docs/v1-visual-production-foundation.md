# V1 — Visual Production Foundation

## Objective

V1 creates the first reusable look-development and motion layer for World Makers while the visible environment and avatar are still source-controlled proxy geometry.

It deliberately improves **presentation behavior before asset fidelity**. V2–V5 will replace proxy surfaces, meshes and avatar geometry with authored production assets.

## Atmosphere and lighting

`AWMCaribbeanRainforestPrototype` now owns a compact look-development rig:

- `UDirectionalLightComponent` — primary warm sunlight;
- `USkyLightComponent` — ambient sky contribution;
- `USkyAtmosphereComponent` — atmospheric sky foundation;
- `UExponentialHeightFogComponent` — depth separation and perimeter atmosphere.

The rig is driven by `UWMVisualProfileSettings`, not by unrelated hard-coded values in gameplay code.

`FWMAtmosphereLook` exposes:

- sun intensity;
- skylight intensity;
- fog density;
- fog height falloff;
- gameplay camera field of view.

The first Caribbean look remains deliberately restrained so the quiet building clearing is readable and atmosphere does not become gameplay obstruction.

## Camera look

The follow camera consumes `CameraFOVDegrees` from the same visual profile. This gives look-development and later device tiers one authoritative place to tune scene scale/readability without changing player controls.

## Temporary procedural avatar motion

The existing six-piece avatar is still a proxy, but it is no longer visually inert.

`FWMPrototypeMotionStyle` converts normalized movement speed, gait phase and airborne state into a small deterministic pose:

- opposing arm/leg swing;
- body and head bob;
- restrained forward lean;
- distinct airborne pose.

`AWMPlayerCharacter::Tick` applies that pose to the six primitive components. This allows evaluation of cadence, camera relationship and child-like silhouette in motion before the V4 skeletal mesh and V5 animation graph exist.

This is intentionally not a substitute for production rigging. V4/V5 remove the primitive-proxy motion path from production use.

## What V1 does not claim

V1 does **not** claim that World Makers now has production-ready graphics.

The following remain subsequent visual-production work:

- authored master materials and material instances (V2);
- authored rainforest meshes, foliage, rocks, terrain and real water surface (V3);
- production child avatar, skeleton and modular customization (V4);
- authored locomotion and interaction animation graph (V5);
- Niagara / scientific / fantastic visual effects (V6);
- cinematic and UI motion language (V7);
- device-certified rendering/performance (V8).

The current `Cube`, `Sphere` and `Cylinder` meshes remain explicit proxies.

## Verification contract

V1 source verification requires:

1. sane look-development parameter ranges;
2. atmospheric components present in the rainforest visual slice;
3. camera FOV sourced from the visual profile;
4. deterministic/bounded procedural proxy motion;
5. automation tests for idle, locomotion, malformed inputs and airborne motion;
6. preservation of the art guide's production-asset boundary;
7. Repository Quality execution of the V1 source gate.

Native visual approval still requires Unreal rendering and representative-device evidence. Source CI can prove contracts and compilation lineage, but it cannot prove final art quality by itself.
