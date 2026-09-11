#!/usr/bin/env python3
"""Assess a committed N2 authored visual release candidate.

RELEASE_CANDIDATE is fail-closed: the canonical and packaged P1 registries must
be identical and fully activated, activation provenance must be valid, P5 native
inventory must belong to the exact release commit, and a game-runtime takeover
report must prove that authored environment/avatar/AnimBP/VFX/presentation paths
won over their procedural fallbacks.
"""
from __future__ import annotations

import argparse
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


def assess(canonical_path: Path, packaged_path: Path, provenance_path: Path, inventory_path: Path, runtime_path: Path, expected_release_commit: str) -> dict:
    result = {
        "schemaVersion": 1,
        "phase": "N2",
        "status": "BLOCKED",
        "releaseCandidate": False,
        "releaseCommit": expected_release_commit,
        "checks": {},
        "fingerprint": {},
        "blockers": [],
    }
    if not full_sha(expected_release_commit):
        result["blockers"].append("release commit must be a full lowercase SHA")
        return result

    paths = {
        "canonicalRegistry": canonical_path,
        "packagedRegistry": packaged_path,
        "activationProvenance": provenance_path,
        "nativeInventory": inventory_path,
        "runtimeTakeoverReport": runtime_path,
    }
    for label, path in paths.items():
        if not path.is_file():
            result["blockers"].append(f"missing {label}")
    if result["blockers"]:
        return result

    result["fingerprint"] = {label + "Sha256": sha256_file(path) for label, path in paths.items()}
    canonical = load(canonical_path)
    packaged = load(packaged_path)
    provenance = load(provenance_path)
    inventory = load(inventory_path)
    runtime = load(runtime_path)

    parity = canonical == packaged
    result["checks"]["registryParity"] = parity
    if not parity:
        result["blockers"].append("canonical and packaged P1 registries differ")

    assets = canonical.get("assets", [])
    registry_active = len(assets) == 16 and all(row.get("authoredPresent") is True for row in assets)
    result["checks"]["registryFullyActivated"] = registry_active
    if not registry_active:
        result["blockers"].append("P1 registry is not fully authored-active")

    provenance_ok = (
        provenance.get("phase") == "N2"
        and provenance.get("status") == "ACTIVATION_READY"
        and provenance.get("activationReady") is True
        and provenance.get("activationMode") == "committed"
        and provenance.get("humanApprovalRequired") is True
        and len(provenance.get("candidateSha256", "")) == 64
        and full_sha(provenance.get("n1SourceCommit", ""))
    )
    result["checks"]["activationProvenance"] = provenance_ok
    if not provenance_ok:
        result["blockers"].append("N2 committed activation provenance is invalid")

    collections = (
        inventory.get("registryAssets", []),
        inventory.get("animationAssets", []),
        inventory.get("vfxAssets", []),
        inventory.get("presentationAssets", []),
    )
    inventory_ok = (
        inventory.get("schemaVersion") == 1
        and inventory.get("buildCommit") == expected_release_commit
        and len(collections[0]) == 16
        and len(collections[1]) == 17
        and len(collections[2]) == 17
        and len(collections[3]) == 7
        and all(row.get("exists") is True for collection in collections for row in collection)
    )
    result["checks"]["nativeInventoryExactBuild"] = inventory_ok
    if not inventory_ok:
        result["blockers"].append("P5 native inventory is incomplete or belongs to another commit")

    runtime_required_true = (
        "authoredEnvironmentActive",
        "authoredAvatarActive",
        "authoredAnimationBlueprintActive",
        "authoredVfxReady",
        "authoredPresentationReady",
    )
    runtime_required_false = ("proceduralEnvironmentVisible", "proceduralAvatarActive")
    runtime_ok = (
        runtime.get("schemaVersion") == 1
        and runtime.get("phase") == "N2"
        and runtime.get("buildCommit") == expected_release_commit
        and all(runtime.get(key) is True for key in runtime_required_true)
        and all(runtime.get(key) is False for key in runtime_required_false)
    )
    result["checks"]["runtimeAuthoredTakeover"] = runtime_ok
    if not runtime_ok:
        result["blockers"].append("runtime did not prove full authored visual takeover")

    if result["blockers"]:
        return result

    result["status"] = "RELEASE_CANDIDATE"
    result["releaseCandidate"] = True
    result["p5CertificationRequired"] = True
    result["v8CertificationRequired"] = True
    result["deviceCertificationStillSeparate"] = True
    return result


