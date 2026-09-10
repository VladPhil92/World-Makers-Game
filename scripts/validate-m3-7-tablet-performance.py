#!/usr/bin/env python3
"""Validate M3.7 tablet performance profiles and local capture contracts."""

from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CANONICAL = ROOT / "content/performance/tablet-performance-profiles.json"
PACKAGED = ROOT / "game/Content/WorldMakers/Performance/tablet-performance-profiles.json"
SCHEMA = ROOT / "content/performance/tablet-performance-profile.schema.json"

REQUIRED_FILES = (
    "game/Source/WorldMakers/Performance/WMPerformanceProfileTypes.h",
    "game/Source/WorldMakers/Performance/WMPerformanceProfileTypes.cpp",
    "game/Source/WorldMakers/Performance/WMPerformanceProfileSubsystem.h",
    "game/Source/WorldMakers/Performance/WMPerformanceProfileSubsystem.cpp",
    "game/Source/WorldMakers/Performance/WMPerformanceCaptureSubsystem.h",
    "game/Source/WorldMakers/Performance/WMPerformanceCaptureSubsystem.cpp",
    "game/Source/WorldMakers/Environment/WMCaribbeanRainforestPrototype.h",
    "game/Source/WorldMakers/Environment/WMCaribbeanRainforestPrototype.cpp",
    "game/Source/WorldMakers/Private/Tests/WMPerformanceProfileTests.cpp",
    "game/Config/DefaultGame.ini",
    "docs/m3-7-tablet-performance.md",
)

ALLOWED_CVARS = {
    "sg.ViewDistanceQuality",
    "sg.AntiAliasingQuality",
    "sg.ShadowQuality",
    "sg.PostProcessQuality",
    "sg.TextureQuality",
    "sg.EffectsQuality",
    "sg.FoliageQuality",
    "r.ScreenPercentage",
    "r.Streaming.PoolSize",
    "foliage.DensityScale",
    "grass.DensityScale",
    "r.Shadow.DistanceScale",
}


