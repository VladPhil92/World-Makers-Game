#!/usr/bin/env python3
"""Fail-closed V8 visual/device certification assessor."""

from __future__ import annotations

import argparse
import hashlib
import json
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MATRIX = ROOT / "content/visual/certification/visual-certification-v8.json"
REQUIRED_TABLET_PLATFORMS = {"iPadOS", "Android"}
MIN_FRAME_SAMPLES = 1800


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def fail_reason(result: dict, message: str) -> None:
    result["reasons"].append(message)


def load_matrix() -> dict:
    return json.loads(MATRIX.read_text(encoding="utf-8"))


def validate_evidence_file(path: Path, root: Path, matrix: dict) -> tuple[bool, list[str], str | None]:
    reasons: list[str] = []
    try:
        evidence = json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:
        return False, [f"{path.name}: invalid JSON: {exc}"], None

    platform = evidence.get("platform")
    profile_id = evidence.get("profileId")
    budgets = {item["profileId"]: item for item in matrix["budgets"]}
    budget = budgets.get(profile_id)
    if evidence.get("schemaVersion") != 1:
        reasons.append(f"{path.name}: schemaVersion must be 1")
    if evidence.get("status") != "passed":
        reasons.append(f"{path.name}: status must be passed")
    if platform not in REQUIRED_TABLET_PLATFORMS:
        reasons.append(f"{path.name}: unsupported platform")
    if evidence.get("unrealVersion") != matrix.get("unrealVersion"):
        reasons.append(f"{path.name}: Unreal version mismatch")
    commit = evidence.get("buildCommit", "")
    if not isinstance(commit, str) or len(commit) != 40 or any(c not in "0123456789abcdef" for c in commit):
        reasons.append(f"{path.name}: buildCommit must be a full lowercase SHA")
    if budget is None:
        reasons.append(f"{path.name}: unknown performance profile")
        return False, reasons, platform

    for key in ("captureFile", "screenshotFile"):
        rel = evidence.get(key)
        if not isinstance(rel, str) or not rel or ".." in Path(rel).parts or Path(rel).is_absolute():
            reasons.append(f"{path.name}: invalid {key}")
            continue
        actual = (root / rel).resolve()
        if root.resolve() not in actual.parents and actual != root.resolve():
            reasons.append(f"{path.name}: {key} escapes evidence root")
            continue
        if not actual.is_file():
            reasons.append(f"{path.name}: missing {key} payload")
            continue
        expected_hash = evidence.get(key.replace("File", "Sha256"), "")
        if sha256_file(actual) != expected_hash:
            reasons.append(f"{path.name}: {key} SHA-256 mismatch")

    review = evidence.get("visualReview") or {}
    for key in ("sceneReadable", "uiLegible", "cameraComfortable", "noCriticalArtifacts"):
        if review.get(key) is not True:
            reasons.append(f"{path.name}: visualReview.{key} must be true")

    scenarios = evidence.get("scenarios")
    if not isinstance(scenarios, list):
        reasons.append(f"{path.name}: scenarios missing")
        return False, reasons, platform
    by_id = {item.get("id"): item for item in scenarios if isinstance(item, dict)}
    required = set(matrix["requiredScenarios"])
    if set(by_id) != required or len(scenarios) != len(required):
        reasons.append(f"{path.name}: all four required scenarios must appear exactly once")

    numeric_limits = {
        "p95FrameTimeMs": "maxP95FrameTimeMs",
        "gameThreadP95Ms": "maxGameThreadP95Ms",
        "renderThreadP95Ms": "maxRenderThreadP95Ms",
        "gpuP95Ms": "maxGpuP95Ms",
        "peakDrawCalls": "maxDrawCalls",
        "peakVisibleTriangles": "maxVisibleTriangles",
        "peakResidentTextureMB": "maxResidentTextureMB",
        "peakActiveVfx": "maxActiveVfx",
    }
    positive_metrics = {
        "p95FrameTimeMs",
        "gameThreadP95Ms",
        "renderThreadP95Ms",
        "gpuP95Ms",
        "peakDrawCalls",
        "peakVisibleTriangles",
        "peakResidentTextureMB",
    }
    minimum_samples = int(matrix.get("minimumFrameSamplesPerScenario", MIN_FRAME_SAMPLES))
    for scenario_id in sorted(required):
        scenario = by_id.get(scenario_id)
        if not scenario:
            continue
        if int(scenario.get("frameSamples", 0)) < minimum_samples:
            reasons.append(f"{path.name}: {scenario_id} has fewer than {minimum_samples} frames")
        average = scenario.get("averageFrameTimeMs")
        if not isinstance(average, (int, float)) or float(average) <= 0.0:
            reasons.append(f"{path.name}: {scenario_id}.averageFrameTimeMs must be measured and positive")
        for sample_key, budget_key in numeric_limits.items():
            value = scenario.get(sample_key)
            if not isinstance(value, (int, float)) or value < 0:
                reasons.append(f"{path.name}: {scenario_id}.{sample_key} invalid")
            elif sample_key in positive_metrics and float(value) <= 0.0:
                reasons.append(f"{path.name}: {scenario_id}.{sample_key} must be measured and positive")
            elif float(value) > float(budget[budget_key]):
                reasons.append(f"{path.name}: {scenario_id}.{sample_key} exceeds {budget_key}")
        if scenario_id == "visual.science.vfx-burst" and int(scenario.get("peakActiveVfx", 0)) < 1:
            reasons.append(f"{path.name}: visual.science.vfx-burst must measure at least one active VFX")

    return not reasons, reasons, platform


