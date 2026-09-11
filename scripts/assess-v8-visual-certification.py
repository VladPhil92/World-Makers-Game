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
REQUIRED_PROFILES = {
    "performance.tablet.low",
    "performance.tablet.medium",
    "performance.tablet.high",
}
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


def validate_evidence_file(
    path: Path,
    root: Path,
    matrix: dict,
    expected_commit: str | None = None,
) -> tuple[bool, list[str], str | None, str | None, str | None]:
    reasons: list[str] = []
    try:
        evidence = json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:
        return False, [f"{path.name}: invalid JSON: {exc}"], None, None, None

    platform = evidence.get("platform")
    profile_id = evidence.get("profileId")
    commit = evidence.get("buildCommit")
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
    if not isinstance(commit, str) or len(commit) != 40 or any(c not in "0123456789abcdef" for c in commit):
        reasons.append(f"{path.name}: buildCommit must be a full lowercase SHA")
    elif expected_commit and commit != expected_commit:
        reasons.append(f"{path.name}: buildCommit does not match the certification commit")
    if profile_id not in REQUIRED_PROFILES or budget is None:
        reasons.append(f"{path.name}: unknown performance profile")
        return False, reasons, platform, profile_id, commit

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
        return False, reasons, platform, profile_id, commit
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

    return not reasons, reasons, platform, profile_id, commit


def assess(evidence_dir: Path, expected_commit: str | None = None) -> dict:
    matrix = load_matrix()
    result = {
        "schemaVersion": 1,
        "status": "BLOCKED",
        "certified": False,
        "evidenceIntegrity": False,
        "requiredPlatforms": sorted(REQUIRED_TABLET_PLATFORMS),
        "requiredProfiles": sorted(REQUIRED_PROFILES),
        "platformsPresent": [],
        "profilesPresent": [],
        "buildCommitsPresent": [],
        "evidenceFiles": [],
        "reasons": [],
    }
    if expected_commit is not None:
        if len(expected_commit) != 40 or any(c not in "0123456789abcdef" for c in expected_commit):
            fail_reason(result, "Expected commit must be a full lowercase SHA")
            return result

    if not evidence_dir.is_dir():
        fail_reason(result, "Evidence directory does not exist")
        return result

    files = sorted(evidence_dir.glob("*.json"))
    if not files:
        fail_reason(result, "No V8 device evidence JSON files found")
        return result

    platforms: set[str] = set()
    profiles: set[str] = set()
    commits: set[str] = set()
    all_valid = True
    for path in files:
        valid, reasons, platform, profile_id, commit = validate_evidence_file(path, evidence_dir, matrix, expected_commit)
        result["evidenceFiles"].append({"file": path.name, "valid": valid})
        if platform in REQUIRED_TABLET_PLATFORMS:
            platforms.add(platform)
        if profile_id in REQUIRED_PROFILES:
            profiles.add(profile_id)
        if isinstance(commit, str) and len(commit) == 40:
            commits.add(commit)
        if not valid:
            all_valid = False
            result["reasons"].extend(reasons)

    result["platformsPresent"] = sorted(platforms)
    result["profilesPresent"] = sorted(profiles)
    result["buildCommitsPresent"] = sorted(commits)

    missing_platforms = REQUIRED_TABLET_PLATFORMS - platforms
    if missing_platforms:
        all_valid = False
        fail_reason(result, "Missing representative platform evidence: " + ", ".join(sorted(missing_platforms)))

    missing_profiles = REQUIRED_PROFILES - profiles
    if missing_profiles:
        all_valid = False
        fail_reason(result, "Missing tablet profile evidence: " + ", ".join(sorted(missing_profiles)))

    if len(commits) != 1:
        all_valid = False
        fail_reason(result, "All V8 evidence packages must reference one identical build commit")

    result["evidenceIntegrity"] = all_valid
    certified = all_valid and not missing_platforms and not missing_profiles and len(commits) == 1
    result["certified"] = certified
    result["status"] = "CERTIFIED" if certified else "BLOCKED"
    return result


def write_synthetic_evidence(root: Path, platform: str, profile_id: str) -> Path:
    matrix = load_matrix()
    budget = next(item for item in matrix["budgets"] if item["profileId"] == profile_id)
    tier = profile_id.rsplit(".", 1)[-1]
    stem = f"{platform.lower()}-{tier}"
    capture_rel = f"captures/{stem}.csv"
    screenshot_rel = f"screenshots/{stem}.png"
    capture = root / capture_rel
    screenshot = root / screenshot_rel
    capture.parent.mkdir(parents=True, exist_ok=True)
    screenshot.parent.mkdir(parents=True, exist_ok=True)
    capture.write_bytes(b"synthetic-v8-self-test-capture-" + stem.encode("utf-8"))
    screenshot.write_bytes(b"synthetic-v8-self-test-screenshot-" + stem.encode("utf-8"))
    scenario = {
        "frameSamples": MIN_FRAME_SAMPLES,
        "averageFrameTimeMs": min(10.0, budget["maxP95FrameTimeMs"] * 0.7),
        "p95FrameTimeMs": budget["maxP95FrameTimeMs"] * 0.8,
        "gameThreadP95Ms": budget["maxGameThreadP95Ms"] * 0.8,
        "renderThreadP95Ms": budget["maxRenderThreadP95Ms"] * 0.8,
        "gpuP95Ms": budget["maxGpuP95Ms"] * 0.8,
        "peakDrawCalls": max(1, int(budget["maxDrawCalls"] * 0.8)),
        "peakVisibleTriangles": max(1, int(budget["maxVisibleTriangles"] * 0.8)),
        "peakResidentTextureMB": max(1, int(budget["maxResidentTextureMB"] * 0.8)),
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
    evidence_path = root / f"{stem}.json"
    evidence_path.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    return evidence_path


def self_test() -> int:
    expected = "1" * 40
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        if assess(root, expected)["status"] != "BLOCKED":
            raise SystemExit("Empty evidence directory must fail closed")

        write_synthetic_evidence(root, "Android", "performance.tablet.low")
        if assess(root, expected)["status"] != "BLOCKED":
            raise SystemExit("One platform/profile must not certify")

        write_synthetic_evidence(root, "Android", "performance.tablet.medium")
        if assess(root, expected)["status"] != "BLOCKED":
            raise SystemExit("Missing iPadOS/high evidence must not certify")

        write_synthetic_evidence(root, "iPadOS", "performance.tablet.high")
        if assess(root, expected)["status"] != "CERTIFIED":
            raise SystemExit("Dual-platform all-tier compliant evidence should certify in self-test")

        android_low = root / "android-low.json"
        payload = json.loads(android_low.read_text(encoding="utf-8"))
        payload["scenarios"][0]["peakDrawCalls"] = 999999
        android_low.write_text(json.dumps(payload), encoding="utf-8")
        if assess(root, expected)["status"] != "BLOCKED":
            raise SystemExit("Any over-budget metric must block certification")

        payload["scenarios"][0]["peakDrawCalls"] = 1
        payload["buildCommit"] = "2" * 40
        android_low.write_text(json.dumps(payload), encoding="utf-8")
        if assess(root, expected)["status"] != "BLOCKED":
            raise SystemExit("Evidence from another build commit must block certification")

    print("V8 assessor self-test passed: fail-closed, dual-platform, all-tier, same-build, integrity and budget rules verified.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-dir", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--expected-commit")
    parser.add_argument("--require-certified", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    if args.self_test:
        return self_test()
    if not args.evidence_dir:
        parser.error("--evidence-dir is required unless --self-test is used")

    result = assess(args.evidence_dir, args.expected_commit)
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
