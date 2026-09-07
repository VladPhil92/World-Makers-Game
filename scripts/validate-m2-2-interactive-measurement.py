#!/usr/bin/env python3
"""Validate M2.2 interactive measurement and multi-mission source contracts."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

REQUIRED_FILES = [
    "game/Source/WorldMakers/Mission/WMMissionMeasurementComponent.h",
    "game/Source/WorldMakers/Mission/WMMissionMeasurementComponent.cpp",
    "game/Source/WorldMakers/Private/Tests/WMMissionMeasurementTests.cpp",
    "docs/m2-2-interactive-measurement.md",
    "content/missions/math/example-measure-and-build-short-span.json",
    "game/Content/WorldMakers/Missions/mission.mathematics.measure-and-build-02.json",
]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def main() -> None:
    for relative in REQUIRED_FILES:
        require((ROOT / relative).is_file(), f"Missing M2.2 file: {relative}")

    measurement_h = read("game/Source/WorldMakers/Mission/WMMissionMeasurementComponent.h")
    measurement_cpp = read("game/Source/WorldMakers/Mission/WMMissionMeasurementComponent.cpp")
    geometry_h = read("game/Source/WorldMakers/Mission/WMMissionGeometryActor.h")
    geometry_cpp = read("game/Source/WorldMakers/Mission/WMMissionGeometryActor.cpp")
    runtime_h = read("game/Source/WorldMakers/Mission/WMMissionRuntimeSubsystem.h")
    runtime_cpp = read("game/Source/WorldMakers/Mission/WMMissionRuntimeSubsystem.cpp")
    player_cpp = read("game/Source/WorldMakers/Player/WMPlayerCharacter.cpp")
    hud_cpp = read("game/Source/WorldMakers/UI/WMBuildHUDWidget.cpp")
    input_ini = read("game/Config/DefaultInput.ini")
    tests_cpp = read("game/Source/WorldMakers/Private/Tests/WMMissionMeasurementTests.cpp")

    for token in (
        "EWMMissionMeasurementState",
        "AwaitingFirstPoint",
        "AwaitingSecondPoint",
        "FWMMissionMeasurementModel",
        "CapturePointFromView",
        "ResetMeasurement",
    ):
        require(token in measurement_h, f"Interactive measurement contract missing: {token}")

    for token in (
        "LineTraceSingleByChannel",
        "IsWorldPointInsideBox",
        "Model.CapturePoint",
        "RecordMeasurement(CompletedDistanceCm)",
        "SetInteractiveMeasurementStart",
        "SetInteractiveMeasurementComplete",
    ):
        require(token in measurement_cpp, f"Interactive measurement implementation missing: {token}")

    require("RecordActiveGeometryMeasurement" not in hud_cpp, "Child HUD must not use the legacy anchor-auto-measure helper")
    require("CapturePointFromView" in hud_cpp, "Measure button must capture real view-selected points")
    require("ResetMeasurementButton" in hud_cpp and "NextMissionButton" in hud_cpp, "HUD must expose reset and next-mission controls")
    require("AwaitingSecondPoint" in hud_cpp and "point A selected" in hud_cpp, "HUD must communicate the two-point measurement state")

    for token in (
        "InteractiveStartMarker",
        "InteractiveEndMarker",
        "InteractiveSegment",
        "ClearInteractiveMeasurement",
        "ConfigureTargetSpanCm",
    ):
        require(token in geometry_h or token in geometry_cpp, f"Mission geometry visualization missing: {token}")
    require("SetCollisionEnabled(ECollisionEnabled::NoCollision)" in geometry_cpp, "Measurement visualization must remain non-colliding")

    for token in (
        "ReloadMissionCatalog",
        "ActivateMission",
        "CycleMission",
        "GetAvailableMissionIds",
        "GetActiveMissionId",
        "MissionCatalog",
        "AvailableMissionIds",
    ):
        require(token in runtime_h, f"Mission catalog API missing: {token}")
    for token in (
        "FindFiles",
        "MissionCatalog.Contains",
        "MissionFiles.Sort",
        "ActivateMission(DefaultPrototypeMissionId)",
        "ConfigureTargetSpanCm",
    ):
        require(token in runtime_cpp, f"Mission catalog implementation missing: {token}")
    require("WorldMakers/Missions/mission.mathematics.measure-and-build-01.json" not in runtime_cpp, "Runtime must not bootstrap by one fixed mission file path")

    require("MissionMeasurementComponent" in player_cpp, "Player must own interactive measurement component")
    for action in ("MeasureMission", "ResetMissionMeasurement", "CycleMission"):
        require(action in input_ini and action in player_cpp, f"Missing prototype input action: {action}")

    canonical_by_id: dict[str, dict] = {}
    for path in sorted((ROOT / "content/missions").rglob("*.json")):
        payload = json.loads(path.read_text(encoding="utf-8"))
        mission_id = payload.get("id")
        if mission_id:
            require(mission_id not in canonical_by_id, f"Duplicate canonical mission id: {mission_id}")
            canonical_by_id[mission_id] = payload

    packaged_paths = sorted((ROOT / "game/Content/WorldMakers/Missions").glob("*.json"))
    require(len(packaged_paths) >= 2, "M2.2 requires at least two packaged mission definitions")
    packaged_ids: set[str] = set()
    for path in packaged_paths:
        payload = json.loads(path.read_text(encoding="utf-8"))
        mission_id = payload.get("id")
        require(bool(mission_id), f"Packaged mission missing id: {path.name}")
        require(mission_id not in packaged_ids, f"Duplicate packaged mission id: {mission_id}")
        packaged_ids.add(mission_id)
        require(mission_id in canonical_by_id, f"Packaged mission has no canonical source: {mission_id}")
        require(payload == canonical_by_id[mission_id], f"Packaged mission must match canonical source exactly: {mission_id}")
        review = payload.get("review", {})
        runtime = payload.get("runtime", {})
        if review.get("pedagogy") != "approved" or review.get("safety") != "approved":
            require(runtime.get("prototypeOnly") is True, f"Draft mission must remain prototypeOnly: {mission_id}")

    require("mission.mathematics.measure-and-build-01" in packaged_ids, "Original M2 mission missing from catalog")
    require("mission.mathematics.measure-and-build-02" in packaged_ids, "Second M2.2 mission missing from catalog")

    for test_name in (
        "WorldMakers.Missions.Measurement.CapturesTwoValidPoints",
        "WorldMakers.Missions.Catalog.MultipleDefinitions",
    ):
        require(test_name in tests_cpp, f"Missing M2.2 automation test: {test_name}")

    print(f"M2.2 interactive measurement and multi-mission contracts validated across {len(packaged_paths)} packaged missions.")


if __name__ == "__main__":
    main()
