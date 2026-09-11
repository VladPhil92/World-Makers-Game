#!/usr/bin/env python3
"""Prepare reviewed first-person activation manifests from a valid activation candidate.

This script deliberately writes to an output directory. It does not mutate repository files.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
import re
from pathlib import Path

SHA1_RE = re.compile(r"^[0-9a-f]{40}$")
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")


def fail(message: str) -> None:
    raise SystemExit("First-person activation preparation failed: " + message)


def read_bytes_equal(a: Path, b: Path, label: str) -> bytes:
    a_bytes = a.read_bytes()
    b_bytes = b.read_bytes()
    if a_bytes != b_bytes:
        fail(f"{label} canonical/staged files differ")
    return a_bytes


def load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path: Path, data: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--candidate", required=True)
    parser.add_argument("--canonical-pack", default="content/visual/first-person/first-person-authored-pack-v1.json")
    parser.add_argument("--staged-pack", default="game/Content/WorldMakers/Visual/first-person-authored-pack-v1.json")
    parser.add_argument("--canonical-activation", default="content/visual/first-person/first-person-native-activation-v1.json")
    parser.add_argument("--staged-activation", default="game/Content/WorldMakers/Visual/first-person-native-activation-v1.json")
    parser.add_argument("--output-dir", required=True)
    args = parser.parse_args()

    candidate_path = Path(args.candidate)
    candidate = load_json(candidate_path)
    if candidate.get("schemaVersion") != 1 or candidate.get("candidateId") != "visual.first-person-native-activation-candidate.v1":
        fail("candidate identity mismatch")
    if candidate.get("status") != "ACTIVATION_CANDIDATE":
        fail("candidate is not ACTIVATION_CANDIDATE")
    target_commit = str(candidate.get("targetCommitSha", ""))
    if not SHA1_RE.fullmatch(target_commit):
        fail("candidate target commit is invalid")
    for key in ("nativeImportReportSha256", "reviewEvidenceSha256"):
        if not SHA256_RE.fullmatch(str(candidate.get(key, ""))):
            fail(f"candidate {key} is invalid")
    for key in ("allFiveAssetsApproved", "allNineAnimationsApproved", "humanReviewApproved", "deviceReviewApproved"):
        if candidate.get(key) is not True:
            fail(f"candidate approval missing: {key}")

    canonical_pack = Path(args.canonical_pack)
    staged_pack = Path(args.staged_pack)
    canonical_activation = Path(args.canonical_activation)
    staged_activation = Path(args.staged_activation)
    read_bytes_equal(canonical_pack, staged_pack, "authored pack")
    read_bytes_equal(canonical_activation, staged_activation, "activation manifest")

    pack = load_json(canonical_pack)
    activation = load_json(canonical_activation)
    if activation.get("activated") is not False or activation.get("status") != "blocked":
        fail("source activation manifest must still be blocked before reviewed activation")
    if any(item.get("authoredPresent") is not False for item in pack.get("assets", [])):
        fail("source asset pack already contains activated assets")
    if any(item.get("authoredPresent") is not False for item in pack.get("animations", [])):
        fail("source asset pack already contains activated animations")

    candidate_sha = hashlib.sha256(candidate_path.read_bytes()).hexdigest()
    activated_pack = copy.deepcopy(pack)
    activated_pack["status"] = "native-review-approved"
    for item in activated_pack["assets"]:
        item["authoredPresent"] = True
    for item in activated_pack["animations"]:
        item["authoredPresent"] = True
    activated_pack["certificationBoundary"]["authoredAssetsPresent"] = True

    activated_manifest = copy.deepcopy(activation)
    activated_manifest.update({
        "status": "activated",
        "activated": True,
        "targetCommitSha": target_commit,
        "nativeImportReportSha256": candidate["nativeImportReportSha256"],
        "reviewEvidenceSha256": candidate["reviewEvidenceSha256"],
        "activationCandidateSha256": candidate_sha,
        "allFiveAssetsApproved": True,
        "allNineAnimationsApproved": True,
        "humanReviewApproved": True,
        "deviceReviewApproved": True,
    })

    out = Path(args.output_dir)
    canonical_pack_out = out / "content/visual/first-person/first-person-authored-pack-v1.json"
    staged_pack_out = out / "game/Content/WorldMakers/Visual/first-person-authored-pack-v1.json"
    canonical_activation_out = out / "content/visual/first-person/first-person-native-activation-v1.json"
    staged_activation_out = out / "game/Content/WorldMakers/Visual/first-person-native-activation-v1.json"
    for path in (canonical_pack_out, staged_pack_out):
        write_json(path, activated_pack)
    for path in (canonical_activation_out, staged_activation_out):
        write_json(path, activated_manifest)

    if canonical_pack_out.read_bytes() != staged_pack_out.read_bytes():
        fail("generated pack parity failed")
    if canonical_activation_out.read_bytes() != staged_activation_out.read_bytes():
        fail("generated activation parity failed")
    print(f"Prepared reviewed first-person activation for {target_commit} in {out}")


if __name__ == "__main__":
    main()
