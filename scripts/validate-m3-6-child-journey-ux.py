#!/usr/bin/env python3
"""Validate M3.6 child journey UX source contracts."""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

REQUIRED_FILES = (
    "game/Source/WorldMakers/UI/WMChildJourneyTypes.h",
    "game/Source/WorldMakers/UI/WMChildJourneyTypes.cpp",
    "game/Source/WorldMakers/UI/WMChildJourneySubsystem.h",
    "game/Source/WorldMakers/UI/WMChildJourneySubsystem.cpp",
    "game/Source/WorldMakers/UI/WMChildJourneyWidget.h",
    "game/Source/WorldMakers/UI/WMChildJourneyWidget.cpp",
    "game/Source/WorldMakers/Private/Tests/WMChildJourneyTests.cpp",
    "docs/m3-6-child-journey-ux.md",
)


def fail(message: str) -> None:
    raise SystemExit(message)


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require_tokens(path: str, *tokens: str) -> None:
    text = read(path)
    missing = [token for token in tokens if token not in text]
    if missing:
        fail(f"{path}: missing M3.6 contract markers: {missing}")


def main() -> None:
    missing = [path for path in REQUIRED_FILES if not (ROOT / path).is_file()]
    if missing:
        fail(f"Missing M3.6 files: {missing}")

    require_tokens(
        "game/Source/WorldMakers/UI/WMChildJourneyTypes.h",
        "EWMChildAdventureKind",
        "EWMChildAdventureState",
        "Hidden",
        "Ready",
        "InProgress",
        "Complete",
        "FWMChildAdventureCard",
        "FWMChildJourneySnapshot",
        "FWMChildJourneyRules",
    )
    require_tokens(
        "game/Source/WorldMakers/UI/WMChildJourneySubsystem.cpp",
        "GetJourneyReadModel",
        "GetKnownObservationIds",
        "GetInterventionReadModel",
        "IsRewardGranted",
        "GetStateSnapshot",
        "ActivateMission",
        "journey.rainforest-observations",
        "eco.leaf-roof",
        "eco.rainforest-planter",
        "eco.bamboo-bridge",
    )
    require_tokens(
        "game/Source/WorldMakers/UI/WMChildJourneyWidget.cpp",
        "My Adventures",
        "Continue Adventure",
        "You can stop whenever you like.",
        "Card.State == EWMChildAdventureState::Hidden",
        "TouchTargetWidth",
        "TouchTargetHeight",
    )
    require_tokens(
        "game/Source/WorldMakers/UI/WMBuildHUDWidget.cpp",
        "NextMissionButton",
        "My Adventures",
        "UWMChildJourneyWidget",
        "SetPanelOpen",
        "ResolveChildTitle",
        "eco.leaf-roof",
        "eco.rainforest-planter",
        "eco.bamboo-bridge",
        "point A selected",
        "CapturePointFromView",
    )
    require_tokens(
        "game/Source/WorldMakers/Environment/WMBiomeRuntimeSubsystem.h",
        "GetKnownObservationIds",
    )

    child_projection = read("game/Source/WorldMakers/UI/WMChildJourneySubsystem.cpp")
    forbidden_mutations = (
        "EnsureRewardGranted(",
        "ApplyTrustedEffect(",
        "ApplyAction(",
        "RecordObservationEvidence(",
        "RecordMeasurement(",
        "RecordStructureSpan(",
    )
    violations = [token for token in forbidden_mutations if token in child_projection]
    if violations:
        fail(f"Child journey projection must not mutate trusted progress domains: {violations}")

    widget_source = read("game/Source/WorldMakers/UI/WMChildJourneyWidget.cpp")
    hud_source = read("game/Source/WorldMakers/UI/WMBuildHUDWidget.cpp")
    if "CycleMission(" in widget_source:
        fail("My Adventures must not blindly cycle missions")
    if "FText::FromString(Missions->GetActiveMissionId().ToString())" in hud_source:
        fail("Primary child HUD must not render raw mission IDs")

    child_ui_source = (widget_source + "\n" + hud_source).lower()
    pressure_patterns = (
        r"\bxp\b",
        r"\bstreak\b",
        r"\bleaderboard\b",
        r"\bcountdown\b",
        r"daily reward",
        r"limited time",
        r"buy now",
        r"\bshop\b",
    )
    found_pressure = [pattern for pattern in pressure_patterns if re.search(pattern, child_ui_source)]
    if found_pressure:
        fail(f"Child-facing UI contains pressure/commercial language: {found_pressure}")

    widget_header = read("game/Source/WorldMakers/UI/WMChildJourneyWidget.h")
    width = re.search(r"TouchTargetWidth\s*=\s*([0-9.]+)f", widget_header)
    height = re.search(r"TouchTargetHeight\s*=\s*([0-9.]+)f", widget_header)
    if not width or not height or float(width.group(1)) < 112.0 or float(height.group(1)) < 72.0:
        fail("My Adventures footer controls must preserve the established tablet touch-target baseline")

    tests = read("game/Source/WorldMakers/Private/Tests/WMChildJourneyTests.cpp")
    for name in (
        "WorldMakers.UI.ChildJourney.StateMapping",
        "WorldMakers.UI.ChildJourney.ProgressiveReveal",
        "WorldMakers.UI.ChildJourney.FriendlyLabelsHideTechnicalIds",
    ):
        if name not in tests:
            fail(f"Missing M3.6 automation test: {name}")

    repo_quality = read(".github/workflows/repo-quality.yml")
    unreal_ci = read(".github/workflows/unreal-ci.yml")
    gate = "python scripts/validate-m3-6-child-journey-ux.py"
    if gate not in repo_quality or gate not in unreal_ci:
        fail("M3.6 validator must run in Repository Quality and Unreal CI")

    roadmap = read("docs/roadmap.md")
    if "M3.6 — child journey UX: source-complete" not in roadmap:
        fail("Roadmap must preserve M3.6 as source-complete")
    if "M3.7 — tablet performance profiles and capture:" not in roadmap:
        fail("Roadmap must preserve the M3.7 successor phase")

    print("M3.6 Child Journey UX source contract validated: trusted read models composed, calm My Adventures UI present, no progress-domain mutation or pressure loop detected.")


if __name__ == "__main__":
    main()
