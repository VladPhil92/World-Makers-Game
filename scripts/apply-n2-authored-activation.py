#!/usr/bin/env python3
"""Validate and stage a human-approved N1 authored-asset activation.

This tool never commits or pushes. It verifies that an N1 ACTIVATION_CANDIDATE
is provenance-correct and differs from the fail-closed P1 registry only by
`authoredPresent: false -> true`. It writes an activated registry plus an
activation provenance record for explicit human review.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
import tempfile
from pathlib import Path


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path: Path, payload: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def full_sha(value: str) -> bool:
    return isinstance(value, str) and len(value) == 40 and all(c in "0123456789abcdef" for c in value)


def validate_candidate(base: dict, candidate: dict) -> list[str]:
    blockers: list[str] = []
    if base.get("schemaVersion") != candidate.get("schemaVersion"):
        blockers.append("candidate schemaVersion differs from base registry")

    base_assets = base.get("assets", [])
    candidate_assets = candidate.get("assets", [])
    if len(base_assets) != 16 or len(candidate_assets) != 16:
        blockers.append("base and candidate registries must both contain exactly 16 assets")
        return blockers

    if [row.get("id") for row in base_assets] != [row.get("id") for row in candidate_assets]:
        blockers.append("candidate asset order/IDs differ from base registry")
        return blockers

    for before, after in zip(base_assets, candidate_assets):
        if before.get("authoredPresent") is not False:
            blockers.append(f"base registry is not fail-closed for {before.get('id')}")
        if after.get("authoredPresent") is not True:
            blockers.append(f"candidate does not activate {before.get('id')}")
        before_copy = copy.deepcopy(before)
        after_copy = copy.deepcopy(after)
        before_copy.pop("authoredPresent", None)
        after_copy.pop("authoredPresent", None)
        if before_copy != after_copy:
            blockers.append(f"candidate changed fields other than authoredPresent for {before.get('id')}")

    base_top = copy.deepcopy(base)
    candidate_top = copy.deepcopy(candidate)
    base_top.pop("assets", None)
    candidate_top.pop("assets", None)
    if base_top != candidate_top:
        blockers.append("candidate changed top-level P1 registry metadata")
    return blockers


def assess(
    base_path: Path,
    candidate_path: Path,
    n1_result_path: Path,
    expected_source_commit: str,
    expected_candidate_sha256: str,
    activation_mode: str,
) -> tuple[dict, dict | None]:
    result = {
        "schemaVersion": 1,
        "phase": "N2",
        "status": "BLOCKED",
        "activationReady": False,
        "activationMode": activation_mode,
        "automaticRegistryCommit": False,
        "n1SourceCommit": expected_source_commit,
        "candidateSha256": "",
        "baseRegistrySha256": "",
        "n1ResultSha256": "",
        "blockers": [],
    }

    if activation_mode not in {"rehearsal", "committed"}:
        result["blockers"].append("activation mode must be rehearsal or committed")
        return result, None
    if not full_sha(expected_source_commit):
        result["blockers"].append("expected N1 source commit must be a full lowercase SHA")
        return result, None
    if len(expected_candidate_sha256) != 64 or any(c not in "0123456789abcdef" for c in expected_candidate_sha256):
        result["blockers"].append("expected candidate SHA-256 must be lowercase hexadecimal")
        return result, None

    for label, path in (("base registry", base_path), ("candidate", candidate_path), ("N1 result", n1_result_path)):
        if not path.is_file():
            result["blockers"].append(f"missing {label}")
    if result["blockers"]:
        return result, None

    result["baseRegistrySha256"] = sha256_file(base_path)
    result["candidateSha256"] = sha256_file(candidate_path)
    result["n1ResultSha256"] = sha256_file(n1_result_path)

    if result["candidateSha256"] != expected_candidate_sha256:
        result["blockers"].append("candidate SHA-256 does not match human-approved hash")

    n1 = load(n1_result_path)
    if n1.get("phase") != "N1" or n1.get("status") != "ACTIVATION_CANDIDATE" or n1.get("activationCandidate") is not True:
        result["blockers"].append("N1 result is not an ACTIVATION_CANDIDATE")
    if n1.get("buildCommit") != expected_source_commit:
        result["blockers"].append("N1 result buildCommit does not match expected source commit")
    if n1.get("humanReviewRequiredBeforeApplyingCandidate") is not True:
        result["blockers"].append("N1 result does not preserve human review requirement")

    base = load(base_path)
    candidate = load(candidate_path)
    result["blockers"].extend(validate_candidate(base, candidate))

    if result["blockers"]:
        return result, None

    activated = copy.deepcopy(candidate)
    result["status"] = "ACTIVATION_READY"
    result["activationReady"] = True
    result["assetCount"] = 16
    result["humanApprovalRequired"] = True
    return result, activated


def self_test() -> int:
    base = {
        "schemaVersion": 1,
        "units": "centimeters",
        "assets": [
            {"id": f"asset-{i}", "kind": "StaticMesh", "objectPath": f"/Game/A{i}.A{i}", "authoredPresent": False}
            for i in range(16)
        ],
    }
    candidate = copy.deepcopy(base)
    for row in candidate["assets"]:
        row["authoredPresent"] = True
    n1 = {
        "schemaVersion": 1,
        "phase": "N1",
        "status": "ACTIVATION_CANDIDATE",
        "activationCandidate": True,
        "buildCommit": "1" * 40,
        "humanReviewRequiredBeforeApplyingCandidate": True,
    }

    with tempfile.TemporaryDirectory(prefix="wm-n2-") as temp:
        root = Path(temp)
        base_path = root / "base.json"
        candidate_path = root / "candidate.json"
        n1_path = root / "n1.json"
        write_json(base_path, base)
        write_json(candidate_path, candidate)
        write_json(n1_path, n1)
        candidate_sha = sha256_file(candidate_path)

        result, activated = assess(base_path, candidate_path, n1_path, "1" * 40, candidate_sha, "committed")
        if result["status"] != "ACTIVATION_READY" or activated is None:
            raise SystemExit("valid N1 candidate must become ACTIVATION_READY")

        tampered = copy.deepcopy(candidate)
        tampered["assets"][0]["objectPath"] = "/Game/Tampered.Tampered"
        write_json(candidate_path, tampered)
        tampered_sha = sha256_file(candidate_path)
        result, activated = assess(base_path, candidate_path, n1_path, "1" * 40, tampered_sha, "committed")
        if result["status"] != "BLOCKED" or activated is not None:
            raise SystemExit("candidate field drift must block N2")

        write_json(candidate_path, candidate)
        result, _ = assess(base_path, candidate_path, n1_path, "1" * 40, "0" * 64, "committed")
        if result["status"] != "BLOCKED":
            raise SystemExit("unapproved candidate hash must block N2")

        wrong_n1 = copy.deepcopy(n1)
        wrong_n1["buildCommit"] = "2" * 40
        write_json(n1_path, wrong_n1)
        result, _ = assess(base_path, candidate_path, n1_path, "1" * 40, candidate_sha, "committed")
        if result["status"] != "BLOCKED":
            raise SystemExit("cross-commit N1 evidence must block N2")

    print("N2 activation self-test passed: approved hash, field-preservation and same-source-commit rules verified.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--base-registry", type=Path)
    parser.add_argument("--candidate", type=Path)
    parser.add_argument("--n1-result", type=Path)
    parser.add_argument("--expected-source-commit")
    parser.add_argument("--expected-candidate-sha256")
    parser.add_argument("--mode", choices=("rehearsal", "committed"), default="rehearsal")
    parser.add_argument("--output-registry", type=Path)
    parser.add_argument("--output-provenance", type=Path)
    parser.add_argument("--require-ready", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    if args.self_test:
        return self_test()

    required = (
        args.base_registry,
        args.candidate,
        args.n1_result,
        args.expected_source_commit,
        args.expected_candidate_sha256,
        args.output_registry,
        args.output_provenance,
    )
    if any(value is None for value in required):
        parser.error("base registry, candidate, N1 result, expected hashes/commit and outputs are required")

    result, activated = assess(
        args.base_registry,
        args.candidate,
        args.n1_result,
        args.expected_source_commit,
        args.expected_candidate_sha256,
        args.mode,
    )
    write_json(args.output_provenance, result)
    if activated is not None:
        write_json(args.output_registry, activated)
    elif args.output_registry.exists():
        args.output_registry.unlink()

    print(json.dumps(result, indent=2))
    if args.require_ready and result["status"] != "ACTIVATION_READY":
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
