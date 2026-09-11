#!/usr/bin/env python3
"""Validate V8 visual optimization and device-certification source contracts."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CANONICAL = ROOT / "content/visual/certification/visual-certification-v8.json"
PACKAGED = ROOT / "game/Content/WorldMakers/Visual/Certification/visual-certification-v8.json"
SCHEMA = ROOT / "content/visual/certification/v8-visual-device-evidence.schema.json"
EXAMPLE = ROOT / "docs/templates/v8-visual-device-evidence.example.json"

REQUIRED_FILES = (
    "game/Source/WorldMakers/Performance/WMVisualCertificationTypes.h",
    "game/Source/WorldMakers/Performance/WMVisualCertificationTypes.cpp",
    "game/Source/WorldMakers/Private/Tests/WMVisualCertificationTests.cpp",
    "scripts/assess-v8-visual-certification.py",
    "docs/v8-visual-optimization-device-certification.md",
    ".github/workflows/v8-visual-certification.yml",
)

REQUIRED_SCENARIOS = {
    "visual.rainforest.explore",
    "visual.build.dense",
    "visual.science.vfx-burst",
    "visual.adventure.reveal",
}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def main() -> None:
    missing = [path for path in REQUIRED_FILES if not (ROOT / path).is_file()]
    require(not missing, f"Missing V8 files: {missing}")
    require(CANONICAL.is_file() and PACKAGED.is_file() and SCHEMA.is_file() and EXAMPLE.is_file(), "V8 matrix/schema/template is incomplete")
    require(CANONICAL.read_bytes() == PACKAGED.read_bytes(), "Packaged V8 certification matrix must be byte-equivalent to canonical JSON")

    matrix = json.loads(CANONICAL.read_text(encoding="utf-8"))
    require(matrix.get("schemaVersion") == 1, "V8 matrix schemaVersion must be 1")
    require(matrix.get("unrealVersion") == "5.8.2", "V8 must lock Unreal Engine 5.8.2")
    require(matrix.get("minimumFrameSamplesPerScenario") == 1800, "V8 requires 1,800 samples per scenario")
    require(set(matrix.get("requiredPlatforms", [])) == {"iPadOS", "Android"}, "V8 requires iPadOS and Android evidence")
    require(set(matrix.get("requiredScenarios", [])) == REQUIRED_SCENARIOS, "V8 four-scenario matrix drifted")

    budgets = matrix.get("budgets", [])
    require(len(budgets) == 3, "V8 requires exactly Low/Mid/High tablet budgets")
    by_id = {item.get("profileId"): item for item in budgets}
    required_profiles = {"performance.tablet.low", "performance.tablet.medium", "performance.tablet.high"}
    require(set(by_id) == required_profiles, "V8 tablet profile set drifted")
    require(by_id["performance.tablet.low"]["targetFps"] == 30, "Low must remain 30 FPS")
    require(by_id["performance.tablet.medium"]["targetFps"] == 30, "Medium must remain 30 FPS")
    require(by_id["performance.tablet.high"]["targetFps"] == 60, "High must remain 60 FPS")
    for item in budgets:
        fps = item["targetFps"]
        require(abs((1000.0 / fps) - float(item["maxP95FrameTimeMs"])) <= 0.02, f"Frame budget mismatch: {item['profileId']}")
        for key in ("maxGameThreadP95Ms", "maxRenderThreadP95Ms", "maxGpuP95Ms"):
            require(0 < float(item[key]) <= float(item["maxP95FrameTimeMs"]), f"Invalid thread/GPU ceiling {key}: {item['profileId']}")
        for key in ("maxDrawCalls", "maxVisibleTriangles", "maxResidentTextureMB", "maxActiveVfx"):
            require(isinstance(item[key], int) and item[key] > 0, f"Invalid visual resource ceiling {key}: {item['profileId']}")

    rules = matrix.get("certificationRules", {})
    for key in ("allRequiredScenariosPerDevice", "allTabletProfilesRequired", "singleBuildCommitRequired", "allMetricsWithinBudget", "captureHashRequired", "screenshotHashRequired", "visualReviewRequired", "certificationIsFailClosed"):
        require(rules.get(key) is True, f"V8 release rule must be enabled: {key}")
    require(rules.get("sourceCiCanSelfCertify") is False, "Source CI must never self-certify V8")

    schema = json.loads(SCHEMA.read_text(encoding="utf-8"))
    require(schema.get("type") == "object" and schema.get("additionalProperties") is False, "V8 evidence schema must be closed")
    props = schema.get("properties", {})
    require(props.get("platform", {}).get("enum") == ["iPadOS", "Android"], "V8 schema platform contract drifted")
    require(props.get("unrealVersion", {}).get("const") == "5.8.2", "V8 schema must lock UE 5.8.2")
    forbidden = {"serialNumber", "deviceId", "advertisingId", "accountId", "childProfileId", "email", "biometric"}
    require(not (forbidden & set(props)), "V8 evidence must not collect persistent hardware/user identifiers")
    scenario_items = props["scenarios"]["items"]["properties"]
    require(set(scenario_items["id"]["enum"]) == REQUIRED_SCENARIOS, "V8 evidence scenario enum drifted")

    example = json.loads(EXAMPLE.read_text(encoding="utf-8"))
    require(example.get("status") == "pending", "V8 example must never ship pre-certified")
    review = example.get("visualReview", {})
    require(all(review.get(key) is False for key in ("sceneReadable", "uiLegible", "cameraComfortable", "noCriticalArtifacts")), "V8 example review must default to false")
    require(all(item.get("frameSamples") == 0 for item in example.get("scenarios", [])), "V8 example must contain no fabricated frame evidence")

    types_h = read("game/Source/WorldMakers/Performance/WMVisualCertificationTypes.h")
    types_cpp = read("game/Source/WorldMakers/Performance/WMVisualCertificationTypes.cpp")
    for token in ("FWMVisualCertificationBudget", "FWMVisualCertificationSample", "FWMVisualCertificationVerdict", "FWMVisualCertificationEvaluator", "MinimumFrameSamples = 1800"):
        require(token in types_h, f"V8 C++ contract missing: {token}")
    for token in ("bEnoughFrameSamples", "bFrameTimeWithinBudget", "bThreadsWithinBudget", "bGpuWithinBudget", "bDrawCallsWithinBudget", "bTrianglesWithinBudget", "bTextureMemoryWithinBudget", "bVfxWithinBudget", "bWithinBudget"):
        require(token in types_cpp, f"V8 evaluator missing fail-closed metric: {token}")

    tests = read("game/Source/WorldMakers/Private/Tests/WMVisualCertificationTests.cpp")
    for name in (
        "WorldMakers.Visual.Certification.PassingSampleRequiresMeasuredBudgetCompliance",
        "WorldMakers.Visual.Certification.FrameSampleFloorFailsClosed",
        "WorldMakers.Visual.Certification.AnyVisualBudgetFailureBlocksVerdict",
    ):
        require(name in tests, f"Missing V8 automation test: {name}")

    assessor = read("scripts/assess-v8-visual-certification.py")
    for token in (
        '"CERTIFIED" if certified else "BLOCKED"',
        "--require-certified",
        "--self-test",
        'REQUIRED_TABLET_PLATFORMS = {"iPadOS", "Android"}',
        "REQUIRED_PLATFORM_PROFILE_PAIRS",
        "platformProfilePairsPresent",
        "Counter(pairs)",
        "Five of six platform/profile pairs must not certify",
        "MIN_FRAME_SAMPLES = 1800",
        "sha256_file",
        "captureSha256",
        "screenshotSha256",
        "visualReview",
        "requiredScenarios",
    ):
        require(token in assessor, f"V8 assessor missing fail-closed token: {token}")
    self_test = subprocess.run([sys.executable, str(ROOT / "scripts/assess-v8-visual-certification.py"), "--self-test"], cwd=ROOT, capture_output=True, text=True, check=False)
    require(self_test.returncode == 0, "V8 assessor self-test failed:\n" + self_test.stdout + self_test.stderr)

    cert_workflow = read(".github/workflows/v8-visual-certification.yml")
    for token in (
        "workflow_dispatch:",
        "self-hosted",
        "Windows",
        "X64",
        "unreal",
        "UNREAL_SELF_HOSTED_ENABLED",
        "UNREAL_ENGINE_ROOT",
        "V8_DEVICE_EVIDENCE_ROOT",
        "Automation RunTests WorldMakers.",
        "--require-certified",
        "v8-visual-device-certification-${{ github.sha }}",
    ):
        require(token in cert_workflow, f"V8 certification workflow missing: {token}")
    require("pull_request:" not in cert_workflow and "push:" not in cert_workflow, "V8 certification workflow must remain manual")

    default_game = read("game/Config/DefaultGame.ini")
    require('+DirectoriesToAlwaysStageAsNonUFS=(Path="WorldMakers/Visual")' in default_game, "V8 packaged matrix must remain staged via WorldMakers/Visual")

    docs = read("docs/v8-visual-optimization-device-certification.md").lower()
    for token in ("fail-closed", "1,800", "ipados", "android", "p95", "draw calls", "sha-256", "camera comfort", "not `certified`", "source-complete"):
        require(token in docs, f"V8 documentation boundary missing: {token}")

    repo_quality = read(".github/workflows/repo-quality.yml")
    require("python scripts/validate-v8-visual-certification.py" in repo_quality, "Repository Quality must run V8 source gate")

    roadmap = read("docs/visual-production-roadmap.md")
    require("V8 status: source-complete certification infrastructure." in roadmap, "Visual roadmap must mark V8 source-complete infrastructure")
    require("actual device certification remains blocked until representative evidence passes" in roadmap.lower(), "Visual roadmap must preserve external certification boundary")

    print("V8 visual optimization/device-certification source contract validated: budgets, four stress scenarios, exact six Android/iPadOS profile pairs, integrity hashes, native manual release gate and source tests are present.")


if __name__ == "__main__":
    main()
