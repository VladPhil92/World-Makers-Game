#!/usr/bin/env python3
"""Validate P1 authored-asset production pipeline contracts."""

from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CANONICAL = ROOT / "content/visual/authored/authored-assets-p1.json"
PACKAGED = ROOT / "game/Content/WorldMakers/Visual/Authored/authored-assets-p1.json"
SCHEMA = ROOT / "content/visual/authored/authored-assets-p1.schema.json"

REQUIRED_FILES = (
    "game/Source/WorldMakers/Visual/WMAuthoredAssetTypes.h",
    "game/Source/WorldMakers/Visual/WMAuthoredAssetTypes.cpp",
    "game/Source/WorldMakers/Visual/WMAuthoredAssetSubsystem.h",
    "game/Source/WorldMakers/Visual/WMAuthoredAssetSubsystem.cpp",
    "game/Source/WorldMakers/Visual/WMAuthoredVisualBridgeSubsystem.h",
    "game/Source/WorldMakers/Visual/WMAuthoredVisualBridgeSubsystem.cpp",
    "game/Source/WorldMakers/Private/Tests/WMAuthoredAssetPipelineTests.cpp",
    "docs/p1-authored-asset-production-pipeline.md",
    "scripts/report-p1-authored-assets.py",
)

KIND_PREFIX = {
    "StaticMesh": "SM_",
    "SkeletalMesh": "SK_",
    "Material": "M_",
    "NiagaraSystem": "NS_",
    "AnimationBlueprint": "ABP_",
    "LevelSequence": "LS_",
    "DataAsset": "DA_",
}

ENVIRONMENT_SET = {
    "environment.rainforest.ground.a",
    "environment.rainforest.terrain.a",
    "environment.rainforest.tree.a",
    "environment.rainforest.tree.b",
    "environment.rainforest.tree.c",
    "environment.rainforest.understory.a",
    "environment.rainforest.rock.a",
    "environment.rainforest.water-edge.a",
    "environment.rainforest.hero-ceiba",
}


