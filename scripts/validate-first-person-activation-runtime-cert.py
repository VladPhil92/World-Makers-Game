#!/usr/bin/env python3
"""Validate first-person activation commit and runtime takeover certification infrastructure."""
from __future__ import annotations

import hashlib
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PROV = ROOT / "content/visual/first-person/first-person-build-provenance-v1.json"
PROV_STAGED = ROOT / "game/Content/WorldMakers/Visual/first-person-build-provenance-v1.json"
ACTIVATION = ROOT / "content/visual/first-person/first-person-native-activation-v1.json"
PACK = ROOT / "content/visual/first-person/first-person-authored-pack-v1.json"
GEN = ROOT / "scripts/generate-first-person-build-provenance.py"
GUARD = ROOT / "scripts/verify-first-person-activation-commit.py"
ASSESS = ROOT / "scripts/assess-first-person-runtime-takeover.py"
RUNTIME_H = ROOT / "game/Source/WorldMakers/Visual/WMFirstPersonNativeActivationRuntime.h"
RUNTIME_CPP = ROOT / "game/Source/WorldMakers/Visual/WMFirstPersonNativeActivationRuntime.cpp"
BRIDGE_H = ROOT / "game/Source/WorldMakers/Visual/WMFirstPersonAuthoredBridgeSubsystem.h"
BRIDGE_CPP = ROOT / "game/Source/WorldMakers/Visual/WMFirstPersonAuthoredBridgeSubsystem.cpp"
TESTS = ROOT / "game/Source/WorldMakers/Private/Tests/WMFirstPersonNativeActivationTests.cpp"
DOC = ROOT / "docs/first-person-activation-runtime-certification-v1.md"
WORKFLOW = ROOT / ".github/workflows/first-person-activation-runtime-cert.yml"
QUALITY = ROOT / ".github/workflows/repo-quality.yml"
SHA1_RE = re.compile(r"^[0-9a-f]{40}$")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit("First-person activation/runtime certification validation failed: " + message)


