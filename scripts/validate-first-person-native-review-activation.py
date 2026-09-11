#!/usr/bin/env python3
"""Validate first-person native review/activation contracts before or after activation."""
from __future__ import annotations

import copy
import hashlib
import json
import re
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
SHA1_RE = re.compile(r"^[0-9a-f]{40}$")


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
        sys.executable, str(ASSESSOR), "--target-commit", target,
        "--contract", str(REVIEW), "--native-report", str(native),
        "--review-evidence", str(evidence), "--output", str(output),
    ], capture_output=True, text=True)
    if expect_success:
        require(result.returncode == 0, f"valid review self-test failed: {result.stdout} {result.stderr}")
    else:
        require(result.returncode != 0, "invalid review unexpectedly produced activation candidate")


def self_test() -> None:
    target = "0123456789abcdef0123456789abcdef01234567"
    scenarios = json.loads(REVIEW.read_text(encoding="utf-8"))["reviewScenarios"]
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        native = root / "native-import-report.json"
        write_json(native, {
            "schemaVersion": 1, "status": "imported-pending-human-review", "commitSha": target,
            "allFiveAssetsImported": True, "allNineAnimationsImported": True, "allSourceImportsComplete": True,
        })
        captures = []
        for index, scenario in enumerate(scenarios):
            capture = root / "captures" / f"capture-{index}.png"
            capture.parent.mkdir(parents=True, exist_ok=True)
            capture.write_bytes(("synthetic:" + scenario).encode())
            captures.append({"scenario": scenario, "path": f"captures/{capture.name}", "sha256": sha(capture)})
        evidence_payload = {
            "schemaVersion": 1,
            "evidenceId": "visual.first-person-native-review-evidence.v1",
            "commitSha": target,
            "nativeImportReportSha256": sha(native),
            "captures": captures,
            "metrics": {
                "maxHandToolGapCm": 1.0, "maxToolPenetrationCm": 0.25, "maxElbowStretchRatio": 1.05,
                "maxWristTwistDegrees": 60.0, "nearPlaneClipFrames": 0, "reticleObstructionFrames": 0,
                "maxToolScreenFraction": 0.18, "materialSlotsArms": 1, "materialSlotsScanner": 2,
                "materialSlotsBuildTool": 2, "materialSlotsMeasureTool": 2, "materialSlotsWrist": 2,
            },
            "checks": {
                "automationTestsPassed": True, "cameraComfortApproved": True, "reducedMotionApproved": True,
                "toolContactApproved": True, "deformationApproved": True,
                "humanReviewApproved": True, "deviceReviewApproved": True,
            },
        }
        evidence = root / "evidence.json"
        write_json(evidence, evidence_payload)
        candidate = root / "candidate.json"
        run_assessor(target, native, evidence, candidate, True)
        candidate_payload = json.loads(candidate.read_text(encoding="utf-8"))
        require(candidate_payload["status"] == "ACTIVATION_CANDIDATE" and candidate_payload["targetCommitSha"] == target, "valid review did not produce candidate")

        # Preparation must remain testable even after repository activation, so use synthetic all-off inputs.
        pack_template = copy.deepcopy(json.loads(PACK.read_text(encoding="utf-8")))
        for item in pack_template["assets"] + pack_template["animations"]:
            item["authoredPresent"] = False
        pack_template["certificationBoundary"]["authoredAssetsPresent"] = False
        blocked_activation = {
            "schemaVersion": 1, "activationId": "visual.first-person-native-activation.v1",
            "status": "blocked", "activated": False, "targetCommitSha": "",
            "nativeImportReportSha256": "", "reviewEvidenceSha256": "", "activationCandidateSha256": "",
            "allFiveAssetsApproved": False, "allNineAnimationsApproved": False,
            "humanReviewApproved": False, "deviceReviewApproved": False,
        }
        cpack, spack = root / "canonical-pack.json", root / "staged-pack.json"
        cact, sact = root / "canonical-activation.json", root / "staged-activation.json"
        write_json(cpack, pack_template); write_json(spack, pack_template)
        write_json(cact, blocked_activation); write_json(sact, blocked_activation)
        prepared = root / "prepared"
        result = subprocess.run([
            sys.executable, str(PREPARE), "--candidate", str(candidate),
            "--canonical-pack", str(cpack), "--staged-pack", str(spack),
            "--canonical-activation", str(cact), "--staged-activation", str(sact),
            "--output-dir", str(prepared),
        ], capture_output=True, text=True)
        require(result.returncode == 0, f"activation preparation self-test failed: {result.stdout} {result.stderr}")
        prepared_pack = json.loads((prepared / "content/visual/first-person/first-person-authored-pack-v1.json").read_text())
        prepared_activation = json.loads((prepared / "content/visual/first-person/first-person-native-activation-v1.json").read_text())
        require(len(prepared_pack["assets"]) == 5 and all(item["authoredPresent"] for item in prepared_pack["assets"]), "prepared five-asset activation invalid")
        require(len(prepared_pack["animations"]) == 9 and all(item["authoredPresent"] for item in prepared_pack["animations"]), "prepared nine-animation activation invalid")
        require(prepared_activation["activated"] is True and prepared_activation["targetCommitSha"] == target, "prepared activation invalid")

        clipping = copy.deepcopy(evidence_payload)
        clipping["metrics"]["nearPlaneClipFrames"] = 1
        clipping_path = root / "clipping.json"; write_json(clipping_path, clipping)
        run_assessor(target, native, clipping_path, root / "bad.json", False)
        wrong_native = root / "wrong-native.json"
        wrong_payload = json.loads(native.read_text()); wrong_payload["commitSha"] = "fedcba9876543210fedcba9876543210fedcba98"; write_json(wrong_native, wrong_payload)
        wrong_evidence = copy.deepcopy(evidence_payload); wrong_evidence["nativeImportReportSha256"] = sha(wrong_native)
        wrong_evidence_path = root / "wrong-evidence.json"; write_json(wrong_evidence_path, wrong_evidence)
        run_assessor(target, wrong_native, wrong_evidence_path, root / "wrong.json", False)


