#!/usr/bin/env python3
"""Validate M2.1 spatial measurement and mission-geometry source contracts."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

REQUIRED_FILES = [
    "game/Source/WorldMakers/Mission/WMMissionGeometryLibrary.h",
    "game/Source/WorldMakers/Mission/WMMissionGeometryLibrary.cpp",
    "game/Source/WorldMakers/Mission/WMMissionGeometryActor.h",
    "game/Source/WorldMakers/Mission/WMMissionGeometryActor.cpp",
    "game/Source/WorldMakers/Private/Tests/WMMissionGeometryTests.cpp",
    "docs/m2-1-spatial-measurement.md",
]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def main() -> None:
    for relative in REQUIRED_FILES:
        require((ROOT / relative).is_file(), f"Missing M2.1 file: {relative}")

    geometry_h = read("game/Source/WorldMakers/Mission/WMMissionGeometryActor.h")
    geometry_cpp = read("game/Source/WorldMakers/Mission/WMMissionGeometryActor.cpp")
    library_cpp = read("game/Source/WorldMakers/Mission/WMMissionGeometryLibrary.cpp")
    missions_h = read("game/Source/WorldMakers/Mission/WMMissionRuntimeSubsystem.h")
    missions_cpp = read("game/Source/WorldMakers/Mission/WMMissionRuntimeSubsystem.cpp")
    building_cpp = read("game/Source/WorldMakers/Building/WMBuildingComponent.cpp")
    game_mode_cpp = read("game/Source/WorldMakers/Game/WMGameMode.cpp")
    hud_cpp = read("game/Source/WorldMakers/UI/WMBuildHUDWidget.cpp")
    tests_cpp = read("game/Source/WorldMakers/Private/Tests/WMMissionGeometryTests.cpp")
    config = read("game/Config/DefaultGame.ini")

    require("MeasureStart" in geometry_h and "MeasureEnd" in geometry_h, "Mission geometry must expose two measure anchors")
    require("BuildZoneHalfExtent" in geometry_h and "UBoxComponent" in geometry_h, "Mission geometry must define a bounded build zone")
    require("RegisterMissionGeometry(this)" in geometry_cpp, "Mission geometry must register with runtime subsystem")
    require("MeasureWorldDistanceCm" in library_cpp, "World-space distance helper missing")
    require("CalculateScopedSpanAlongZoneX" in library_cpp, "Mission-scoped span helper missing")
    require("InverseTransformPosition" in library_cpp, "Scoped span must operate in mission-local geometry")

    require("RecordActiveGeometryMeasurement" in missions_h and "GetLastMeasuredSpanCm" in missions_h, "Mission runtime must expose spatial measurement state")
    require("ActiveGeometry->GetMeasuredSpanCm()" in missions_cpp, "Measurement must come from world-space anchors")

    forbidden_shortcut = "RecordMeasurement(Missions->GetTargetSpanCm())"
    require(forbidden_shortcut not in building_cpp, "Hard-coded target-span measurement shortcut is forbidden")
    require("RecordActiveGeometryMeasurement" in building_cpp, "Building measurement action must use active geometry")
    require("CalculateMissionScopedStructureSpan" in building_cpp, "Building must calculate mission-scoped structure span")
    require("CalculateScopedSpanAlongZoneX" in building_cpp, "Building must use mission-local span geometry")
    require("GetActiveMissionGeometry" in building_cpp, "Building observations must be bounded by active mission geometry")

    require("EnsurePrototypeMissionGeometry" in game_mode_cpp and "SpawnActor<AWMMissionGeometryActor>" in game_mode_cpp, "GameMode must source-spawn prototype mission geometry")
    require("bSpawnPrototypeMissionGeometry=True" in config, "Prototype mission geometry must be explicitly enabled in config")
    require("GetLastMeasuredSpanCm" in hud_cpp and "Measured:" in hud_cpp, "HUD must display the measured spatial value")

    for test_name in (
        "WorldMakers.Missions.Geometry.WorldDistance",
        "WorldMakers.Missions.Geometry.ZoneContainment",
        "WorldMakers.Missions.Geometry.ScopedStructureSpan",
    ):
        require(test_name in tests_cpp, f"Missing automation test: {test_name}")

    print("M2.1 spatial measurement and mission geometry contracts validated.")


if __name__ == "__main__":
    main()
