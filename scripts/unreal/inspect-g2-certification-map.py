#!/usr/bin/env python3
"""Inspect the G2 authored certification map and required rainforest assets in Unreal."""

from __future__ import annotations

import json
import os
from pathlib import Path
import traceback

import unreal

MAP_PACKAGE = "/Game/WorldMakers/Maps/WM_PrototypeCertification"
EXPECTED_GAME_MODE = "/Script/WorldMakers.WMGameMode"
REPO_ROOT = Path(__file__).resolve().parents[2]
CONTRACT_PATH = REPO_ROOT / "content" / "production" / "g2-authored-vertical-slice-v1.json"
REGISTRY_PATH = REPO_ROOT / "content" / "visual" / "authored" / "authored-assets-p1.json"
REPORT = Path(
    os.environ.get(
        "WM_G2_INSPECTION_REPORT",
        str(REPO_ROOT / "artifacts" / "g2-authored" / "g2-native-map-inspection.json"),
    )
)


def read_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def write_report(status: str, blockers: list[str], **extra: object) -> None:
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(
        json.dumps(
            {
                "schema": "worldmakers.g2-native-map-inspection.v1",
                "status": status,
                "blockers": blockers,
                **extra,
            },
            indent=2,
        ),
        encoding="utf-8",
    )


def class_path(obj) -> str:
    if obj is None:
        return ""
    return obj.get_path_name()


def main() -> None:
    contract = read_json(CONTRACT_PATH)
    registry = read_json(REGISTRY_PATH)
    blockers: list[str] = []

    map_path = contract["map"]["packagePath"]
    if map_path != MAP_PACKAGE:
        blockers.append(f"Contract map drift: expected {MAP_PACKAGE}, found {map_path}")
    expected_game_mode = contract["worldSettings"]["gameModeClass"]
    if expected_game_mode != EXPECTED_GAME_MODE:
        blockers.append(f"Contract GameMode drift: expected {EXPECTED_GAME_MODE}, found {expected_game_mode}")

    if blockers:
        write_report("blocked", blockers, mapPackagePath=map_path)
        raise RuntimeError(" | ".join(blockers))

    if not unreal.EditorAssetLibrary.does_asset_exist(MAP_PACKAGE):
        blockers.append(f"Authored map is missing: {MAP_PACKAGE}")
        write_report("blocked", blockers, mapPackagePath=MAP_PACKAGE)
        raise RuntimeError(blockers[0])

    if not unreal.EditorLevelLibrary.load_level(MAP_PACKAGE):
        blockers.append(f"Unable to load authored map: {MAP_PACKAGE}")
        write_report("blocked", blockers, mapPackagePath=MAP_PACKAGE)
        raise RuntimeError(blockers[0])

    world = unreal.EditorLevelLibrary.get_editor_world()
    world_settings = world.get_world_settings() if world else None
    configured_game_mode = world_settings.get_editor_property("default_game_mode") if world_settings else None
    configured_game_mode_path = class_path(configured_game_mode)
    if configured_game_mode_path != EXPECTED_GAME_MODE:
        blockers.append(
            f"WorldSettings default GameMode mismatch: expected {EXPECTED_GAME_MODE}, found {configured_game_mode_path or '<none>'}"
        )

    all_actors = unreal.EditorLevelLibrary.get_all_level_actors()
    actor_records = []
    for actor_contract in contract["requiredAuthoredActors"]:
        label = actor_contract["label"]
        expected_class = actor_contract["classPath"]
        matches = [actor for actor in all_actors if actor.get_actor_label() == label]
        if len(matches) != 1:
            blockers.append(f"Expected exactly one actor labelled {label}; found {len(matches)}")
            continue
        actor = matches[0]
        actual_class = actor.get_class().get_path_name()
        if actual_class != expected_class:
            blockers.append(f"Actor {label} class mismatch: expected {expected_class}, found {actual_class}")
        actor_records.append({"label": label, "classPath": actual_class})

    assets_by_id = {entry["id"]: entry for entry in registry.get("assets", [])}
    asset_records = []
    for asset_id in contract["requiredAuthoredEnvironmentAssetIds"]:
        entry = assets_by_id.get(asset_id)
        if entry is None:
            blockers.append(f"Required asset ID missing from P1 registry: {asset_id}")
            continue

        object_path = entry["objectPath"]
        authored_present = bool(entry.get("authoredPresent"))
        loaded = unreal.load_object(None, object_path)
        loadable = loaded is not None
        if not authored_present:
            blockers.append(f"Required G2 asset is not approved authoredPresent=true: {asset_id}")
        if not loadable:
            blockers.append(f"Required G2 asset is not loadable in Unreal: {object_path}")

        asset_records.append(
            {
                "id": asset_id,
                "objectPath": object_path,
                "authoredPresent": authored_present,
                "loadable": loadable,
                "nativeClass": loaded.get_class().get_path_name() if loaded else None,
            }
        )

    status = "passed" if not blockers else "blocked"
    write_report(
        status,
        blockers,
        mapPackagePath=MAP_PACKAGE,
        gameModeClass=configured_game_mode_path,
        actors=actor_records,
        environmentAssets=asset_records,
    )

    if blockers:
        raise RuntimeError("G2 native inspection blocked: " + " | ".join(blockers))

    unreal.log("World Makers G2 native map inspection: PASS")


try:
    main()
except Exception as exc:
    if not REPORT.exists():
        write_report("failed", [str(exc)], traceback=traceback.format_exc())
    unreal.log_error(f"G2 native inspection failed: {exc}")
    raise
