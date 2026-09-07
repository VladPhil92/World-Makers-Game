# M1.6 — Building Core

## Goal

M1.6 evolves the single-cube M1 prototype into a catalog-backed building core while preserving the M1.5 certification boundary.

## Delivered source capabilities

- text-configured build catalog under `DefaultGame.ini`;
- four prototype definitions: cube, floor, wall, pillar;
- per-piece dimensions, rotation step and support policy;
- selection and cycling without binary assets;
- placement overlap bounds derived from the selected piece dimensions;
- surface height offset derived from actual piece height;
- support-policy checks when stacking onto another build piece;
- non-destructive move mode with commit/cancel;
- move participates in undo/redo history;
- load rejects piece IDs that are not in the active catalog;
- preview exposes valid/invalid semantic state through custom-depth stencil values 1/2;
- source CI and Unreal Automation coverage for the prototype catalog.

## Prototype controls

- `Tab`: cycle piece.
- `M`: begin moving the targeted placed piece.
- `Left Mouse`: place a new piece, or commit a move when move mode is active.
- `Escape`: cancel move.
- `Q` / `E`: rotate.
- `R`: remove.
- `Z` / `Y`: undo / redo.

These are developer controls, not final child-facing tablet UX.

## Data architecture

The initial catalog uses Unreal config rather than authored Data Assets so the source contract remains reviewable and usable before the first `.uasset` pipeline exists. The runtime boundary is intentionally represented by `FWMBuildPieceSpec`/`UWMBuildCatalogSettings`; production art can later migrate definitions to `UPrimaryDataAsset` without coupling placement logic to individual actor subclasses.

## Certification state

M1.6 may be source-complete while M1.5 runtime certification remains blocked. Issue #9 still requires:

- authored certification `.umap`;
- Windows x64 UE 5.8.2 runner;
- native editor build;
- headless `WorldMakers.Building.*` tests;
- smoke/device evidence.

No source-only merge may be described as runtime certification.
