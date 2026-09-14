#!/usr/bin/env python3
"""Fail-closed assessor for M5.6E native epic continuity evidence."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path, PurePosixPath
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
MANIFEST_NAME = "m5-6e-epic-certification-manifest.json"
EXPECTED_FILTER = "WorldMakers.Epic.Persistence."
EXPECTED_TESTS = {
    "WorldMakers.Epic.Persistence.ChapterCheckpointDropsPartialEvidence",
    "WorldMakers.Epic.Persistence.ResumeIndexFailClosed",
    "WorldMakers.Epic.Persistence.NativeSaveGameRoundTrip",
}
EXPECTED_MAP = "game/Content/WorldMakers/Maps/WM_PrototypeCertification.umap"


def read_json(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8-sig") as handle:
        payload = json.load(handle)
    if not isinstance(payload, dict):
        raise ValueError(f"{path.name} must contain a JSON object")
    return payload


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def git_head() -> str:
    result = subprocess.run(
        ["git", "-C", str(ROOT), "rev-parse", "HEAD"],
        check=False,
        capture_output=True,
        text=True,
    )
    return result.stdout.strip() if result.returncode == 0 else ""


def expected_unreal_version() -> str:
    return (ROOT / "game/UNREAL_ENGINE_VERSION").read_text(encoding="utf-8").strip()


def safe_path(evidence_dir: Path, relative: str) -> Path | None:
    candidate = PurePosixPath(relative)
    if candidate.is_absolute() or ".." in candidate.parts or not candidate.parts:
        return None
    root = evidence_dir.resolve()
    resolved = (root / Path(*candidate.parts)).resolve()
    try:
        resolved.relative_to(root)
    except ValueError:
        return None
    return resolved


def assess(evidence_dir: Path, expected_commit: str) -> dict[str, Any]:
    blockers: list[str] = []
    checks = {
        "manifest": False,
        "commit": False,
        "unrealVersion": False,
        "authoredMap": False,
        "nativeStages": False,
        "exactFilter": False,
        "expectedTests": False,
        "saveGameRoundTrip": False,
        "privacyBoundary": False,
        "evidenceIntegrity": False,
    }
    manifest_path = evidence_dir / MANIFEST_NAME
    if not manifest_path.is_file():
        return {
            "schemaVersion": 1,
            "status": "BLOCKED",
            "passed": False,
            "assessedCommit": expected_commit,
            "checks": checks,
            "blockers": [f"missing {MANIFEST_NAME}"],
        }

    try:
        manifest = read_json(manifest_path)
        checks["manifest"] = manifest.get("schemaVersion") == 1
    except (OSError, json.JSONDecodeError, ValueError) as exc:
        return {
            "schemaVersion": 1,
            "status": "BLOCKED",
            "passed": False,
            "assessedCommit": expected_commit,
            "checks": checks,
            "blockers": [f"invalid {MANIFEST_NAME}: {exc}"],
        }

    if not checks["manifest"]:
        blockers.append("unsupported M5.6E manifest schema")

    checks["commit"] = manifest.get("repositoryCommit") == expected_commit
    if not checks["commit"]:
        blockers.append("evidence commit does not match assessed commit")

    version = expected_unreal_version()
    checks["unrealVersion"] = (
        manifest.get("expectedUnrealVersion") == version
        and manifest.get("actualUnrealVersion") == version
    )
    if not checks["unrealVersion"]:
        blockers.append(f"native evidence must use exact Unreal Engine {version}")

    authored = manifest.get("authoredCertificationMap", {})
    checks["authoredMap"] = (
        isinstance(authored, dict)
        and authored.get("path") == EXPECTED_MAP
        and authored.get("present") is True
        and isinstance(authored.get("sha256"), str)
        and len(authored.get("sha256")) == 64
    )
    if not checks["authoredMap"]:
        blockers.append("authored certification map evidence is missing")

    native = manifest.get("nativeChecks", {})
    checks["nativeStages"] = (
        isinstance(native, dict)
        and native.get("runnerPreflight") == "passed"
        and native.get("editorBuild") == "passed"
        and native.get("epicAutomation") == "passed"
        and native.get("exactUnrealVersion") is True
    )
    if not checks["nativeStages"]:
        blockers.append("runner preflight, editor build, or epic automation did not pass")

    checks["exactFilter"] = manifest.get("exactTestFilter") == EXPECTED_FILTER
    if not checks["exactFilter"]:
        blockers.append("automation did not use the exact M5.6E epic persistence filter")

    observed = set(manifest.get("observedTests", []))
    missing = set(manifest.get("missingTests", []))
    checks["expectedTests"] = observed == EXPECTED_TESTS and not missing
    if not checks["expectedTests"]:
        blockers.append("one or more required epic persistence tests were not observed")

    checks["saveGameRoundTrip"] = (
        native.get("saveGameRoundTripObserved") is True
        and "WorldMakers.Epic.Persistence.NativeSaveGameRoundTrip" in observed
    )
    if not checks["saveGameRoundTrip"]:
        blockers.append("native SaveGame round-trip test was not observed")

    privacy = manifest.get("privacyBoundary", {})
    checks["privacyBoundary"] = (
        isinstance(privacy, dict)
        and privacy.get("persistsChapterCheckpointOnly") is True
        and privacy.get("persistsPartialEvidence") is False
        and privacy.get("persistsFreeText") is False
        and privacy.get("persistsMoralChoice") is False
        and privacy.get("persistsPersonalityOrIdeology") is False
    )
    if not checks["privacyBoundary"]:
        blockers.append("privacy-minimized checkpoint boundary is not asserted")

    integrity_ok = True
    for entry in manifest.get("evidenceHashes", []):
        if not isinstance(entry, dict):
            integrity_ok = False
            continue
        relative = entry.get("file")
        expected_hash = entry.get("sha256")
        path = safe_path(evidence_dir, relative) if isinstance(relative, str) else None
        if path is None or not path.is_file() or not isinstance(expected_hash, str) or sha256_file(path) != expected_hash:
            integrity_ok = False
    checks["evidenceIntegrity"] = integrity_ok and bool(manifest.get("evidenceHashes"))
    if not checks["evidenceIntegrity"]:
        blockers.append("evidence hashes are missing, unsafe, or mismatched")

    declared_pass = manifest.get("nativeEpicContinuity", {}).get("passed") is True
    passed = all(checks.values()) and declared_pass and not blockers
    if not declared_pass:
        blockers.append("collector did not declare native epic continuity passed")

    return {
        "schemaVersion": 1,
        "status": "PASSED" if passed else "BLOCKED",
        "passed": passed,
        "assessedCommit": expected_commit,
        "expectedUnrealVersion": version,
        "checks": checks,
        "blockers": blockers,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-dir", default="artifacts/m5-6e-epic")
    parser.add_argument("--expected-commit", default="")
    parser.add_argument("--require-pass", action="store_true")
    args = parser.parse_args()

    evidence_dir = (ROOT / args.evidence_dir).resolve()
    expected_commit = args.expected_commit or git_head()
    result = assess(evidence_dir, expected_commit)
    output = evidence_dir / "m5-6e-epic-assessment.json"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2, sort_keys=True))
    if args.require_pass and not result["passed"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
