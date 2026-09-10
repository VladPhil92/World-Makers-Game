# World Makers SourceArt

This directory is the source-art namespace referenced by the P1 authored-asset manifest.

It is intentionally separate from `game/Content`: DCC sources are not runtime assets and are not staged into packaged builds.

## Coordinate contract

- units: centimeters;
- up: +Z;
- forward: +X;
- applied/frozen transforms before export;
- meaningful production pivots;
- no negative final scale.

## Expected source types

- `.blend`, `.fbx`, `.gltf` for geometry/source scenes;
- `.sbsar` and related controlled material source packages where appropriate;
- `.plan.json` / `.sequence-plan.json` for Unreal-native assets such as Animation Blueprints, Niagara systems, Level Sequences and Data Assets.

A plan file describes authoring intent; it is not the binary Unreal asset.

## Truth rule

Adding a source file here does not make an asset production-ready. The imported `.uasset` must exist at the exact `/Game/WorldMakers/...` path, pass native review, and only then may its manifest row change to `authoredPresent=true`.

Do not place child photographs, biometric scans, private voice recordings, credentials, licensed reference packs without redistribution rights, or personal data in SourceArt.