def fail(message: str) -> None:
    raise SystemExit(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def object_path_to_uasset(object_path: str) -> Path:
    package, _, _object = object_path.partition(".")
    relative = package.removeprefix("/Game/") + ".uasset"
    return ROOT / "game/Content" / relative


def main() -> None:
    missing = [path for path in REQUIRED_FILES if not (ROOT / path).is_file()]
    require(not missing, f"Missing P1 source files: {missing}")
    require(CANONICAL.is_file() and PACKAGED.is_file() and SCHEMA.is_file(), "P1 manifest/schema/package is incomplete")
    require(CANONICAL.read_bytes() == PACKAGED.read_bytes(), "P1 staged manifest must be byte-equivalent to canonical JSON")

    payload = json.loads(CANONICAL.read_text(encoding="utf-8"))
    schema = json.loads(SCHEMA.read_text(encoding="utf-8"))
    require(payload.get("schemaVersion") == 1, "P1 schemaVersion must be 1")
    require(payload.get("units") == "centimeters", "P1 DCC contract must use centimeters")
    require(payload.get("upAxis") == "Z" and payload.get("forwardAxis") == "X", "P1 coordinate system must be +Z up / +X forward")
    require(schema.get("additionalProperties") is False, "P1 manifest schema must reject undeclared root fields")
    require(schema.get("properties", {}).get("assets", {}).get("items", {}).get("additionalProperties") is False, "P1 asset entries must reject undeclared fields")

    required_formats = {"fbx", "gltf", "blend", "sbsar", "json"}
    require(required_formats.issubset(set(payload.get("sourceFormats", []))), "P1 sourceFormats must cover DCC, material and plan artifacts")

    assets = payload.get("assets")
    require(isinstance(assets, list) and len(assets) >= 16, "P1 must define the initial environment/character/material/VFX/presentation production queue")
    by_id = {asset.get("id"): asset for asset in assets if isinstance(asset, dict)}
    require(len(by_id) == len(assets), "P1 authored asset IDs must be unique")
    require(ENVIRONMENT_SET.issubset(by_id), "P1 rainforest authored set is incomplete")
    require("character.player.child-explorer" in by_id, "P1 character skeletal handoff is missing")

    object_paths: set[str] = set()
    for asset_id, asset in by_id.items():
        kind = asset.get("kind")
        object_path = asset.get("objectPath", "")
        source_path = asset.get("sourcePath", "")
        require(kind in KIND_PREFIX, f"Unknown P1 asset kind: {asset_id}")
        require(re.fullmatch(r"[a-z0-9][a-z0-9.-]+", asset_id or "") is not None, f"Invalid stable asset ID: {asset_id}")
        require(object_path.startswith("/Game/WorldMakers/") and "." in object_path, f"Asset must live under /Game/WorldMakers: {asset_id}")
        require("/Engine/" not in object_path and "/Game/StarterContent/" not in object_path, f"Placeholder namespace cannot be authored production art: {asset_id}")
        require(object_path not in object_paths, f"Duplicate objectPath: {object_path}")
        object_paths.add(object_path)

        object_name = object_path.rsplit(".", 1)[-1]
        require(object_name.startswith(KIND_PREFIX[kind]), f"{asset_id} must use {KIND_PREFIX[kind]} naming for {kind}")
        require(source_path.startswith("SourceArt/WorldMakers/"), f"Source-art path escaped World Makers namespace: {asset_id}")
        require(".." not in Path(source_path).parts, f"Source-art path may not traverse directories: {asset_id}")
        require(asset.get("collisionPolicy") in {"none", "proxy", "authored-simple"}, f"Invalid collision policy: {asset_id}")
        require(asset.get("fallbackMode") in {"procedural", "legacy", "none"}, f"Invalid fallback mode: {asset_id}")
        require(1 <= int(asset.get("minLods", 0)) <= 8, f"Invalid LOD floor: {asset_id}")
        require(1 <= int(asset.get("maxMaterialSlots", 0)) <= 4, f"Invalid material-slot ceiling: {asset_id}")
        require(isinstance(asset.get("authoredPresent"), bool), f"authoredPresent must be explicit: {asset_id}")

        if asset["authoredPresent"]:
            uasset = object_path_to_uasset(object_path)
            require(uasset.is_file(), f"{asset_id} is marked authoredPresent=true but {uasset.relative_to(ROOT)} does not exist")

    env_present = {asset_id for asset_id in ENVIRONMENT_SET if by_id[asset_id]["authoredPresent"]}
    require(not env_present or env_present == ENVIRONMENT_SET, "Rainforest authored path is all-or-nothing in P1; partial environment activation is forbidden")

    types_h = read("game/Source/WorldMakers/Visual/WMAuthoredAssetTypes.h")
    types_cpp = read("game/Source/WorldMakers/Visual/WMAuthoredAssetTypes.cpp")
    subsystem_h = read("game/Source/WorldMakers/Visual/WMAuthoredAssetSubsystem.h")
    subsystem_cpp = read("game/Source/WorldMakers/Visual/WMAuthoredAssetSubsystem.cpp")
    bridge_h = read("game/Source/WorldMakers/Visual/WMAuthoredVisualBridgeSubsystem.h")
    bridge_cpp = read("game/Source/WorldMakers/Visual/WMAuthoredVisualBridgeSubsystem.cpp")

    for token in (
        "FWMAuthoredVisualAssetDefinition",
        "FWMAuthoredVisualAssetCatalog",
        "bAuthoredPresent",
        "CanAttemptLoad",
        "/Game/WorldMakers/",
        "SourceArt/WorldMakers/",
    ):
        require(token in types_h or token in types_cpp, f"P1 authored type contract missing: {token}")

    for token in (
        "UGameInstanceSubsystem",
        "WorldMakers/Visual/Authored/authored-assets-p1.json",
        "FSoftObjectPath",
        "GetNumLODs",
        "GetStaticMaterials().Num()",
        "GetLODNum",
        "GetMaterials().Num()",
    ):
        require(token in subsystem_h or token in subsystem_cpp, f"P1 loader missing runtime contract: {token}")

    for token in (
        "UTickableWorldSubsystem",
        "character.player.child-explorer",
        "AuthoredGroundArt",
        "AuthoredTreeArtA",
        "AuthoredTreeArtB",
        "AuthoredTreeArtC",
        "AuthoredUnderstoryArt",
        "AuthoredHeroCeibaArt",
        "SetCollisionEnabled(ECollisionEnabled::NoCollision)",
        "RefreshAvatarVisualPath",
        "SetProceduralEnvironmentVisible(Biome, false)",
        "SetAuthoredEnvironmentVisible(Biome, true)",
    ):
        require(token in bridge_h or token in bridge_cpp, f"P1 bridge missing handoff contract: {token}")

    forbidden_authority = ("RecordComposableEvidence", "GrantReward", "CompleteMission", "AddCurrency", "Purchase")
    bridge_source = subsystem_cpp + "\n" + bridge_cpp
    for token in forbidden_authority:
        require(token not in bridge_source, f"Authored asset pipeline must remain presentation-only; found authority token: {token}")

    tests = read("game/Source/WorldMakers/Private/Tests/WMAuthoredAssetPipelineTests.cpp")
    for name in (
        "WorldMakers.Visual.AuthoredAssets.DefinitionGuardsNamespaceAndFallback",
        "WorldMakers.Visual.AuthoredAssets.CatalogRequiresUniqueStableIds",
        "WorldMakers.Visual.AuthoredAssets.BudgetsStayTabletBounded",
    ):
        require(name in tests, f"Missing P1 automation test: {name}")

    default_game = read("game/Config/DefaultGame.ini")
    require('+DirectoriesToAlwaysStageAsNonUFS=(Path="WorldMakers/Visual")' in default_game, "P1 runtime manifest must remain staged")

    docs = read("docs/p1-authored-asset-production-pipeline.md").lower()
    for token in (
        "centimeters",
        "+z up",
        "+x forward",
        "authoredpresent",
        "all-or-nothing",
        "collision",
        "lod",
        "material slots",
        "skeletal mesh",
        "sourceart",
        "unreal editor",
        "not art-certified",
    ):
        require(token in docs, f"P1 documentation boundary missing: {token}")

    workflow = read(".github/workflows/repo-quality.yml")
    require("python scripts/validate-p1-authored-assets.py" in workflow, "Repository Quality must execute the P1 authored-assets gate")

    print(f"P1 authored asset production contract validated: {len(assets)} queued assets, namespace/naming/LOD/material rules, safe fallbacks, runtime bridge and honest presence flags are wired.")


if __name__ == "__main__":
    main()