def self_test() -> int:
    commit = "a" * 40
    with tempfile.TemporaryDirectory(prefix="wm-n2-release-") as temp:
        root = Path(temp)
        registry = {"schemaVersion": 1, "assets": [{"id": f"a{i}", "authoredPresent": True} for i in range(16)]}
        provenance = {
            "schemaVersion": 1, "phase": "N2", "status": "ACTIVATION_READY", "activationReady": True,
            "activationMode": "committed", "humanApprovalRequired": True, "candidateSha256": "b" * 64,
            "n1SourceCommit": "c" * 40,
        }
        inventory = {
            "schemaVersion": 1, "buildCommit": commit,
            "registryAssets": [{"exists": True} for _ in range(16)],
            "animationAssets": [{"exists": True} for _ in range(17)],
            "vfxAssets": [{"exists": True} for _ in range(17)],
            "presentationAssets": [{"exists": True} for _ in range(7)],
        }
        runtime = {
            "schemaVersion": 1, "phase": "N2", "buildCommit": commit,
            "authoredEnvironmentActive": True, "proceduralEnvironmentVisible": False,
            "authoredAvatarActive": True, "proceduralAvatarActive": False,
            "authoredAnimationBlueprintActive": True, "authoredVfxReady": True,
            "authoredPresentationReady": True,
        }
        files = {}
        for name, payload in (("canonical.json", registry), ("packaged.json", registry), ("provenance.json", provenance), ("inventory.json", inventory), ("runtime.json", runtime)):
            path = root / name
            write_json(path, payload)
            files[name] = path

        result = assess(files["canonical.json"], files["packaged.json"], files["provenance.json"], files["inventory.json"], files["runtime.json"], commit)
        if result["status"] != "RELEASE_CANDIDATE":
            raise SystemExit("complete committed authored stack must become RELEASE_CANDIDATE")

        broken = dict(runtime)
        broken["proceduralAvatarActive"] = True
        write_json(files["runtime.json"], broken)
        result = assess(files["canonical.json"], files["packaged.json"], files["provenance.json"], files["inventory.json"], files["runtime.json"], commit)
        if result["status"] != "BLOCKED":
            raise SystemExit("procedural avatar fallback must block RELEASE_CANDIDATE")

        write_json(files["runtime.json"], runtime)
        other_inventory = dict(inventory)
        other_inventory["buildCommit"] = "d" * 40
        write_json(files["inventory.json"], other_inventory)
        result = assess(files["canonical.json"], files["packaged.json"], files["provenance.json"], files["inventory.json"], files["runtime.json"], commit)
        if result["status"] != "BLOCKED":
            raise SystemExit("cross-commit native inventory must block RELEASE_CANDIDATE")

    print("N2 release self-test passed: committed parity, runtime takeover and same-build inventory rules verified.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--canonical-registry", type=Path)
    parser.add_argument("--packaged-registry", type=Path)
    parser.add_argument("--provenance", type=Path)
    parser.add_argument("--native-inventory", type=Path)
    parser.add_argument("--runtime-report", type=Path)
    parser.add_argument("--expected-release-commit")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--require-release-candidate", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    required = (args.canonical_registry, args.packaged_registry, args.provenance, args.native_inventory, args.runtime_report, args.expected_release_commit, args.output)
    if any(value is None for value in required):
        parser.error("registries, provenance, native inventory, runtime report, release commit and output are required")
    result = assess(args.canonical_registry, args.packaged_registry, args.provenance, args.native_inventory, args.runtime_report, args.expected_release_commit)
    write_json(args.output, result)
    print(json.dumps(result, indent=2))
    if args.require_release_candidate and result["status"] != "RELEASE_CANDIDATE":
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