def fail(message: str) -> None:
    raise SystemExit(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def main() -> None:
    missing = [path for path in REQUIRED_FILES if not (ROOT / path).is_file()]
    require(not missing, f"Missing M3.7 files: {missing}")
    require(CANONICAL.is_file() and PACKAGED.is_file() and SCHEMA.is_file(), "M3.7 profile source/schema/package is incomplete")
    require(CANONICAL.read_bytes() == PACKAGED.read_bytes(), "Packaged performance profile catalog must be byte-equivalent to canonical JSON")

    payload = json.loads(CANONICAL.read_text(encoding="utf-8"))
    schema = json.loads(SCHEMA.read_text(encoding="utf-8"))
    require(payload.get("schemaVersion") == 1, "Performance profile schemaVersion must be 1")
    require(schema.get("type") == "object" and "$defs" in schema and "profile" in schema["$defs"], "Performance profile schema contract is incomplete")

    profiles = payload.get("profiles", [])
    require(isinstance(profiles, list) and len(profiles) >= 4, "M3.7 requires three tablet tiers plus desktop reference")
    by_id = {profile.get("id"): profile for profile in profiles}
    require(len(by_id) == len(profiles), "Performance profile IDs must be unique")
    required_ids = {
        "performance.tablet.low",
        "performance.tablet.medium",
        "performance.tablet.high",
        "performance.desktop.reference",
    }
    require(required_ids.issubset(by_id), f"Missing required performance profiles: {sorted(required_ids - set(by_id))}")

    for profile in profiles:
        fps = profile.get("targetFps")
        frame_ms = profile.get("frameTimeBudgetMs")
        require(isinstance(fps, int) and 30 <= fps <= 120, f"Invalid target FPS: {profile.get('id')}")
        require(isinstance(frame_ms, (int, float)) and 8 <= frame_ms <= 40, f"Invalid frame-time budget: {profile.get('id')}")
        require(abs((1000.0 / fps) - float(frame_ms)) <= 0.02, f"Frame-time budget must match target FPS: {profile.get('id')}")
        require(50 <= profile.get("screenPercentage", 0) <= 100, f"Invalid screen percentage: {profile.get('id')}")
        require(128 <= profile.get("texturePoolMB", 0) <= 4096, f"Invalid texture pool: {profile.get('id')}")
        for key in ("maxWorldActors", "maxPlacedBuildPieces", "maxActiveInteractables"):
            require(isinstance(profile.get(key), int) and profile[key] > 0, f"Invalid structural budget {key}: {profile.get('id')}")
        for key in ("foliageDensityScale", "grassDensityScale", "shadowDistanceScale"):
            require(0.1 <= float(profile.get(key, 0)) <= 1.0, f"Invalid scalability ratio {key}: {profile.get('id')}")

    low = by_id["performance.tablet.low"]
    medium = by_id["performance.tablet.medium"]
    high = by_id["performance.tablet.high"]
    require(low["targetFps"] == 30 and medium["targetFps"] == 30 and high["targetFps"] == 60, "Tablet target FPS contract changed unexpectedly")
    monotonic_keys = (
        "screenPercentage",
        "texturePoolMB",
        "maxWorldActors",
        "maxPlacedBuildPieces",
        "maxActiveInteractables",
        "foliageDensityScale",
        "grassDensityScale",
        "shadowDistanceScale",
    )
    for key in monotonic_keys:
        require(float(low[key]) <= float(medium[key]) <= float(high[key]), f"Tablet tiers must be monotonic for {key}")
    require(low["screenPercentage"] < medium["screenPercentage"] < high["screenPercentage"], "Tablet render scale must increase by tier")

    raw_profile_text = CANONICAL.read_text(encoding="utf-8").lower()
    for forbidden in ("cvar", "consolecommand", "commandline", "exec("):
        require(forbidden not in raw_profile_text, f"Performance JSON must contain bounded values only, not command surfaces: {forbidden}")

    types_h = read("game/Source/WorldMakers/Performance/WMPerformanceProfileTypes.h")
    types_cpp = read("game/Source/WorldMakers/Performance/WMPerformanceProfileTypes.cpp")
    for token in (
        "FWMPerformanceBudget",
        "FWMScalabilityProfile",
        "FWMPerformanceProfileDefinition",
        "FWMPerformanceProfileCatalog",
        "FWMPerformanceCaptureSummary",
        "FWMPerformanceCaptureAccumulator",
        "BuildAllowlistedCVarAssignments",
        "P95FrameTimeMs",
        "bWithinBudget",
    ):
        require(token in types_h or token in types_cpp, f"Performance model missing contract token: {token}")

    discovered_cvars = set(re.findall(r'Result\.Add\(TEXT\("([^"]+)"\)', types_cpp))
    require(discovered_cvars == ALLOWED_CVARS, f"Performance CVar allowlist drifted: {sorted(discovered_cvars ^ ALLOWED_CVARS)}")

    profile_cpp = read("game/Source/WorldMakers/Performance/WMPerformanceProfileSubsystem.cpp")
    for token in (
        "WorldMakers/Performance/tablet-performance-profiles.json",
        "FindConsoleVariable",
        "ECVF_SetByGameSetting",
        "performance.tablet.medium",
        "performance.desktop.reference",
        "PLATFORM_ANDROID || PLATFORM_IOS",
        "UWMVisualProfileSettings",
        "EWMVisualQualityTier::Low",
        "EWMVisualQualityTier::Mid",
        "EWMVisualQualityTier::High",
        "RefreshFromVisualProfile",
    ):
        require(token in profile_cpp, f"Profile runtime missing contract token: {token}")
    require("Exec(" not in profile_cpp and "ConsoleCommand(" not in profile_cpp, "Scalability profiles must not execute arbitrary console commands")

    rainforest_h = read("game/Source/WorldMakers/Environment/WMCaribbeanRainforestPrototype.h")
    rainforest_cpp = read("game/Source/WorldMakers/Environment/WMCaribbeanRainforestPrototype.cpp")
    require("RefreshFromVisualProfile" in rainforest_h and "RefreshFromVisualProfile" in rainforest_cpp, "Rainforest prototype must support hot visual-budget refresh")
    require("bUseProfileDefaultQuality" in rainforest_cpp and "Profile->DefaultQualityTier" in rainforest_cpp, "Rainforest density must remain driven by the selected visual profile")

    default_game = read("game/Config/DefaultGame.ini")
    require('+DirectoriesToAlwaysStageAsNonUFS=(Path="WorldMakers/Performance")' in default_game, "Performance profile JSON must be staged into packaged builds")
    for expected in (
        "LowBudget=(TargetFPS=30",
        "MidBudget=(TargetFPS=30",
        "HighBudget=(TargetFPS=60",
    ):
        require(expected in default_game, f"Existing rainforest visual budget must remain aligned with performance tiers: {expected}")

    capture_h = read("game/Source/WorldMakers/Performance/WMPerformanceCaptureSubsystem.h")
    capture_cpp = read("game/Source/WorldMakers/Performance/WMPerformanceCaptureSubsystem.cpp")
    for token in (
        "UTickableWorldSubsystem",
        "BeginCapture",
        "EndCapture",
        "GetLiveSummary",
        "StructuralSampleIntervalSeconds = 0.50f",
    ):
        require(token in capture_h, f"Capture subsystem missing API contract: {token}")
    for token in (
        "DeltaTime * 1000.0f",
        "TActorIterator<AActor>",
        "GetPlacedPieces().Num()",
        "GetInteractionTargetCount()",
        "GetActionTargetCount()",
        "ProjectSavedDir",
        "WorldMakers/Performance",
    ):
        require(token in capture_cpp, f"Capture implementation missing source contract: {token}")

    performance_sources = "\n".join(
        path.read_text(encoding="utf-8")
        for path in (ROOT / "game/Source/WorldMakers/Performance").glob("*.*")
        if path.suffix in {".h", ".cpp"}
    )
    require("Mission/" not in performance_sources, "Performance domain must remain mission/progression agnostic")
    for forbidden in ("ChildProfileId", "AccountId", "Email", "Purchase", "Commerce", "AdvertisingId"):
        require(forbidden not in performance_sources, f"Performance capture must not collect personal/commercial state: {forbidden}")

    tests = read("game/Source/WorldMakers/Private/Tests/WMPerformanceProfileTests.cpp")
    for name in (
        "WorldMakers.Performance.Profiles.CatalogAndAllowlist",
        "WorldMakers.Performance.Capture.BudgetEvaluation",
    ):
        require(name in tests, f"Missing M3.7 automation test: {name}")

    collector = read("scripts/collect-unreal-certification-evidence.ps1")
    require("WorldMakers\\Performance" in collector and "performanceCapture" in collector, "Certification evidence collector must retain optional M3.7 capture reports")

    repo_quality = read(".github/workflows/repo-quality.yml")
    unreal_ci = read(".github/workflows/unreal-ci.yml")
    gate = "python scripts/validate-m3-7-tablet-performance.py"
    require(gate in repo_quality and gate in unreal_ci, "M3.7 validator must run in Repository Quality and Unreal CI")

    roadmap = read("docs/roadmap.md")
    require("M3.7 — tablet performance profiles and capture: source-complete" in roadmap, "Roadmap must mark M3.7 source-complete")
    require("M3.8 — vertical-slice certification: next" in roadmap, "Roadmap must mark M3.8 next")

    tdd = read("docs/TDD.md")
    for token in ("Tablet Low", "Tablet Medium", "Tablet High", "p95 frame time", "33.34 ms", "16.67 ms"):
        require(token in tdd, f"TDD performance budget missing: {token}")

    docs = read("docs/m3-7-tablet-performance.md").lower()
    for boundary in ("issue #9", "p95", "pii", "not claims", "representative tablets", "m3.8"):
        require(boundary in docs, f"M3.7 documentation boundary missing: {boundary}")

    print("M3.7 tablet performance source contract validated: tiered budgets, visual-density synchronization, packaged profile data, fixed scalability allowlist, privacy-safe local capture, evidence wiring and CI gates are present.")


if __name__ == "__main__":
    main()
