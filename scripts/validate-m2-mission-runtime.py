#!/usr/bin/env python3
"""M2 Mission Runtime and mathematics vertical mission source gate."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    raise SystemExit(message)


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def main() -> None:
    required_files = (
        "docs/m2-mission-runtime.md",
        "game/Source/WorldMakers/Mission/WMMissionTypes.h",
        "game/Source/WorldMakers/Mission/WMMissionTypes.cpp",
        "game/Source/WorldMakers/Mission/WMMissionRuntimeSubsystem.h",
        "game/Source/WorldMakers/Mission/WMMissionRuntimeSubsystem.cpp",
        "game/Source/WorldMakers/Private/Tests/WMMissionRuntimeTests.cpp",
        "game/Content/WorldMakers/Missions/mission.mathematics.measure-and-build-01.json",
    )
    missing = [path for path in required_files if not (ROOT / path).exists()]
    if missing:
        fail(f"Missing M2 files: {missing}")

    canonical = json.loads(read("content/missions/math/example-measure-and-build.json"))
    runtime_copy = json.loads(read("game/Content/WorldMakers/Missions/mission.mathematics.measure-and-build-01.json"))
    if canonical != runtime_copy:
        fail("Packaged runtime mission payload must exactly match the canonical mission source")

    if canonical["id"] != "mission.mathematics.measure-and-build-01":
        fail("M2 canonical mission ID changed unexpectedly")
    if canonical["runtime"]["evaluator"] != "measure-and-build":
        fail("M2 mission must use measure-and-build evaluator")
    if canonical["runtime"]["targetSpanCm"] != 300 or canonical["runtime"]["toleranceCm"] != 20:
        fail("M2 target/tolerance baseline changed without updating the phase contract")
    if canonical["review"]["pedagogy"] != "approved" or canonical["review"]["safety"] != "approved":
        if canonical["runtime"].get("prototypeOnly") is not True:
            fail("Draft review state requires prototypeOnly=true")

    reward_catalog = json.loads(read("content/economy/rewards/default-rewards.json"))
    known_rewards = {item["rewardId"] for item in reward_catalog}
    missing_rewards = [rid for rid in canonical["runtime"]["rewardIds"] if rid not in known_rewards]
    if missing_rewards:
        fail(f"M2 references unknown Trust Economy rewards: {missing_rewards}")

    types_h = read("game/Source/WorldMakers/Mission/WMMissionTypes.h")
    for token in (
        "EWMMissionRuntimeState",
        "FWMLearningEvidenceRecord",
        "FName MissionId",
        "FName EventId",
        "FName ObjectiveId",
        "float NumericValue",
        "int32 Sequence",
        "FWMMissionProgressModel",
    ):
        if token not in types_h:
            fail(f"Mission types missing token: {token}")
    evidence_block = types_h.split("struct WORLDMAKERS_API FWMLearningEvidenceRecord", 1)[1].split("USTRUCT", 1)[0]
    if "FString" in evidence_block or "FText" in evidence_block:
        fail("Learning evidence record must not contain free-text fields")

    types_cpp = read("game/Source/WorldMakers/Mission/WMMissionTypes.cpp")
    for token in (
        "measurement_used_before_build",
        "structure_fits_target_span",
        "State = EWMMissionRuntimeState::Completed",
        "EarnedRewardIds = Definition.RewardIds",
        "!bMeasurementEvidence",
    ):
        if token not in types_cpp:
            fail(f"Mission evaluator missing required behavior token: {token}")

    building_h = read("game/Source/WorldMakers/Building/WMBuildingComponent.h")
    building_cpp = read("game/Source/WorldMakers/Building/WMBuildingComponent.cpp")
    for token in ("UseMissionMeasurementTool", "NotifyMissionOfStructureChange"):
        if token not in building_h or token not in building_cpp:
            fail(f"Building/Mission bridge missing: {token}")

    # M2 established a geometry-derived bridge. M2.1 strengthens it from a global
    # X span to a mission-scoped local-axis span; either implementation name is an
    # acceptable M2 contract, while later phases independently require stricter forms.
    span_functions = ("CalculatePlacedStructureSpanX", "CalculateMissionScopedStructureSpan")
    active_span_function = next((name for name in span_functions if name in building_h and name in building_cpp), None)
    if not active_span_function:
        fail("Building/Mission bridge missing a geometry-derived structure-span function")
    if f"RecordStructureSpan({active_span_function}())" not in building_cpp:
        fail("Building Core must emit geometry-derived structure span to Mission Runtime")

    hud = read("game/Source/WorldMakers/UI/WMBuildHUDWidget.cpp")
    for token in ("MeasureTargetButton", "MissionStatusText", "LOCTEXT"):
        if token not in hud:
            fail(f"M2 child UI missing mission/measurement affordance: {token}")
    if "UseMissionMeasurementTool" not in hud and "CapturePointFromView" not in hud:
        fail("M2 child UI must expose either the original measurement bridge or the interactive measurement path")

    tests = read("game/Source/WorldMakers/Private/Tests/WMMissionRuntimeTests.cpp")
    for test_name in (
        "WorldMakers.Missions.MeasureAndBuild.RequiresMeasurementBeforeBuild",
        "WorldMakers.Missions.Definition.ParsesRuntimeContract",
    ):
        if test_name not in tests:
            fail(f"Missing M2 automation test: {test_name}")

    build_cs = read("game/Source/WorldMakers/WorldMakers.Build.cs")
    if '"Json"' not in build_cs:
        fail("M2 JSON mission loader requires Unreal Json module")

    game_ini = read("game/Config/DefaultGame.ini")
    if 'DirectoriesToAlwaysStageAsNonUFS=(Path="WorldMakers/Missions")' not in game_ini:
        fail("Runtime mission JSON must be staged for packaged builds")

    print("M2 Mission Runtime passed: measurement-before-build, geometry evidence, deterministic rewards and packaged mission payload are wired.")


if __name__ == "__main__":
    main()
