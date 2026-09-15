#!/usr/bin/env python3
"""Validate G4 device performance/stability source infrastructure without claiming device certification."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "content/production/g4-device-performance-stability-v1.json"
TEMPLATE = ROOT / "content/production/g4-stability-evidence-template.json"
G3 = ROOT / "content/production/g3-visual-fidelity-v1.json"
V8 = ROOT / "content/visual/certification/visual-certification-v8.json"
PROFILES = ROOT / "content/performance/tablet-performance-profiles.json"
ASSESSOR = ROOT / "scripts/assess-g4-device-performance.py"
V8_ASSESSOR = ROOT / "scripts/assess-v8-visual-certification.py"
RUNNER = ROOT / "scripts/run-g4-device-certification.ps1"
LAUNCHER = ROOT / "WorldMakers-G4-Certify.cmd"
DOC = ROOT / "docs/g4-device-performance-stability.md"
WORKFLOW = ROOT / ".github/workflows/g4-device-performance.yml"


def load(path: Path) -> dict:
    if not path.is_file():
        raise SystemExit(f"ERROR: missing required file: {path.relative_to(ROOT)}")
    try:
        return json.loads(path.read_text(encoding="utf-8-sig"))
    except json.JSONDecodeError as exc:
        raise SystemExit(f"ERROR: invalid JSON in {path.relative_to(ROOT)}: {exc}")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit("ERROR: " + message)


def require_tokens(path: Path, tokens: tuple[str, ...]) -> None:
    require(path.is_file(), f"missing required file: {path.relative_to(ROOT)}")
    text = path.read_text(encoding="utf-8-sig")
    for token in tokens:
        require(token in text, f"{path.relative_to(ROOT)} missing required token: {token}")


def main() -> int:
    contract = load(CONTRACT)
    template = load(TEMPLATE)
    g3 = load(G3)
    v8 = load(V8)
    profiles = load(PROFILES)

    require(contract.get("schema") == "worldmakers.g4-device-performance-stability.v1", "G4 schema drift")
    require(contract.get("status") == "source-ready-device-evidence-required", "G4 status drift")
    require(contract.get("engine", {}).get("expectedVersion") == "5.8.2", "G4 must remain locked to UE 5.8.2")
    require(g3.get("schema") == "worldmakers.g3-visual-fidelity.v1", "G4 dependency requires G3 contract v1")

    platforms = set(contract.get("requiredPlatforms", []))
    required_profiles = set(contract.get("requiredProfiles", []))
    scenarios = set(contract.get("requiredScenarios", []))
    require(platforms == {"Android", "iPadOS"}, "G4 platform set drift")
    require(required_profiles == {"performance.tablet.low", "performance.tablet.medium", "performance.tablet.high"}, "G4 tablet profile set drift")
    require(scenarios == set(v8.get("requiredScenarios", [])), "G4 scenarios must match V8 exactly")
    require(platforms == set(v8.get("requiredPlatforms", [])), "G4 platforms must match V8 exactly")
    require(contract["performanceEvidence"]["minimumFrameSamplesPerScenario"] == v8["minimumFrameSamplesPerScenario"], "G4 frame sample floor must match V8")

    profile_rows = {row.get("id"): row for row in profiles.get("profiles", [])}
    require(required_profiles <= set(profile_rows), "G4 references unknown tablet performance profiles")
    v8_budgets = {row.get("profileId"): row for row in v8.get("budgets", [])}
    require(required_profiles == set(v8_budgets), "V8 budget set must cover G4 profiles exactly")
    for profile_id in required_profiles:
        require(profile_rows[profile_id].get("targetFps") == v8_budgets[profile_id].get("targetFps"), f"target FPS drift for {profile_id}")
        require(abs(float(profile_rows[profile_id].get("frameTimeBudgetMs")) - float(v8_budgets[profile_id].get("maxP95FrameTimeMs"))) < 0.01, f"frame-time budget drift for {profile_id}")

    stability = contract.get("stabilityEvidence", {})
    require(int(stability.get("minimumSoakDurationSeconds", 0)) >= 900, "G4 soak duration may not be below 15 minutes")
    require(int(stability.get("minimumForegroundBackgroundCycles", 0)) >= 3, "G4 requires at least three foreground/background cycles")
    require(float(stability.get("maxFrameTimeDriftPercent", 999)) <= 15.0, "G4 frame-time drift ceiling weakened")
    require(float(stability.get("maxSustainedPerformanceDropPercent", 999)) <= 15.0, "G4 sustained performance drop ceiling weakened")
    require(float(stability.get("maxProcessMemoryFractionOfPhysical", 2)) <= 0.8, "G4 process memory fraction ceiling weakened")
    require(stability.get("thermalThrottlingAllowed") is False, "G4 may not allow thermal throttling")
    require(stability.get("criticalThermalStateAllowed") is False, "G4 may not allow critical thermal state")
    require(stability.get("physicalDeviceRequired") is True, "G4 requires physical devices")
    require(stability.get("suspendResumeRequired") is True, "G4 requires suspend/resume validation")
    require(stability.get("inputRecoveryRequired") is True, "G4 requires input recovery validation")
    memory_limits = stability.get("maxResidentMemoryGrowthMBByProfile", {})
    require(set(memory_limits) == required_profiles, "G4 memory-growth profile budget set drift")
    require(all(isinstance(v, (int, float)) and 0 < float(v) <= 192 for v in memory_limits.values()), "G4 memory growth budgets invalid or weakened")

    cert = contract.get("certification", {})
    for key in ("requiresG3CertifiedSameCommit", "requiresV8CertifiedSameCommit", "requiresExactSixStabilityPackages", "requiresCleanCurrentMain"):
        require(cert.get(key) is True, f"G4 certification must require {key}")
    require(cert.get("resultStates") == ["CERTIFIED", "BLOCKED", "NON_CERTIFYING_PASS"], "G4 result state contract drift")

    require(template.get("status") == "pending", "G4 evidence template must remain pending")
    require(template.get("device", {}).get("isPhysicalDevice") is False, "G4 evidence template may not pre-approve physical-device evidence")
    require(template.get("reliability", {}).get("suspendResumePassed") is False, "G4 template may not pre-pass suspend/resume")
    require(template.get("reliability", {}).get("inputRecoveryPassed") is False, "G4 template may not pre-pass input recovery")

    require_tokens(V8_ASSESSOR, ("REQUIRED_PLATFORM_PROFILE_PAIRS", "captureSha256", "screenshotSha256", "CERTIFIED"))
    require_tokens(ASSESSOR, ("thermalThrottlingObserved", "residentMemoryGrowthMB", "foregroundBackgroundCycles", "load_v8_module", "G3 must be CERTIFIED", "--self-test"))
    require_tokens(RUNNER, ("assess-g4-device-performance.py", "NON_CERTIFYING_PASS", "origin/main", "g4-device-performance.json"))
    require_tokens(LAUNCHER, ("run-g4-device-certification.ps1",))
    require_tokens(DOC, ("G4", "Android", "iPadOS", "15 minutes", "WorldMakers-G4-Certify.cmd", "Production Alpha"))
    require_tokens(WORKFLOW, ("G4 Device Performance", "validate-g4-device-performance.py", "--self-test"))

    print("World Makers G4 Device Performance & Stability source contract: PASS")
    print("  Source infrastructure only; no physical-device certification is claimed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
