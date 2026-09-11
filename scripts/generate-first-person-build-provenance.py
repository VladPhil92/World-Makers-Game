#!/usr/bin/env python3
"""Generate non-versioned first-person build provenance for packaging.

The activation manifest identifies the source commit that received native review. This script
binds that immutable review identity to the actual activation/build commit after checkout,
avoiding a self-referential Git SHA inside a committed manifest.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path

SHA1_RE = re.compile(r"^[0-9a-f]{40}$")
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")


def fail(message: str) -> None:
    raise SystemExit("First-person build provenance generation failed: " + message)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-commit", required=True)
    parser.add_argument("--activation-manifest", default="game/Content/WorldMakers/Visual/first-person-native-activation-v1.json")
    parser.add_argument("--output", default="game/Content/WorldMakers/Visual/first-person-build-provenance-v1.json")
    args = parser.parse_args()

    build_commit = args.build_commit.strip().lower()
    if not SHA1_RE.fullmatch(build_commit):
        fail("--build-commit must be a lowercase 40-character Git SHA")

    activation_path = Path(args.activation_manifest)
    if not activation_path.is_file():
        fail(f"activation manifest not found: {activation_path}")
    activation_bytes = activation_path.read_bytes()
    activation = json.loads(activation_bytes)
    if activation.get("schemaVersion") != 1 or activation.get("activationId") != "visual.first-person-native-activation.v1":
        fail("activation manifest identity mismatch")
    if activation.get("status") != "activated" or activation.get("activated") is not True:
        fail("activation manifest is not activated")

    reviewed = str(activation.get("targetCommitSha", ""))
    candidate_sha = str(activation.get("activationCandidateSha256", ""))
    if not SHA1_RE.fullmatch(reviewed):
        fail("activation targetCommitSha is invalid")
    if not SHA256_RE.fullmatch(candidate_sha):
        fail("activationCandidateSha256 is invalid")

    payload = {
        "schemaVersion": 1,
        "provenanceId": "visual.first-person-build-provenance.v1",
        "status": "bound",
        "buildCommitSha": build_commit,
        "reviewedSourceCommitSha": reviewed,
        "activationCandidateSha256": candidate_sha,
        "activationManifestSha256": hashlib.sha256(activation_bytes).hexdigest(),
    }

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"Bound first-person activation reviewed at {reviewed} to build {build_commit}")


if __name__ == "__main__":
    main()
