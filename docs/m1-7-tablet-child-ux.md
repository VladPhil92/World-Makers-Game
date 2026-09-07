# M1.7 — Tablet Interaction & Child UX

## Purpose

M1.7 removes the prototype behavior where any touch placed a building piece and introduces an explicit, reversible interaction shell for tablet-oriented usability testing.

## Interaction contract

- A viewport tap by itself does **not** mutate the world.
- A viewport drag only becomes camera input after a configurable 24-pixel dead zone.
- Large on-screen controls expose the building actions explicitly.
- Placement confirmation is disabled when the current preview is invalid.
- Move mode exposes separate confirm and cancel actions.
- Existing keyboard/gamepad controls remain available for desktop development.

## Prototype build toolbar

Two compact rows expose:

- previous/next piece;
- rotate left/right;
- build / move here;
- move existing piece;
- remove;
- undo;
- redo;
- cancel move.

Touch targets use a 112 x 72 Slate-unit baseline. This is an engineering usability baseline, not final visual design.

## Localization

Child-facing toolbar and status strings use Unreal `LOCTEXT` entries. Production should move piece/content naming to the project's localization pipeline rather than relying on prototype IDs or raw config strings.

## Safety and cognition

The interaction model favors explicit actions and reversibility. A child cannot place an object simply by touching the exploration viewport, and move transactions preserve the original object until confirmation.

## Remaining device work

M1.7 is **not tablet-certified**. Before claiming tablet readiness, run representative iPadOS/Android device tests covering:

- touch delivery through UMG versus game input;
- camera drag sensitivity and handedness;
- UI scaling at supported resolutions/aspect ratios;
- motor-accessibility target sizing;
- virtual locomotion controls;
- orientation policy;
- accidental activation rate;
- frame time and memory while the HUD is active.

Runtime certification remains blocked by Issue #9.