def write_json(path: Path, payload: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run_assessor(activation: Path, provenance: Path, report: Path, build: str, output: Path, success: bool) -> None:
    result = subprocess.run([
        sys.executable, str(ASSESS), "--activation", str(activation), "--provenance", str(provenance),
        "--runtime-report", str(report), "--expected-build-commit", build, "--output", str(output),
    ], capture_output=True, text=True)
    if success:
        require(result.returncode == 0, f"valid takeover assessor self-test failed: {result.stdout} {result.stderr}")
    else:
        require(result.returncode != 0, "invalid takeover evidence unexpectedly certified")


def git(cwd: Path, *args: str) -> str:
    return subprocess.run(["git", *args], cwd=cwd, capture_output=True, text=True, check=True).stdout.strip()


def guard_self_test() -> None:
    with tempfile.TemporaryDirectory() as temp:
        repo = Path(temp)
        git(repo, "init"); git(repo, "config", "user.email", "ci@example.invalid"); git(repo, "config", "user.name", "World Makers CI")
        pack_paths = [repo / "content/visual/first-person/first-person-authored-pack-v1.json", repo / "game/Content/WorldMakers/Visual/first-person-authored-pack-v1.json"]
        activation_paths = [repo / "content/visual/first-person/first-person-native-activation-v1.json", repo / "game/Content/WorldMakers/Visual/first-person-native-activation-v1.json"]
        blocked_pack = {"assets": [{"id": f"asset-{i}", "authoredPresent": False} for i in range(5)], "animations": [{"id": f"anim-{i}", "authoredPresent": False} for i in range(9)]}
        blocked_activation = {"status": "blocked", "activated": False, "targetCommitSha": "", "allFiveAssetsApproved": False, "allNineAnimationsApproved": False, "humanReviewApproved": False, "deviceReviewApproved": False}
        for p in pack_paths: write_json(p, blocked_pack)
        for p in activation_paths: write_json(p, blocked_activation)
        git(repo, "add", "."); git(repo, "commit", "-m", "reviewed source")
        reviewed = git(repo, "rev-parse", "HEAD")
        activated_pack = {"assets": [{"id": f"asset-{i}", "authoredPresent": True} for i in range(5)], "animations": [{"id": f"anim-{i}", "authoredPresent": True} for i in range(9)]}
        activated = {"status": "activated", "activated": True, "targetCommitSha": reviewed, "allFiveAssetsApproved": True, "allNineAnimationsApproved": True, "humanReviewApproved": True, "deviceReviewApproved": True}
        for p in pack_paths: write_json(p, activated_pack)
        for p in activation_paths: write_json(p, activated)
        git(repo, "add", "."); git(repo, "commit", "-m", "activation manifests")
        ok = subprocess.run([sys.executable, str(GUARD), "--reviewed-source", reviewed, "--repo-root", str(repo)], capture_output=True, text=True)
        require(ok.returncode == 0, f"valid activation-only diff failed guard: {ok.stdout} {ok.stderr}")
        extra = repo / "game/runtime.cpp"; extra.parent.mkdir(parents=True, exist_ok=True); extra.write_text("unexpected runtime change\n")
        git(repo, "add", "."); git(repo, "commit", "-m", "unexpected runtime mutation")
        bad = subprocess.run([sys.executable, str(GUARD), "--reviewed-source", reviewed, "--repo-root", str(repo)], capture_output=True, text=True)
        require(bad.returncode != 0, "activation diff guard accepted runtime mutation")


def provenance_and_assessor_self_test() -> None:
    reviewed = "0123456789abcdef0123456789abcdef01234567"
    build = "1111111111111111111111111111111111111111"
    candidate = "c" * 64
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        activation = root / "activation.json"
        write_json(activation, {
            "schemaVersion": 1, "activationId": "visual.first-person-native-activation.v1", "status": "activated", "activated": True,
            "targetCommitSha": reviewed, "nativeImportReportSha256": "a" * 64, "reviewEvidenceSha256": "b" * 64,
            "activationCandidateSha256": candidate, "allFiveAssetsApproved": True, "allNineAnimationsApproved": True,
            "humanReviewApproved": True, "deviceReviewApproved": True,
        })
        provenance = root / "provenance.json"
        result = subprocess.run([sys.executable, str(GEN), "--build-commit", build, "--activation-manifest", str(activation), "--output", str(provenance)], capture_output=True, text=True)
        require(result.returncode == 0, f"provenance generator failed: {result.stdout} {result.stderr}")
        prov = json.loads(provenance.read_text())
        require(prov["status"] == "bound" and prov["buildCommitSha"] == build and prov["reviewedSourceCommitSha"] == reviewed, "generated provenance identity drifted")
        require(prov["activationCandidateSha256"] == candidate and prov["activationManifestSha256"] == sha256(activation), "generated provenance fingerprint drifted")
        valid_report = {
            "schemaVersion": 1, "reportId": "visual.first-person-runtime-takeover.v1", "status": "TAKEOVER_ACTIVE",
            "buildCommitSha": build, "reviewedSourceCommitSha": reviewed, "activationCandidateSha256": candidate,
            "reviewTakeoverEnabled": False, "productionActivationApproved": True, "certificationMode": True,
            "authoredSetComplete": True, "animationCount": 9, "firstPersonInteractionActive": True, "takeoverActive": True,
            "authoredArmsVisible": True, "proxyHandsHidden": True, "toolMatchesAuthored": True, "wristMatchesAuthored": True,
            "activeModeId": "firstperson.scan", "activeActionId": "scan-hold",
        }
        report = root / "report.json"; write_json(report, valid_report)
        certification = root / "cert.json"; run_assessor(activation, provenance, report, build, certification, True)
        require(json.loads(certification.read_text())["status"] == "TAKEOVER_CERTIFIED", "valid takeover did not certify")
        for field, value in (("reviewTakeoverEnabled", True), ("proxyHandsHidden", False), ("toolMatchesAuthored", False)):
            bad_report = dict(valid_report); bad_report[field] = value
            if field != "reviewTakeoverEnabled": bad_report["status"] = "BLOCKED_OR_FALLBACK"
            bad_path = root / f"bad-{field}.json"; write_json(bad_path, bad_report)
            run_assessor(activation, provenance, bad_path, build, root / f"bad-{field}-cert.json", False)
        run_assessor(activation, provenance, report, "2" * 40, root / "wrong-build.json", False)


def main() -> None:
    required = (PROV, PROV_STAGED, ACTIVATION, PACK, GEN, GUARD, ASSESS, RUNTIME_H, RUNTIME_CPP, BRIDGE_H, BRIDGE_CPP, TESTS, DOC, WORKFLOW, QUALITY)
    missing = [str(p.relative_to(ROOT)) for p in required if not p.is_file()]
    require(not missing, f"missing files: {missing}")
    require(PROV.read_bytes() == PROV_STAGED.read_bytes(), "build provenance canonical/staged parity failed")
    provenance = json.loads(PROV.read_text())
    require(provenance.get("provenanceId") == "visual.first-person-build-provenance.v1" and provenance.get("status") == "unbound", "repository provenance template must remain unbound")
    require(all(provenance.get(k, "") == "" for k in ("buildCommitSha", "reviewedSourceCommitSha", "activationCandidateSha256", "activationManifestSha256")), "unbound template must not contain release identity")

    activation = json.loads(ACTIVATION.read_text())
    pack = json.loads(PACK.read_text())
    require(len(pack.get("assets", [])) == 5 and len(pack.get("animations", [])) == 9, "authored pack cardinality drifted")
    flags = [bool(item["authoredPresent"]) for item in pack["assets"] + pack["animations"]]
    all_off, all_on = not any(flags), all(flags)
    require(all_off or all_on, "partial activation is forbidden")
    if all_off:
        require(activation.get("status") == "blocked" and activation.get("activated") is False, "all-off pack requires blocked activation")
    else:
        require(activation.get("status") == "activated" and activation.get("activated") is True, "all-on pack requires activated manifest")
        require(SHA1_RE.fullmatch(str(activation.get("targetCommitSha", ""))) is not None, "activated manifest reviewed source SHA invalid")
        for key in ("allFiveAssetsApproved", "allNineAnimationsApproved", "humanReviewApproved", "deviceReviewApproved"):
            require(activation.get(key) is True, f"activated approval missing: {key}")

    runtime = RUNTIME_H.read_text() + "\n" + RUNTIME_CPP.read_text()
    for token in ("FWMFirstPersonBuildProvenance", "ReviewedSourceCommitSha", "TryLoadPackagedBuildProvenance", "PackagedBuildProvenanceRelativePath", "ActivationCandidateSha256"):
        require(token in runtime, f"activation runtime missing: {token}")
    require("TargetCommitSha == Provenance.ReviewedSourceCommitSha" in runtime, "runtime must bind reviewed source instead of self-referential build SHA")
    bridge = BRIDGE_H.read_text() + "\n" + BRIDGE_CPP.read_text()
    for token in ("WMFirstPersonTakeoverCertificationMode", "WMFirstPersonTakeoverReport=", "TryLoadPackagedBuildProvenance", "TAKEOVER_ACTIVE", "proxyHandsHidden", "toolMatchesAuthored", "wristMatchesAuthored", "SetPersistentModeById"):
        require(token in bridge, f"bridge certification token missing: {token}")
    for forbidden in ("AddMovementInput", "TryPlaceCurrentPiece", "RecordComposableEvidence", "GrantReward", "SetGlobalTimeDilation"):
        require(forbidden not in bridge, f"takeover certification acquired gameplay authority: {forbidden}")

    tests = TESTS.read_text()
    for name in ("WorldMakers.Visual.FirstPersonNativeActivation.BlockedManifestFailsClosed", "WorldMakers.Visual.FirstPersonNativeActivation.ExactCommitAllowsProductionTakeover", "WorldMakers.Visual.FirstPersonNativeActivation.WrongCommitFailsClosed", "WorldMakers.Visual.FirstPersonNativeActivation.UnboundBuildProvenanceFailsClosed"):
        require(name in tests, f"missing native activation test: {name}")
    workflow = WORKFLOW.read_text()
    for token in ("reviewed_source_commit", "verify-first-person-activation-commit.py", "generate-first-person-build-provenance.py", "WMFirstPersonTakeoverCertificationMode", "WMFirstPersonTakeoverReport", "assess-first-person-runtime-takeover.py"):
        require(token in workflow, f"runtime certification workflow missing: {token}")
    require("python scripts/validate-first-person-activation-runtime-cert.py" in QUALITY.read_text(), "Repository Quality must execute runtime certification gate")
    doc = DOC.read_text().lower()
    for phrase in ("self-reference", "reviewed source commit", "activation commit", "build provenance", "takeover_certified", "proxy rollback"):
        require(phrase in doc, f"documentation missing: {phrase}")

    provenance_and_assessor_self_test(); guard_self_test()
    state = "all-on/activated" if all_on else "all-off/blocked"
    print(f"First-person activation/runtime certification validated in {state} state: reviewed-source binding, activation-only diff guard, build provenance, authored takeover evidence and proxy rollback are wired.")


if __name__ == "__main__":
    main()
