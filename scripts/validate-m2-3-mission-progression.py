#!/usr/bin/env python3
"""Validate M2.3 mission progression, persistence and learning-journey contracts."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

REQUIRED_FILES = [
    "game/Source/WorldMakers/Mission/WMMissionJourneySaveGame.h",
    "game/Source/WorldMakers/Private/Tests/WMMissionJourneyTests.cpp",
    "docs/m2-3-mission-progression.md",
]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def load_missions() -> dict[str, dict]:
    missions: dict[str, dict] = {}
    for path in sorted((ROOT / "content/missions").rglob("*.json")):
        if path.name == "mission.schema.json":
            continue
        payload = json.loads(path.read_text(encoding="utf-8"))
        mission_id = payload.get("id")
        if not mission_id:
            continue
        require(mission_id not in missions, f"Duplicate mission ID: {mission_id}")
        missions[mission_id] = payload
    return missions


def validate_dependency_graph(missions: dict[str, dict]) -> None:
    visiting: set[str] = set()
    visited: set[str] = set()

    def visit(mission_id: str) -> None:
        if mission_id in visited:
            return
        require(mission_id not in visiting, f"Mission prerequisite cycle detected at {mission_id}")
        visiting.add(mission_id)
        runtime = missions[mission_id].get("runtime", {})
        prerequisites = runtime.get("prerequisiteMissionIds", [])
        require(len(prerequisites) == len(set(prerequisites)), f"Duplicate prerequisites for {mission_id}")
        for prerequisite in prerequisites:
            require(prerequisite != mission_id, f"Mission cannot require itself: {mission_id}")
            require(prerequisite in missions, f"Unknown prerequisite {prerequisite} for {mission_id}")
            visit(prerequisite)
        visiting.remove(mission_id)
        visited.add(mission_id)

    for mission_id in missions:
        visit(mission_id)


def main() -> None:
    for relative in REQUIRED_FILES:
        require((ROOT / relative).is_file(), f"Missing M2.3 file: {relative}")

    schema = json.loads(read("content/missions/schema/mission.schema.json"))
    runtime_properties = schema["properties"]["runtime"]["properties"]
    require("prerequisiteMissionIds" in runtime_properties, "Mission schema must support prerequisiteMissionIds")

    missions = load_missions()
    validate_dependency_graph(missions)
    mission_one = "mission.mathematics.measure-and-build-01"
    mission_two = "mission.mathematics.measure-and-build-02"
    require(mission_one in missions and mission_two in missions, "M2.3 requires both mathematics missions")
    require(missions[mission_two]["runtime"].get("prerequisiteMissionIds") == [mission_one], "Mission 02 must depend on Mission 01")

    packaged_two = json.loads(read("game/Content/WorldMakers/Missions/mission.mathematics.measure-and-build-02.json"))
    require(packaged_two == missions[mission_two], "Packaged Mission 02 must match canonical source exactly")

    types_h = read("game/Source/WorldMakers/Mission/WMMissionTypes.h")
    types_cpp = read("game/Source/WorldMakers/Mission/WMMissionTypes.cpp")
    for token in (
        "EWMJourneyMissionState",
        "Locked",
        "Available",
        "FWMJourneyMissionReadModel",
        "FWMMissionJourneyModel",
        "PrerequisiteMissionIds",
        "GrantedRewardIds",
        "ApplyCompletion",
        "ResolveState",
    ):
        require(token in types_h or token in types_cpp, f"Journey model missing: {token}")
    require("CompletedMissionIds.Contains" in types_cpp, "Journey must resolve prerequisite/completion state from stable IDs")
    require("!GrantedRewardIds.Contains" in types_cpp, "Reward grants must be idempotent")

    save_h = read("game/Source/WorldMakers/Mission/WMMissionJourneySaveGame.h")
    for token in ("USaveGame", "FormatVersion", "CompletedMissionIds", "GrantedRewardIds", "LastActiveMissionId"):
        require(token in save_h, f"Journey SaveGame missing: {token}")
    forbidden_save_tokens = ("FString", "FText", "Email", "DisplayName", "Analytics", "Transaction", "Receipt", "AdId")
    for token in forbidden_save_tokens:
        require(token not in save_h, f"Journey SaveGame must remain privacy-minimized; forbidden token: {token}")

    runtime_h = read("game/Source/WorldMakers/Mission/WMMissionRuntimeSubsystem.h")
    runtime_cpp = read("game/Source/WorldMakers/Mission/WMMissionRuntimeSubsystem.cpp")
    for token in (
        "GetActivatableMissionIds",
        "GetJourneyReadModel",
        "GetJourneyMissionState",
        "GetGrantedRewardIds",
        "LoadJourneyProgress",
        "SaveJourneyProgress",
        "ValidateCatalogDependencies",
        "FinalizeMissionCompletion",
    ):
        require(token in runtime_h or token in runtime_cpp, f"Runtime journey API missing: {token}")
    for token in ("LoadGameFromSlot", "SaveGameToSlot", "DoesSaveGameExist", "Journey.CanActivate", "Journey.ApplyCompletion"):
        require(token in runtime_cpp, f"Runtime persistence/progression implementation missing: {token}")
    require("const TArray<FName> ActivatableMissionIds = GetActivatableMissionIds()" in runtime_cpp, "Mission cycling must skip locked missions")

    tests = read("game/Source/WorldMakers/Private/Tests/WMMissionJourneyTests.cpp")
    for test_name in (
        "WorldMakers.Missions.Journey.PrerequisiteUnlockOrder",
        "WorldMakers.Missions.Journey.IdempotentRewardGrants",
        "WorldMakers.Missions.Journey.ExportRestoreStableIds",
    ):
        require(test_name in tests, f"Missing M2.3 automation test: {test_name}")

    parent_contract = read("services/backend/contracts/parent-progress.openapi.yaml")
    require("missionJourney:" in parent_contract and "locked" in parent_contract and "completed" in parent_contract, "Parent progress contract must expose minimized journey projection")

    docs = read("docs/m2-3-mission-progression.md")
    for forbidden_pattern in ("daily streak", "premium currency", "raw learning evidence"):
        require(forbidden_pattern.lower() in docs.lower(), f"M2.3 boundary documentation missing: {forbidden_pattern}")

    print(f"M2.3 mission progression validated across {len(missions)} canonical missions: prerequisites, persistence, idempotent rewards and read model are wired.")


if __name__ == "__main__":
    main()
