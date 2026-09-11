#!/usr/bin/env python3
"""Validate N3 native release certification source contracts."""
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "content/visual/certification/n3-native-release-certification.json"
SCHEMA = ROOT / "content/visual/certification/n3-native-package-evidence.schema.json"
REVIEW_EXAMPLE = ROOT / "docs/templates/n3-release-review.example.json"
ASSESSOR = ROOT / "scripts/assess-n3-native-release-certification.py"
COLLECTOR = ROOT / "scripts/collect-n3-package-evidence.py"
WORKFLOW = ROOT / ".github/workflows/n3-native-release-certification.yml"
DOC = ROOT / "docs/n3-native-release-certification.md"
REPO_QUALITY = ROOT / ".github/workflows/repo-quality.yml"
V8_ASSESSOR = ROOT / "scripts/assess-v8-visual-certification.py"
P5_VALIDATOR = ROOT / "scripts/validate-p5-art-polish-certification.py"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit("N3 validation failed: " + message)


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def main() -> None:
    required = (CONTRACT, SCHEMA, REVIEW_EXAMPLE, ASSESSOR, COLLECTOR, WORKFLOW, DOC, REPO_QUALITY, V8_ASSESSOR, P5_VALIDATOR)
    missing = [str(path.relative_to(ROOT)) for path in required if not path.is_file()]
    require(not missing, f"missing files: {missing}")

    contract = load(CONTRACT)
    require(contract.get("schemaVersion") == 1 and contract.get("phase") == "N3", "invalid N3 identity")
    require(contract.get("unrealVersion") == "5.8.2", "N3 must lock Unreal Engine 5.8.2")
    require(set(contract.get("requiredPlatforms", [])) == {"Android", "iPadOS"}, "N3 platform matrix drifted")
    profiles = {
        "performance.tablet.low",
        "performance.tablet.medium",
        "performance.tablet.high",
    }
    require(set(contract.get("requiredProfiles", [])) == profiles, "N3 profile matrix drifted")
    expected_pairs = {f"{platform}/{profile}" for platform in ("Android", "iPadOS") for profile in profiles}
    require(set(contract.get("requiredPlatformProfilePairs", [])) == expected_pairs, "N3 must require exactly six platform/profile pairs")
    require(len(contract["requiredPlatformProfilePairs"]) == 6, "N3 pair list must contain exactly six unique rows")
    require(contract["packageRules"]["Android"]["allowedKinds"] == ["apk", "aab"], "Android package kinds drifted")
    require(contract["packageRules"]["iPadOS"]["allowedKinds"] == ["ipa"], "iPadOS package kind drifted")
    require(int(contract["smokeTest"]["minimumSessionSeconds"]) >= 120, "N3 smoke session floor cannot be weakened below 120 seconds")
    require(len(contract["smokeTest"]["requiredChecks"]) == 7, "N3 smoke path must preserve seven gameplay checks")

    rules = contract.get("certificationRules", {})
    for key in (
        "n2ReleaseCandidateRequired",
        "p5CertifiedRequired",
        "v8CertifiedRequired",
        "exactSixPlatformProfilePairsRequired",
        "singleBuildCommitRequired",
        "packageHashRequired",
        "packageSignatureVerificationRequired",
        "installVerificationRequired",
        "launchVerificationRequired",
        "automationVerificationRequired",
        "runtimeTakeoverVerificationRequired",
        "v8EvidenceLinkRequired",
        "zeroCrashRequirement",
        "humanReleaseReviewRequired",
        "certificationIsFailClosed",
    ):
        require(rules.get(key) is True, f"N3 fail-closed rule missing: {key}")
    require(rules.get("sourceCiCanSelfCertify") is False, "source CI may never self-certify N3")
    boundary = contract.get("productionBoundary", {})
    require(boundary.get("nativePackagesPresent") is False and boundary.get("representativeDeviceEvidencePresent") is False and boundary.get("releaseCertified") is False, "source manifest must not claim native release evidence")

    schema = load(SCHEMA)
    require(schema.get("type") == "object" and schema.get("additionalProperties") is False, "N3 package schema must be closed")
    props = schema.get("properties", {})
    required_props = set(schema.get("required", []))
    for key in ("packageSha256", "installLogSha256", "runtimeLogSha256", "screenshotSha256", "v8EvidenceSha256", "signatureVerified", "runtimeTakeoverVerified", "crashCount", "smokeChecks"):
        require(key in props and key in required_props, f"N3 schema missing required property: {key}")
    forbidden = {"serialNumber", "deviceId", "advertisingId", "accountId", "childProfileId", "email", "biometric"}
    require(not (forbidden & set(props)), "N3 evidence schema may not collect persistent device/user identifiers")

    review = load(REVIEW_EXAMPLE)
    require(review.get("status") == "pending" and review.get("buildCommit") == "0" * 40, "N3 review example must remain unapproved")
    require(not any((review.get("checks") or {}).values()), "N3 review example cannot pre-approve release checks")

    assessor = ASSESSOR.read_text(encoding="utf-8")
    for token in (
        '"RELEASE_CERTIFIED"',
        "--require-release-certified",
        "requiredPlatformProfilePairs",
        "Counter(pairs)",
        "packageSha256",
        "v8EvidenceSha256",
        "crashCount",
        "runtimeTakeoverVerified",
        "humanReleaseReview",
        "--self-test",
    ):
        require(token in assessor, f"N3 assessor missing: {token}")
    run = subprocess.run([sys.executable, str(ASSESSOR), "--self-test"], cwd=ROOT, capture_output=True, text=True, check=False)
    require(run.returncode == 0, "N3 assessor self-test failed:\n" + run.stdout + run.stderr)

    collector = COLLECTOR.read_text(encoding="utf-8")
    for token in ("--signature-verified", "--install-verified", "--launch-verified", "--automation-passed", "--runtime-takeover-verified", '"passed" if passed else "failed"'):
        require(token in collector, f"N3 evidence collector missing explicit verification control: {token}")

    v8 = V8_ASSESSOR.read_text(encoding="utf-8")
    for token in ("REQUIRED_PLATFORM_PROFILE_PAIRS", "platformProfilePairsPresent", "Five of six platform/profile pairs must not certify", "Counter(pairs)"):
        require(token in v8, f"V8 exact-six hardening missing: {token}")
    v8_run = subprocess.run([sys.executable, str(V8_ASSESSOR), "--self-test"], cwd=ROOT, capture_output=True, text=True, check=False)
    require(v8_run.returncode == 0, "V8 hardened assessor self-test failed:\n" + v8_run.stdout + v8_run.stderr)

    p5_validator = P5_VALIDATOR.read_text(encoding="utf-8")
    require("all(authored_flags) or not any(authored_flags)" in p5_validator, "P5 source gate must allow fail-closed pre-activation and complete N2 post-activation states")

    workflow = WORKFLOW.read_text(encoding="utf-8")
    for token in ("workflow_dispatch:", "N3_RELEASE_COORDINATOR_ENABLED", "N3_RELEASE_EVIDENCE_ROOT", "self-hosted", "release-coordinator", "assess-n3-native-release-certification.py", "--require-release-certified", "n3-native-release-certification-${{ github.sha }}"):
        require(token in workflow, f"N3 workflow missing: {token}")
    require("pull_request:" not in workflow and "push:" not in workflow, "N3 native certification must remain manually dispatched")

    docs = DOC.read_text(encoding="utf-8").lower()
    for phrase in ("release_certified", "exactly one evidence package", "six native package proofs", "sha-256", "zero", "source ci cannot self-certify", "child-safety"):
        require(phrase in docs, f"N3 documentation missing: {phrase}")

    repo_quality = REPO_QUALITY.read_text(encoding="utf-8")
    require("python scripts/validate-n3-native-release-certification.py" in repo_quality, "Repository Quality must execute N3 source gate")

    print("N3 native release source contract validated: exact six-pair matrix, dependency-chain composition, package/log integrity, zero-crash smoke evidence, human release review and manual native coordinator gate are wired.")


if __name__ == "__main__":
    main()
