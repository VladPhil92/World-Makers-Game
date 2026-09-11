#!/usr/bin/env python3
"""Certify production first-person authored takeover from packaged runtime evidence."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path

SHA1_RE = re.compile(r"^[0-9a-f]{40}$")
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")


def fail(message: str) -> None:
    raise SystemExit("First-person runtime takeover certification failed: " + message)


def load(path: Path) -> dict:
    if not path.is_file():
        fail(f"missing evidence file: {path}")
    return json.loads(path.read_text(encoding="utf-8"))


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--activation", required=True)
    parser.add_argument("--provenance", required=True)
    parser.add_argument("--runtime-report", required=True)
    parser.add_argument("--expected-build-commit", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    expected_build = args.expected_build_commit.strip().lower()
    if not SHA1_RE.fullmatch(expected_build):
        fail("expected build commit must be a lowercase 40-character Git SHA")

    activation_path = Path(args.activation)
    provenance_path = Path(args.provenance)
    report_path = Path(args.runtime_report)
    activation = load(activation_path)
    provenance = load(provenance_path)
    report = load(report_path)

    if activation.get("schemaVersion") != 1 or activation.get("activationId") != "visual.first-person-native-activation.v1":
        fail("activation manifest identity mismatch")
    if activation.get("status") != "activated" or activation.get("activated") is not True:
        fail("activation manifest is not activated")
    reviewed = str(activation.get("targetCommitSha", ""))
    candidate_sha = str(activation.get("activationCandidateSha256", ""))
    if not SHA1_RE.fullmatch(reviewed) or not SHA256_RE.fullmatch(candidate_sha):
        fail("activation review identity is malformed")
    for key in ("allFiveAssetsApproved", "allNineAnimationsApproved", "humanReviewApproved", "deviceReviewApproved"):
        if activation.get(key) is not True:
            fail(f"activation approval missing: {key}")

    if provenance.get("schemaVersion") != 1 or provenance.get("provenanceId") != "visual.first-person-build-provenance.v1":
        fail("build provenance identity mismatch")
    if provenance.get("status") != "bound":
        fail("build provenance is not bound")
    if provenance.get("buildCommitSha") != expected_build:
        fail("build provenance does not match expected build commit")
    if provenance.get("reviewedSourceCommitSha") != reviewed:
        fail("build provenance does not match reviewed source commit")
    if provenance.get("activationCandidateSha256") != candidate_sha:
        fail("build provenance activation candidate mismatch")
    if provenance.get("activationManifestSha256") != sha256(activation_path):
        fail("activation manifest hash does not match packaged provenance")

    if report.get("schemaVersion") != 1 or report.get("reportId") != "visual.first-person-runtime-takeover.v1":
        fail("runtime report identity mismatch")
    if report.get("status") != "TAKEOVER_ACTIVE":
        fail("runtime did not report TAKEOVER_ACTIVE")
    if report.get("buildCommitSha") != expected_build:
        fail("runtime report build commit mismatch")
    if report.get("reviewedSourceCommitSha") != reviewed:
        fail("runtime report reviewed source mismatch")
    if report.get("activationCandidateSha256") != candidate_sha:
        fail("runtime report candidate fingerprint mismatch")
    if report.get("reviewTakeoverEnabled") is not False:
        fail("runtime certification may not depend on review bypass")
    if report.get("certificationMode") is not True:
        fail("runtime report was not captured in explicit certification mode")
    if report.get("activeModeId") != "firstperson.scan":
        fail("certification mode must exercise the first-person scan presentation path")
    for key in (
        "productionActivationApproved",
        "authoredSetComplete",
        "firstPersonInteractionActive",
        "takeoverActive",
        "authoredArmsVisible",
        "proxyHandsHidden",
        "toolMatchesAuthored",
        "wristMatchesAuthored",
    ):
        if report.get(key) is not True:
            fail(f"runtime takeover condition failed: {key}")
    if int(report.get("animationCount", 0)) != 9:
        fail("runtime must load exactly nine required first-person animations")

    certification = {
        "schemaVersion": 1,
        "certificationId": "visual.first-person-runtime-takeover-certification.v1",
        "status": "TAKEOVER_CERTIFIED",
        "buildCommitSha": expected_build,
        "reviewedSourceCommitSha": reviewed,
        "activationCandidateSha256": candidate_sha,
        "activationManifestSha256": sha256(activation_path),
        "buildProvenanceSha256": sha256(provenance_path),
        "runtimeReportSha256": sha256(report_path),
        "reviewBypassUsed": False,
        "certificationMode": True,
        "authoredSet": {"assets": 5, "animations": 9},
        "proxyHandsHidden": True,
        "authoredArmsVisible": True,
        "toolAndWristAuthored": True,
    }
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(certification, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"First-person runtime takeover certified for build {expected_build}, reviewed source {reviewed}")


if __name__ == "__main__":
    main()