def main() -> None:
    required = (REVIEW, REVIEW_STAGED, ACTIVATION, ACTIVATION_STAGED, EVIDENCE_TEMPLATE, PACK, PACK_STAGED, ASSESSOR, PREPARE, IMPORTER, RUNTIME_H, RUNTIME_CPP, BRIDGE_H, BRIDGE_CPP, TESTS, DOC, WORKFLOW, QUALITY)
    missing = [str(p.relative_to(ROOT)) for p in required if not p.is_file()]
    require(not missing, f"missing files: {missing}")
    require(REVIEW.read_bytes() == REVIEW_STAGED.read_bytes(), "review canonical/staged parity failed")
    require(ACTIVATION.read_bytes() == ACTIVATION_STAGED.read_bytes(), "activation canonical/staged parity failed")
    require(PACK.read_bytes() == PACK_STAGED.read_bytes(), "pack canonical/staged parity failed")

    review = json.loads(REVIEW.read_text(encoding="utf-8"))
    require(review.get("reviewId") == "visual.first-person-native-review.v1", "review identity drifted")
    require(set(review["reviewScenarios"]) == {"explore-tool-raise-lower", "scan-anticipate-hold-settle", "build-point-confirm", "measure-focus", "observe-focus", "reduced-motion-pass"}, "review scenarios drifted")
    thresholds = review["qualityThresholds"]
    require(thresholds["maxHandToolGapCm"] <= 1.5 and thresholds["maxToolPenetrationCm"] <= 0.5, "contact thresholds weakened")
    require(thresholds["maxElbowStretchRatio"] <= 1.10, "elbow threshold weakened")
    require(thresholds["maxNearPlaneClipFrames"] == 0 and thresholds["maxReticleObstructionFrames"] == 0, "clipping tolerance must remain zero")
    require(thresholds["maxToolScreenFraction"] <= 0.22, "tool footprint threshold weakened")

    activation = json.loads(ACTIVATION.read_text(encoding="utf-8"))
    pack = json.loads(PACK.read_text(encoding="utf-8"))
    flags = [bool(item["authoredPresent"]) for item in pack["assets"] + pack["animations"]]
    all_off, all_on = not any(flags), all(flags)
    require(all_off or all_on, "partial activation is forbidden")
    if all_off:
        require(activation.get("status") == "blocked" and activation.get("activated") is False, "all-off pack requires blocked activation")
    else:
        require(activation.get("status") == "activated" and activation.get("activated") is True, "all-on pack requires activated manifest")
        require(SHA1_RE.fullmatch(str(activation.get("targetCommitSha", ""))) is not None, "activated manifest reviewed SHA invalid")
        for key in ("allFiveAssetsApproved", "allNineAnimationsApproved", "humanReviewApproved", "deviceReviewApproved"):
            require(activation.get(key) is True, f"activated approval missing: {key}")

    importer = IMPORTER.read_text(encoding="utf-8")
    for token in ("--commit-sha", "commitSha", "normalized_commit"):
        require(token in importer, f"importer is not exact-review-commit aware: {token}")
    runtime = RUNTIME_H.read_text(encoding="utf-8") + "\n" + RUNTIME_CPP.read_text(encoding="utf-8")
    for token in ("FWMFirstPersonNativeActivationState", "AllowsProductionTakeover", "TryLoadPackagedState", "first-person-native-activation-v1.json"):
        require(token in runtime, f"activation runtime missing: {token}")
    bridge = BRIDGE_H.read_text(encoding="utf-8") + "\n" + BRIDGE_CPP.read_text(encoding="utf-8")
    for token in ("WMEnableFirstPersonAuthored", "WMBuildCommit=", "bReviewTakeoverEnabled", "bProductionActivationApproved", "AllowsProductionTakeover"):
        require(token in bridge, f"bridge review/production boundary missing: {token}")
    for forbidden in ("AddMovementInput", "TryPlaceCurrentPiece", "RecordComposableEvidence", "GrantReward", "SetGlobalTimeDilation"):
        require(forbidden not in bridge, f"activation bridge acquired gameplay authority: {forbidden}")

    tests = TESTS.read_text(encoding="utf-8")
    for name in ("WorldMakers.Visual.FirstPersonNativeActivation.BlockedManifestFailsClosed", "WorldMakers.Visual.FirstPersonNativeActivation.ExactCommitAllowsProductionTakeover", "WorldMakers.Visual.FirstPersonNativeActivation.WrongCommitFailsClosed"):
        require(name in tests, f"missing activation automation test: {name}")
    workflow = WORKFLOW.read_text(encoding="utf-8")
    for token in ("target_commit", "review_evidence_path", "--commit-sha", "WMEnableFirstPersonAuthored", "WMBuildCommit", "assess-first-person-native-review.py", "prepare-first-person-native-activation.py"):
        require(token in workflow, f"native review workflow missing: {token}")
    require("python scripts/validate-first-person-native-review-activation.py" in QUALITY.read_text(encoding="utf-8"), "Repository Quality must execute native review gate")
    doc = DOC.read_text(encoding="utf-8").lower()
    for phrase in ("review takeover", "production takeover", "1.5 cm", "0 frames", "activation_candidate", "exact commit"):
        require(phrase in doc, f"documentation missing: {phrase}")

    self_test()
    state = "all-on/activated" if all_on else "all-off/blocked"
    print(f"First-person native review/activation validated in {state} state: exact-review-commit evidence, six hash-verified scenarios, quantified quality gates and deliberate activation preparation remain intact.")


if __name__ == "__main__":
    main()
