#!/usr/bin/env python3
"""Validate first-person native review/activation source contracts and fail-closed behavior."""
from __future__ import annotations

import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REVIEW = ROOT / "content/visual/first-person/first-person-native-review-v1.json"
REVIEW_STAGED = ROOT / "game/Content/WorldMakers/Visual/first-person-native-review-v1.json"
ACTIVATION = ROOT / "content/visual/first-person/first-person-native-activation-v1.json"
ACTIVATION_STAGED = ROOT / "game/Content/WorldMakers/Visual/first-person-native-activation-v1.json"
EVIDENCE_TEMPLATE = ROOT / "content/visual/first-person/first-person-native-review-evidence.template.json"
PACK = ROOT / "content/visual/first-person/first-person-authored-pack-v1.json"
PACK_STAGED = ROOT / "game/Content/WorldMakers/Visual/first-person-authored-pack-v1.json"
ASSESSOR = ROOT / "scripts/assess-first-person-native-review.py"
PREPARE = ROOT / "scripts/prepare-first-person-native-activation.py"
IMPORTER = ROOT / "scripts/unreal-import-first-person-authored.py"
RUNTIME_H = ROOT / "game/Source/WorldMakers/Visual/WMFirstPersonNativeActivationRuntime.h"
RUNTIME_CPP = ROOT / "game/Source/WorldMakers/Visual/WMFirstPersonNativeActivationRuntime.cpp"
BRIDGE_H = ROOT / "game/Source/WorldMakers/Visual/WMFirstPersonAuthoredBridgeSubsystem.h"
BRIDGE_CPP = ROOT / "game/Source/WorldMakers/Visual/WMFirstPersonAuthoredBridgeSubsystem.cpp"
TESTS = ROOT / "game/Source/WorldMakers/Private/Tests/WMFirstPersonNativeActivationTests.cpp"
DOC = ROOT / "docs/first-person-native-review-activation-v1.md"
WORKFLOW = ROOT / ".github/workflows/first-person-native-review-activation.yml"
QUALITY = ROOT / ".github/workflows/repo-quality.yml"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit("First-person native review/activation validation failed: " + message)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_json(path: Path, payload: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def run_assessor(target: str, native: Path, evidence: Path, output: Path, expect_success: bool) -> None:
    result = subprocess.run([
        sys.executable, str(ASSESSOR),
        "--target-commit", target,
        "--contract", str(REVIEW),
        "--native-report", str(native),
        "--review-evidence", str(evidence),
        "--output", str(output),
    ], capture_output=True, text=True)
    if expect_success:
        require(result.returncode == 0, f"valid review self-test failed: {result.stdout} {result.stderr}")
    else:
        require(result.returncode != 0, "invalid review evidence unexpectedly produced an activation candidate")


def self_test() -> None:
    target = "0123456789abcdef0123456789abcdef01234567"
    scenarios = json.loads(REVIEW.read_text(encoding="utf-8"))["reviewScenarios"]
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        native = root / "native-import-report.json"
        native_payload = {
            "schemaVersion": 1,
            "status": "imported-pending-human-review",
            "commitSha": target,
            "allFiveAssetsImported": True,
            "allNineAnimationsImported": True,
            "allSourceImportsComplete": True,
        }
        write_json(native, native_payload)

        captures_dir = root / "captures"
        captures_dir.mkdir()
        captures = []
        for index, scenario in enumerate(scenarios):
            capture = captures_dir / f"capture-{index}.png"
            capture.write_bytes(("synthetic-review-capture:" + scenario).encode("utf-8"))
            captures.append({"scenario": scenario, "path": f"captures/{capture.name}", "sha256": sha(capture)})

        evidence = root / "native-review-evidence.json"
        evidence_payload = {
            "schemaVersion": 1,
            "evidenceId": "visual.first-person-native-review-evidence.v1",
            "commitSha": target,
            "nativeImportReportSha256": sha(native),
            "captures": captures,
            "metrics": {
                "maxHandToolGapCm": 1.0,
                "maxToolPenetrationCm": 0.25,
                "maxElbowStretchRatio": 1.05,
                "maxWristTwistDegrees": 60.0,
                "nearPlaneClipFrames": 0,
                "reticleObstructionFrames": 0,
                "maxToolScreenFraction": 0.18,
                "materialSlotsArms": 1,
                "materialSlotsScanner": 2,
                "materialSlotsBuildTool": 2,
                "materialSlotsMeasureTool": 2,
                "materialSlotsWrist": 2,
            },
            "checks": {
                "automationTestsPassed": True,
                "cameraComfortApproved": True,
                "reducedMotionApproved": True,
                "toolContactApproved": True,
                "deformationApproved": True,
                "humanReviewApproved": True,
                "deviceReviewApproved": True,
            },
        }
        write_json(evidence, evidence_payload)
        candidate = root / "candidate.json"
        run_assessor(target, native, evidence, candidate, True)
        candidate_payload = json.loads(candidate.read_text(encoding="utf-8"))
        require(candidate_payload["status"] == "ACTIVATION_CANDIDATE", "valid self-test did not produce ACTIVATION_CANDIDATE")
        require(candidate_payload["targetCommitSha"] == target, "activation candidate commit drifted")

        prepared = root / "prepared"
        result = subprocess.run([
            sys.executable, str(PREPARE),
            "--candidate", str(candidate),
            "--canonical-pack", str(PACK),
            "--staged-pack", str(PACK_STAGED),
            "--canonical-activation", str(ACTIVATION),
            "--staged-activation", str(ACTIVATION_STAGED),
            "--output-dir", str(prepared),
        ], capture_output=True, text=True)
        require(result.returncode == 0, f"activation preparation self-test failed: {result.stdout} {result.stderr}")
        prepared_pack = json.loads((prepared / "content/visual/first-person/first-person-authored-pack-v1.json").read_text(encoding="utf-8"))
        prepared_activation = json.loads((prepared / "content/visual/first-person/first-person-native-activation-v1.json").read_text(encoding="utf-8"))
        require(all(item["authoredPresent"] for item in prepared_pack["assets"]), "prepared pack did not activate all five assets")
        require(all(item["authoredPresent"] for item in prepared_pack["animations"]), "prepared pack did not activate all nine animations")
        require(prepared_activation["activated"] is True and prepared_activation["targetCommitSha"] == target, "prepared activation manifest is invalid")

        clipping = dict(evidence_payload)
        clipping["metrics"] = dict(evidence_payload["metrics"])
        clipping["metrics"]["nearPlaneClipFrames"] = 1
        clipping_path = root / "clipping.json"
        write_json(clipping_path, clipping)
        run_assessor(target, native, clipping_path, root / "should-not-exist.json", False)

        wrong_commit_native = dict(native_payload)
        wrong_commit_native["commitSha"] = "fedcba9876543210fedcba9876543210fedcba98"
        wrong_native = root / "wrong-native.json"
        write_json(wrong_native, wrong_commit_native)
        wrong_evidence = dict(evidence_payload)
        wrong_evidence["nativeImportReportSha256"] = sha(wrong_native)
        wrong_evidence_path = root / "wrong-evidence.json"
        write_json(wrong_evidence_path, wrong_evidence)
        run_assessor(target, wrong_native, wrong_evidence_path, root / "wrong-candidate.json", False)


def main() -> None:
    required = (
        REVIEW, REVIEW_STAGED, ACTIVATION, ACTIVATION_STAGED, EVIDENCE_TEMPLATE, PACK, PACK_STAGED,
        ASSESSOR, PREPARE, IMPORTER, RUNTIME_H, RUNTIME_CPP, BRIDGE_H, BRIDGE_CPP, TESTS, DOC, WORKFLOW, QUALITY,
    )
    missing = [str(path.relative_to(ROOT)) for path in required if not path.is_file()]
    require(not missing, f"missing files: {missing}")
    require(REVIEW.read_bytes() == REVIEW_STAGED.read_bytes(), "review contract canonical/staged parity failed")
    require(ACTIVATION.read_bytes() == ACTIVATION_STAGED.read_bytes(), "activation manifest canonical/staged parity failed")
    require(PACK.read_bytes() == PACK_STAGED.read_bytes(), "authored pack canonical/staged parity failed")

    review = json.loads(REVIEW.read_text(encoding="utf-8"))
    require(review.get("reviewId") == "visual.first-person-native-review.v1", "review contract identity drifted")
    require(set(review["reviewScenarios"]) == {
        "explore-tool-raise-lower", "scan-anticipate-hold-settle", "build-point-confirm",
        "measure-focus", "observe-focus", "reduced-motion-pass",
    }, "review scenario matrix drifted")
    thresholds = review["qualityThresholds"]
    require(thresholds["maxHandToolGapCm"] <= 1.5, "hand/tool gap threshold weakened")
    require(thresholds["maxToolPenetrationCm"] <= 0.5, "tool penetration threshold weakened")
    require(thresholds["maxElbowStretchRatio"] <= 1.10, "elbow stretch threshold weakened")
    require(thresholds["maxNearPlaneClipFrames"] == 0 and thresholds["maxReticleObstructionFrames"] == 0, "clipping/reticle tolerance must remain zero")
    require(thresholds["maxToolScreenFraction"] <= 0.22, "tool screen fraction threshold weakened")

    activation = json.loads(ACTIVATION.read_text(encoding="utf-8"))
    require(activation.get("activationId") == "visual.first-person-native-activation.v1", "activation identity drifted")
    require(activation.get("status") == "blocked" and activation.get("activated") is False, "source activation must remain fail-closed before native evidence")
    require(all(not activation[key] for key in ("allFiveAssetsApproved", "allNineAnimationsApproved", "humanReviewApproved", "deviceReviewApproved")), "source activation approvals must remain false")

    pack = json.loads(PACK.read_text(encoding="utf-8"))
    require(all(item["authoredPresent"] is False for item in pack["assets"]), "source pack assets must remain unactivated")
    require(all(item["authoredPresent"] is False for item in pack["animations"]), "source pack animations must remain unactivated")

    importer = IMPORTER.read_text(encoding="utf-8")
    for token in ("--commit-sha", "commitSha", "normalized_commit"):
        require(token in importer, f"native importer is not exact-commit aware: {token}")

    runtime = RUNTIME_H.read_text(encoding="utf-8") + "\n" + RUNTIME_CPP.read_text(encoding="utf-8")
    for token in ("FWMFirstPersonNativeActivationState", "AllowsProductionTakeover", "TryLoadPackagedState", "first-person-native-activation-v1.json"):
        require(token in runtime, f"activation runtime missing token: {token}")
    bridge = BRIDGE_H.read_text(encoding="utf-8") + "\n" + BRIDGE_CPP.read_text(encoding="utf-8")
    for token in ("WMEnableFirstPersonAuthored", "WMBuildCommit=", "bReviewTakeoverEnabled", "bProductionActivationApproved", "AllowsProductionTakeover"):
        require(token in bridge, f"authored bridge missing review/production boundary: {token}")
    for forbidden in ("AddMovementInput", "TryPlaceCurrentPiece", "RecordComposableEvidence", "GrantReward", "SetGlobalTimeDilation"):
        require(forbidden not in bridge, f"activation bridge acquired gameplay authority: {forbidden}")

    tests = TESTS.read_text(encoding="utf-8")
    for name in (
        "WorldMakers.Visual.FirstPersonNativeActivation.BlockedManifestFailsClosed",
        "WorldMakers.Visual.FirstPersonNativeActivation.ExactCommitAllowsProductionTakeover",
        "WorldMakers.Visual.FirstPersonNativeActivation.WrongCommitFailsClosed",
    ):
        require(name in tests, f"missing activation automation test: {name}")

    workflow = WORKFLOW.read_text(encoding="utf-8")
    for token in ("target_commit", "review_evidence_path", "--commit-sha", "WMEnableFirstPersonAuthored", "WMBuildCommit", "assess-first-person-native-review.py", "prepare-first-person-native-activation.py"):
        require(token in workflow, f"native activation workflow missing token: {token}")
    quality = QUALITY.read_text(encoding="utf-8")
    require("python scripts/validate-first-person-native-review-activation.py" in quality, "Repository Quality must execute native review/activation gate")

    doc = DOC.read_text(encoding="utf-8").lower()
    for phrase in ("review takeover", "production takeover", "1.5 cm", "0 frames", "activation_candidate", "exact commit"):
        require(phrase in doc, f"documentation missing: {phrase}")

    self_test()
    print("First-person native review/activation validated: exact-commit native reports, six hash-verified review scenarios, quantified deformation/contact/camera gates, fail-closed runtime activation and non-mutating activation preparation are wired.")


if __name__ == "__main__":
    main()
