# M1 — Playable Building Prototype

## Objective

Validate the core World Makers loop before investing in production art, multiplayer, or curriculum UI:

**move → aim → preview → place → rotate → remove → undo/redo → save/load**.

This slice is intentionally built from C++ and Engine-provided placeholder geometry so gameplay behavior can be reviewed without binary Blueprint or art dependencies.

## Prototype controls

| Capability | Keyboard / mouse | Gamepad | Touch |
| --- | --- | --- | --- |
| Move | W/A/S/D | Left stick | Future virtual stick |
| Camera | Mouse | Right stick | Future drag-look |
| Place | Left click | Face button bottom | Tap places current preview |
| Rotate clockwise | E | Right shoulder | Future build HUD button |
| Rotate counter-clockwise | Q | Left shoulder | Future build HUD button |
| Remove targeted piece | R | Face button right | Future build HUD button |
| Undo | Z | UI required for child build | Future build HUD button |
| Redo | Y | UI required for child build | Future build HUD button |
| Save local prototype | F5 | Developer-only | Automatic/manual UI later |
| Load local prototype | F9 | Developer-only | Automatic/manual UI later |

The touch binding in M1 is deliberately minimal. A production child-facing tablet control scheme requires dedicated UX work and usability testing; desktop key bindings must not become the final children's interface.

## Building rules

- Default cell size: 100 Unreal units.
- Rotation step: 90 degrees.
- Placement is traced from the player's camera and snapped deterministically.
- The preview has collision disabled and is excluded from persistence.
- Placed pieces are tagged `WM_PlayerBuild`.
- M1 uses `/Engine/BasicShapes/Cube.Cube` as placeholder geometry.
- There is no resource cost, inventory loss, combat, stealing, or punitive failure.

## Persistence

`UWMWorldSaveGame` stores only:

- save format version;
- `PieceId`;
- piece transform.

It intentionally stores no child name, parent name, email, birth date, phone number, account identifier, learning record, or analytics payload.

## Undo/redo

The component maintains an in-memory command history capped at 50 entries. Placement and removal are reversible. Loading a saved world clears the command history to avoid stale actor references.

## Prototype environment

`AWMGameMode` selects `AWMPlayerCharacter` and can generate a simple collision ground from Engine geometry. This exists only to make the code-first slice testable before the first authored biome map exists. Disable `bSpawnPrototypeGround` when an authored level supplies its own terrain.

## Test strategy

### Standard GitHub runner

`scripts/validate-m1-prototype.py` checks:

- required source/config files;
- game mode registration;
- action mappings;
- required building capabilities;
- absence of common PII fields in the local build-save model.

### Unreal self-hosted runner

When `UNREAL_SELF_HOSTED_ENABLED=true` and `UNREAL_ENGINE_ROOT` are configured, CI:

1. builds `WorldMakersEditor`;
2. launches `UnrealEditor-Cmd.exe` headlessly;
3. executes `WorldMakers.Building.*` automation tests.

## M1 acceptance status

- [x] Native third-person placeholder character.
- [x] Deterministic grid snapping.
- [x] Placement preview.
- [x] Place pieces.
- [x] Rotate pieces.
- [x] Remove pieces.
- [x] Undo/redo direction implemented.
- [x] Local save/load.
- [x] Keyboard/mouse and gamepad developer controls.
- [x] Minimal tap-to-place touch path.
- [x] Automated grid/rotation tests authored.
- [x] Lightweight CI validation.
- [ ] Full UE5 compile on self-hosted runner.
- [ ] Real-device tablet interaction study.
- [ ] Production build-mode HUD and touch controls.
- [ ] Representative tablet performance capture.

## Exit criteria for M1

M1 can merge once the standard GitHub checks pass. It becomes **runtime-certified** only after the first self-hosted Unreal compile/test run succeeds. Device performance and production touch UX remain explicit follow-up work and must not be inferred from source-level CI.