def assess(evidence_dir: Path) -> dict:
    matrix = load_matrix()
    result = {
        "schemaVersion": 1,
        "status": "BLOCKED",
        "certified": False,
        "evidenceIntegrity": False,
        "requiredPlatforms": sorted(REQUIRED_TABLET_PLATFORMS),
        "platformsPresent": [],
        "evidenceFiles": [],
        "reasons": [],
    }
    if not evidence_dir.is_dir():
        fail_reason(result, "Evidence directory does not exist")
        return result

    files = sorted(evidence_dir.glob("*.json"))
    if not files:
        fail_reason(result, "No V8 device evidence JSON files found")
        return result

    platforms: set[str] = set()
    all_valid = True
    for path in files:
        valid, reasons, platform = validate_evidence_file(path, evidence_dir, matrix)
        result["evidenceFiles"].append({"file": path.name, "valid": valid})
        if platform in REQUIRED_TABLET_PLATFORMS:
            platforms.add(platform)
        if not valid:
            all_valid = False
            result["reasons"].extend(reasons)

    result["platformsPresent"] = sorted(platforms)
    missing_platforms = REQUIRED_TABLET_PLATFORMS - platforms
    if missing_platforms:
        all_valid = False
        fail_reason(result, "Missing representative platform evidence: " + ", ".join(sorted(missing_platforms)))

    result["evidenceIntegrity"] = all_valid
    certified = all_valid and not missing_platforms
    result["certified"] = certified
    result["status"] = "CERTIFIED" if certified else "BLOCKED"
    return result


def write_synthetic_evidence(root: Path, platform: str, profile_id: str) -> None:
    matrix = load_matrix()
    budget = next(item for item in matrix["budgets"] if item["profileId"] == profile_id)
    capture_rel = f"captures/{platform.lower()}.csv"
    screenshot_rel = f"screenshots/{platform.lower()}.png"
    capture = root / capture_rel
    screenshot = root / screenshot_rel
    capture.parent.mkdir(parents=True, exist_ok=True)
    screenshot.parent.mkdir(parents=True, exist_ok=True)
    capture.write_bytes(b"synthetic-v8-self-test-capture")
    screenshot.write_bytes(b"synthetic-v8-self-test-screenshot")
    scenario = {
        "frameSamples": MIN_FRAME_SAMPLES,
        "averageFrameTimeMs": min(10.0, budget["maxP95FrameTimeMs"]),
        "p95FrameTimeMs": budget["maxP95FrameTimeMs"] * 0.8,
        "gameThreadP95Ms": budget["maxGameThreadP95Ms"] * 0.8,
        "renderThreadP95Ms": budget["maxRenderThreadP95Ms"] * 0.8,
        "gpuP95Ms": budget["maxGpuP95Ms"] * 0.8,
        "peakDrawCalls": int(budget["maxDrawCalls"] * 0.8),
        "peakVisibleTriangles": int(budget["maxVisibleTriangles"] * 0.8),
        "peakResidentTextureMB": int(budget["maxResidentTextureMB"] * 0.8),
        "peakActiveVfx": max(1, int(budget["maxActiveVfx"] * 0.8)),
    }
    payload = {
        "schemaVersion": 1,
        "status": "passed",
        "platform": platform,
        "deviceClass": f"synthetic-{platform}",
        "osVersion": "self-test",
        "unrealVersion": "5.8.2",
        "buildCommit": "1" * 40,
        "profileId": profile_id,
        "captureTool": "CSVProfiler",
        "captureFile": capture_rel,
        "captureSha256": sha256_file(capture),
        "screenshotFile": screenshot_rel,
        "screenshotSha256": sha256_file(screenshot),
        "visualReview": {
            "sceneReadable": True,
            "uiLegible": True,
            "cameraComfortable": True,
            "noCriticalArtifacts": True,
            "reviewerRole": "visual-qa",
        },
        "scenarios": [{"id": sid, **scenario} for sid in matrix["requiredScenarios"]],
    }
    (root / f"{platform.lower()}.json").write_text(json.dumps(payload, indent=2), encoding="utf-8")


def self_test() -> int:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        blocked = assess(root)
        if blocked["status"] != "BLOCKED":
            raise SystemExit("Empty evidence directory must fail closed")
        write_synthetic_evidence(root, "Android", "performance.tablet.low")
        one_platform = assess(root)
        if one_platform["status"] != "BLOCKED":
            raise SystemExit("One platform must not certify")
        write_synthetic_evidence(root, "iPadOS", "performance.tablet.high")
        certified = assess(root)
        if certified["status"] != "CERTIFIED":
            raise SystemExit("Synthetic two-platform compliant evidence should certify in self-test")
        android = json.loads((root / "android.json").read_text(encoding="utf-8"))
        android["scenarios"][0]["peakDrawCalls"] = 999999
        (root / "android.json").write_text(json.dumps(android), encoding="utf-8")
        over_budget = assess(root)
        if over_budget["status"] != "BLOCKED":
            raise SystemExit("Any over-budget metric must block certification")
    print("V8 assessor self-test passed: fail-closed, dual-platform, integrity and budget rules verified.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-dir", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--require-certified", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    if args.self_test:
        return self_test()
    if not args.evidence_dir:
        parser.error("--evidence-dir is required unless --self-test is used")

    result = assess(args.evidence_dir)
    rendered = json.dumps(result, indent=2, sort_keys=True)
    print(rendered)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered + "\n", encoding="utf-8")
    if args.require_certified and result["status"] != "CERTIFIED":
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
