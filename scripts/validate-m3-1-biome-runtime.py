#!/usr/bin/env python3
"""Validate M3.1 data-driven biome runtime and exploration contracts."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]

CANONICAL = ROOT / "content/biomes/caribbean-rainforest/runtime.json"
PACKAGED = ROOT / "game/Content/WorldMakers/Biomes/biome.caribbean-rainforest.json"
SCHEMA = ROOT / "content/biomes/schema/biome-runtime.schema.json"

REQUIRED_FILES = [
    "game/Source/WorldMakers/Environment/WMBiomeTypes.h",
    "game/Source/WorldMakers/Environment/WMBiomeTypes.cpp",
    "game/Source/WorldMakers/Environment/WMBiomeRuntimeSubsystem.h",
    "game/Source/WorldMakers/Environment/WMBiomeRuntimeSubsystem.cpp",
    "game/Source/WorldMakers/Environment/WMExplorationComponent.h",
    "game/Source/WorldMakers/Environment/WMExplorationComponent.cpp",
    "game/Source/WorldMakers/Private/Tests/WMBiomeRuntimeTests.cpp",
    "docs/m3-1-biome-runtime-exploration.md",
]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def all_keys(value: Any) -> set[str]:
    result: set[str] = set()
    if isinstance(value, dict):
        for key, child in value.items():
            result.add(key.lower())
            result.update(all_keys(child))
    elif isinstance(value, list):
        for child in value:
            result.update(all_keys(child))
    return result


def inside(point: list[float], center: list[float], extent: list[float]) -> bool:
    return all(abs(point[index] - center[index]) <= extent[index] for index in range(3))


def main() -> None:
    for relative in REQUIRED_FILES:
        require((ROOT / relative).is_file(), f"Missing M3.1 file: {relative}")
    require(CANONICAL.is_file() and PACKAGED.is_file() and SCHEMA.is_file(), "M3.1 biome source/schema/package is incomplete")
    require(CANONICAL.read_bytes() == PACKAGED.read_bytes(), "Packaged biome runtime must be byte-equivalent to canonical runtime JSON")

    payload = json.loads(CANONICAL.read_text(encoding="utf-8"))
    schema = json.loads(SCHEMA.read_text(encoding="utf-8"))
    require(schema.get("type") == "object" and "$defs" in schema, "Biome runtime schema must define the object contract")
    require(payload.get("schemaVersion") == 1, "Biome runtime schemaVersion must be 1")
    require(payload.get("id") == "biome.caribbean-rainforest", "Canonical M3.1 biome ID must remain stable")
    require(payload.get("prototypeOnly") is True, "Caribbean Rainforest must remain prototypeOnly during M3.1")
    require(payload.get("originCm") == [0, 0, 0], "M3.1 prototype biome origin must remain deterministic")

    zones = payload.get("zones", [])
    points = payload.get("pointsOfInterest", [])
    require(len(zones) >= 3, "M3.1 requires at least three semantic zones")
    require(len(points) >= 3, "M3.1 requires at least three points of interest")

    zone_ids = [zone.get("id") for zone in zones]
    require(len(zone_ids) == len(set(zone_ids)) and all(zone_ids), "Biome zone IDs must be unique and non-empty")
    zone_by_id = {zone["id"]: zone for zone in zones}
    require(any(zone.get("buildAllowed") is True for zone in zones), "Biome must expose an explicit build-allowed zone")
    for zone in zones:
        center = zone.get("centerCm")
        extent = zone.get("extentCm")
        require(isinstance(center, list) and len(center) == 3, f"Invalid centerCm for {zone['id']}")
        require(isinstance(extent, list) and len(extent) == 3 and all(value > 0 for value in extent), f"Invalid extentCm for {zone['id']}")
        require(isinstance(zone.get("missionIds"), list), f"missionIds must be explicit for {zone['id']}")

    point_ids = [point.get("id") for point in points]
    discovery_ids = [point.get("discoveryId") for point in points]
    require(len(point_ids) == len(set(point_ids)) and all(point_ids), "POI IDs must be unique and non-empty")
    require(len(discovery_ids) == len(set(discovery_ids)) and all(discovery_ids), "Discovery IDs must be unique and non-empty")
    for point in points:
        zone_id = point.get("zoneId")
        require(zone_id in zone_by_id, f"POI {point['id']} references unknown zone {zone_id}")
        require(50 <= point.get("discoveryRadiusCm", 0) <= 2000, f"POI {point['id']} discovery radius is outside M3.1 bounds")
        require(inside(point["locationCm"], zone_by_id[zone_id]["centerCm"], zone_by_id[zone_id]["extentCm"]), f"POI {point['id']} must live inside its semantic zone")

    forbidden_keys = {"name", "email", "displayname", "freetext", "analyticsid", "advertisingid", "transaction", "receipt"}
    require(not (all_keys(payload) & forbidden_keys), "Biome runtime must remain free of identity, free-text analytics and commerce payloads")

    known_mission_ids: set[str] = set()
    for mission_path in (ROOT / "content/missions").rglob("*.json"):
        if mission_path.name == "mission.schema.json":
            continue
        known_mission_ids.add(json.loads(mission_path.read_text(encoding="utf-8"))["id"])
    for zone in zones:
        for mission_id in zone["missionIds"]:
            require(mission_id in known_mission_ids, f"Zone {zone['id']} references unknown mission {mission_id}")

    types_h = read("game/Source/WorldMakers/Environment/WMBiomeTypes.h")
    types_cpp = read("game/Source/WorldMakers/Environment/WMBiomeTypes.cpp")
    for token in ("FWMBiomeRuntimeDefinition", "FWMBiomeZoneDefinition", "FWMPointOfInterestDefinition", "FWMExplorationProgressModel", "TryParseJson", "FindZoneAtWorldLocation", "RegisterDiscovery"):
        require(token in types_h or token in types_cpp, f"M3.1 biome semantic model missing: {token}")
    require("DiscoveredIds.Contains" in types_cpp, "Discovery registration must be idempotent")

    subsystem_h = read("game/Source/WorldMakers/Environment/WMBiomeRuntimeSubsystem.h")
    subsystem_cpp = read("game/Source/WorldMakers/Environment/WMBiomeRuntimeSubsystem.cpp")
    for token in ("UWorldSubsystem", "ReloadBiomeCatalog", "ActivateBiome", "ObserveLocation", "OnZoneChanged", "OnDiscoveryRegistered", "GetNearbyPointOfInterestIds"):
        require(token in subsystem_h or token in subsystem_cpp, f"Biome runtime subsystem missing: {token}")
    require("WorldMakers/Biomes" in subsystem_cpp, "Biome runtime must discover packaged biome JSON rather than hard-code the semantic world")

    component_h = read("game/Source/WorldMakers/Environment/WMExplorationComponent.h")
    component_cpp = read("game/Source/WorldMakers/Environment/WMExplorationComponent.cpp")
    require("ObservationIntervalSeconds" in component_h and "ObserveNow" in component_h, "Exploration component must use bounded location observation")
    require("GetSubsystem<UWMBiomeRuntimeSubsystem>" in component_cpp, "Exploration component must feed the biome subsystem")
    require("WMMissionRuntimeSubsystem" not in component_h + component_cpp, "Exploration core must not couple to mission-specific runtime")

    player_h = read("game/Source/WorldMakers/Player/WMPlayerCharacter.h")
    player_cpp = read("game/Source/WorldMakers/Player/WMPlayerCharacter.cpp")
    require("UWMExplorationComponent" in player_h and "CreateDefaultSubobject<UWMExplorationComponent>" in player_cpp, "Player must own the exploration component")

    default_game = read("game/Config/DefaultGame.ini")
    require('Path="WorldMakers/Biomes"' in default_game, "Biome runtime JSON must be staged as NonUFS content")

    tests = read("game/Source/WorldMakers/Private/Tests/WMBiomeRuntimeTests.cpp")
    for test_name in (
        "WorldMakers.Exploration.Definition.ParsesRuntimeContract",
        "WorldMakers.Exploration.Geometry.ZoneContainment",
        "WorldMakers.Exploration.Discovery.IdempotentStableIds",
    ):
        require(test_name in tests, f"Missing M3.1 automation test: {test_name}")

    repo_ci = read(".github/workflows/repo-quality.yml")
    unreal_ci = read(".github/workflows/unreal-ci.yml")
    require("validate-m3-1-biome-runtime.py" in repo_ci and "validate-m3-1-biome-runtime.py" in unreal_ci, "M3.1 validator must run in both CI gates")

    docs = read("docs/m3-1-biome-runtime-exploration.md").lower()
    for boundary in ("issue #9", "stable ids", "pii", "session-local", "mission"):
        require(boundary in docs, f"M3.1 documentation boundary missing: {boundary}")

    print(f"M3.1 biome runtime validated: {len(zones)} semantic zones, {len(points)} POIs, packaged parity, exploration integration and CI contracts are present.")


if __name__ == "__main__":
    main()
