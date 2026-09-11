#!/usr/bin/env python3
"""Assess first-person authored native review evidence and emit an activation candidate."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path

SHA1_RE = re.compile(r"^[0-9a-f]{40}$")
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def fail(message: str) -> None:
    raise SystemExit("First-person native review BLOCKED: " + message)


def load_json(path: Path) -> dict:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:
        fail(f"cannot read {path}: {exc}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--target-commit", required=True)
    parser.add_argument("--contract", default="content/visual/first-person/first-person-native-review-v1.json")
    parser.add_argument("--native-report", required=True)
    parser.add_argument("--review-evidence", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    target_commit = args.target_commit.strip().lower()
    if not SHA1_RE.fullmatch(target_commit):
        fail("target commit must be a full 40-character lowercase Git SHA")

    contract_path = Path(args.contract)
    native_path = Path(args.native_report)
    review_path = Path(args.review_evidence)
    contract = load_json(contract_path)
    native = load_json(native_path)
    review = load_json(review_path)

    if contract.get("schemaVersion") != 1 or contract.get("reviewId") != "visual.first-person-native-review.v1":
        fail("review contract identity mismatch")
    if native.get("schemaVersion") != 1:
        fail("native report schema mismatch")
    if review.get("schemaVersion") != 1 or review.get("evidenceId") != "visual.first-person-native-review-evidence.v1":
        fail("review evidence identity mismatch")

    if native.get("commitSha") != target_commit:
        fail("native import report commit does not match target commit")
    if review.get("commitSha") != target_commit:
        fail("review evidence commit does not match target commit")

    if not native.get("allFiveAssetsImported") or not native.get("allNineAnimationsImported") or not native.get("allSourceImportsComplete"):
        fail("native import report is incomplete")

    native_sha = sha256_file(native_path)
    if review.get("nativeImportReportSha256") != native_sha:
        fail("review evidence does not reference the exact native import report")

    required_scenarios = set(contract.get("reviewScenarios", []))
    captures = review.get("captures", [])
    if len(captures) < int(contract["requiredEvidence"]["reviewCaptureCountMinimum"]):
        fail("not enough review captures")
    seen_scenarios: set[str] = set()
    evidence_root = review_path.parent
    for item in captures:
        scenario = item.get("scenario", "")
        rel_path = item.get("path", "")
        digest = item.get("sha256", "")
        if scenario in seen_scenarios:
            fail(f"duplicate capture scenario: {scenario}")
        seen_scenarios.add(scenario)
        if not SHA256_RE.fullmatch(digest):
            fail(f"invalid capture hash for {scenario}")
        capture_path = (evidence_root / rel_path).resolve()
        try:
            capture_path.relative_to(evidence_root.resolve())
        except ValueError:
            fail(f"capture path escapes evidence root: {rel_path}")
        if not capture_path.is_file():
            fail(f"capture missing: {rel_path}")
        if sha256_file(capture_path) != digest:
            fail(f"capture hash mismatch: {scenario}")
    if not required_scenarios.issubset(seen_scenarios):
        fail(f"missing review scenarios: {sorted(required_scenarios - seen_scenarios)}")

    thresholds = contract["qualityThresholds"]
    metrics = review.get("metrics", {})
    checks = review.get("checks", {})

    numeric_limits = {
        "maxHandToolGapCm": thresholds["maxHandToolGapCm"],
        "maxToolPenetrationCm": thresholds["maxToolPenetrationCm"],
        "maxElbowStretchRatio": thresholds["maxElbowStretchRatio"],
        "maxWristTwistDegrees": thresholds["maxWristTwistDegrees"],
        "nearPlaneClipFrames": thresholds["maxNearPlaneClipFrames"],
        "reticleObstructionFrames": thresholds["maxReticleObstructionFrames"],
        "maxToolScreenFraction": thresholds["maxToolScreenFraction"],
        "materialSlotsArms": thresholds["maxMaterialSlotsArms"],
        "materialSlotsScanner": thresholds["maxMaterialSlotsTool"],
        "materialSlotsBuildTool": thresholds["maxMaterialSlotsTool"],
        "materialSlotsMeasureTool": thresholds["maxMaterialSlotsTool"],
        "materialSlotsWrist": thresholds["maxMaterialSlotsWrist"],
    }
    for key, limit in numeric_limits.items():
        value = metrics.get(key)
        if not isinstance(value, (int, float)) or isinstance(value, bool):
            fail(f"missing numeric metric: {key}")
        if value < 0 or value > limit:
            fail(f"metric exceeds threshold: {key}={value} > {limit}")

    required_checks = (
        "automationTestsPassed",
        "cameraComfortApproved",
        "reducedMotionApproved",
        "toolContactApproved",
        "deformationApproved",
        "humanReviewApproved",
        "deviceReviewApproved",
    )
    for key in required_checks:
        if checks.get(key) is not True:
            fail(f"review check not approved: {key}")

    review_sha = sha256_file(review_path)
    candidate = {
        "schemaVersion": 1,
        "candidateId": "visual.first-person-native-activation-candidate.v1",
        "status": "ACTIVATION_CANDIDATE",
        "targetCommitSha": target_commit,
        "nativeImportReportSha256": native_sha,
        "reviewEvidenceSha256": review_sha,
        "allFiveAssetsApproved": True,
        "allNineAnimationsApproved": True,
        "humanReviewApproved": True,
        "deviceReviewApproved": True,
        "reviewScenarios": sorted(required_scenarios),
    }
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(candidate, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"ACTIVATION_CANDIDATE: {target_commit} -> {output}")


if __name__ == "__main__":
    main()
