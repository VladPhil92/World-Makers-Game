#!/usr/bin/env python3
"""Fail-closed Production Alpha readiness assessor."""
from __future__ import annotations

import argparse
import hashlib
import json
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTRACT_PATH = ROOT / "content/production/production-alpha-readiness-v1.json"
TARGET_IDS = {"windows-reference", "android-alpha", "ipados-alpha"}
MOBILE_TARGETS = {"android-alpha", "ipados-alpha"}
REQUIRED_SMOKE = {
    "installLaunch",
    "certificationMapLoad",
    "firstPersonControl",
    "observeScan",
    "scienceInteraction",
    "buildPlace",
    "saveReload",
    "cleanExit",
}


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def full_sha(value: object) -> bool:
    return isinstance(value, str) and len(value) == 40 and all(c in "0123456789abcdef" for c in value)


def add(result: dict, message: str) -> None:
    result["reasons"].append(message)


def validate_g4(path: Path, expected_commit: str, result: dict) -> bool:
    if not path.is_file():
        add(result, f"Missing G4 result: {path}")
        return False
    try:
        data = load(path)
    except Exception as exc:
        add(result, f"Invalid G4 result JSON: {exc}")
        return False
    ok = True
    if data.get("status") != "CERTIFIED" or data.get("certified") is not True:
        add(result, "G4 must be CERTIFIED before Production Alpha readiness can certify")
        ok = False
    commit = data.get("repositoryCommit") or data.get("buildCommit")
    if commit != expected_commit:
        add(result, "G4 result must reference the exact Production Alpha build commit")
        ok = False
    return ok


def validate_manifest(path: Path, target: dict, expected_commit: str, result: dict) -> tuple[bool, dict | None]:
    try:
        data = load(path)
    except Exception as exc:
        add(result, f"{path.name}: invalid build manifest JSON: {exc}")
        return False, None
    ok = True
    target_id = target["id"]
    checks = {
        "schema": data.get("schema") == "worldmakers.alpha-build-manifest.v1",
        "status": data.get("status") == "built",
        "releaseChannel": data.get("releaseChannel") == "internal-alpha",
        "repositoryCommit": data.get("repositoryCommit") == expected_commit,
        "engineVersion": data.get("engineVersion") == "5.8.2",
        "targetId": data.get("targetId") == target_id,
        "unrealPlatform": data.get("unrealPlatform") == target["unrealPlatform"],
        "configuration": data.get("configuration") == target["configuration"],
    }
    for name, passed in checks.items():
        if not passed:
            add(result, f"{path.name}: manifest {name} mismatch")
            ok = False

    build_id = data.get("buildId")
    if not isinstance(build_id, str) or not build_id:
        add(result, f"{path.name}: buildId is required")
        ok = False
    if not data.get("builtAtUtc"):
        add(result, f"{path.name}: builtAtUtc is required")
        ok = False
    host = data.get("buildHost") or {}
    if not host.get("os") or not host.get("machineClass"):
        add(result, f"{path.name}: build host provenance is incomplete")
        ok = False

    source = data.get("source") or {}
    if source.get("originMainCommit") != expected_commit:
        add(result, f"{path.name}: origin/main provenance must match build commit")
        ok = False
    if source.get("cleanWorktree") is not True:
        add(result, f"{path.name}: certified build manifest requires cleanWorktree=true")
        ok = False
    if source.get("certifyingContext") is not True:
        add(result, f"{path.name}: build was not generated in certifying context")
        ok = False

    signing = data.get("signing") or {}
    if target_id in MOBILE_TARGETS:
        if signing.get("status") != "verified" or not signing.get("identity"):
            add(result, f"{path.name}: mobile alpha package requires verified signing identity")
            ok = False
    elif signing.get("status") not in {"verified", "internal-not-required"}:
        add(result, f"{path.name}: Windows reference signing status is not acceptable")
        ok = False

    archive_raw = data.get("archiveRoot")
    archive = Path(archive_raw) if isinstance(archive_raw, str) and archive_raw else None
    if archive is None or not archive.is_dir():
        add(result, f"{path.name}: archiveRoot is missing on the certification machine")
        return False, data
    rows = data.get("artifactFiles")
    if not isinstance(rows, list) or not rows:
        add(result, f"{path.name}: artifactFiles must be non-empty")
        return False, data

    aggregate_lines: list[str] = []
    total = 0
    for row in rows:
        if not isinstance(row, dict):
            add(result, f"{path.name}: invalid artifactFiles entry")
            ok = False
            continue
        rel = row.get("path")
        if not isinstance(rel, str) or not rel or ".." in Path(rel).parts or Path(rel).is_absolute():
            add(result, f"{path.name}: invalid artifact path")
            ok = False
            continue
        actual = (archive / rel).resolve()
        try:
            actual.relative_to(archive.resolve())
        except ValueError:
            add(result, f"{path.name}: artifact path escapes archive root: {rel}")
            ok = False
            continue
        if not actual.is_file():
            add(result, f"{path.name}: missing packaged artifact: {rel}")
            ok = False
            continue
        size = actual.stat().st_size
        digest = sha256_file(actual)
        if row.get("bytes") != size:
            add(result, f"{path.name}: artifact size mismatch: {rel}")
            ok = False
        if row.get("sha256") != digest:
            add(result, f"{path.name}: artifact SHA-256 mismatch: {rel}")
            ok = False
        total += size
        aggregate_lines.append(f"{rel}|{size}|{digest}")

    aggregate = hashlib.sha256((("\n".join(aggregate_lines)) + "\n").encode()).hexdigest()
    if data.get("aggregateArtifactSha256") != aggregate:
        add(result, f"{path.name}: aggregate artifact SHA-256 mismatch")
        ok = False
    if data.get("aggregateArtifactBytes") != total:
        add(result, f"{path.name}: aggregate artifact byte count mismatch")
        ok = False
    return ok, data


def validate_smoke(path: Path, manifest: dict, target_id: str, expected_commit: str, result: dict) -> bool:
    try:
        data = load(path)
    except Exception as exc:
        add(result, f"{path.name}: invalid smoke evidence JSON: {exc}")
        return False
    ok = True
    if data.get("schema") != "worldmakers.alpha-smoke-evidence.v1" or data.get("status") != "passed":
        add(result, f"{path.name}: smoke evidence must use v1 schema and status=passed")
        ok = False
    if data.get("repositoryCommit") != expected_commit:
        add(result, f"{path.name}: smoke evidence commit mismatch")
        ok = False
    if data.get("targetId") != target_id:
        add(result, f"{path.name}: smoke target mismatch")
        ok = False
    if data.get("buildId") != manifest.get("buildId"):
        add(result, f"{path.name}: smoke buildId does not match packaged build")
        ok = False
    if not data.get("device") or not data.get("testedAtUtc") or not data.get("tester"):
        add(result, f"{path.name}: smoke tester/device/time provenance is incomplete")
        ok = False
    checks = data.get("checks") or {}
    for check in REQUIRED_SMOKE:
        if checks.get(check) is not True:
            add(result, f"{path.name}: smoke check failed: {check}")
            ok = False
    if target_id in MOBILE_TARGETS and checks.get("backgroundResume") is not True:
        add(result, f"{path.name}: mobile background/resume smoke check must pass")
        ok = False
    if data.get("crashes") != 0 or data.get("fatalErrors") != 0:
        add(result, f"{path.name}: smoke run reported crash/fatal error")
        ok = False
    issues = data.get("blockingIssues")
    if not isinstance(issues, list) or issues:
        add(result, f"{path.name}: blockingIssues must be an empty list")
        ok = False
    return ok


def validate_ops(path: Path, expected_commit: str, result: dict) -> bool:
    if not path.is_file():
        add(result, f"Missing Alpha operations readiness review: {path}")
        return False
    try:
        data = load(path)
    except Exception as exc:
        add(result, f"Invalid Alpha operations review: {exc}")
        return False
    ok = True
    if data.get("schema") != "worldmakers.alpha-ops-readiness.v1" or data.get("status") != "passed":
        add(result, "Alpha operations review must use v1 schema and status=passed")
        ok = False
    if data.get("repositoryCommit") != expected_commit or not data.get("reviewedAtUtc") or not data.get("reviewer"):
        add(result, "Alpha operations review provenance is incomplete or references another commit")
        ok = False
    crash = data.get("crashTelemetry") or {}
    for key in ("collectorConfigured", "symbolsRetained", "buildCommitCorrelation", "platformVersionCorrelation"):
        if crash.get(key) is not True:
            add(result, f"Alpha crash telemetry requirement not met: {key}")
            ok = False
    for key in ("childPiiCollected", "freeTextChildDataCollected"):
        if crash.get(key) is not False:
            add(result, f"Alpha crash telemetry privacy requirement not met: {key}=false required")
            ok = False
    discipline = data.get("releaseDiscipline") or {}
    for key in ("internalAlphaChannelDefined", "rollbackOrWithdrawalProcedureDefined", "releaseNotesPrepared"):
        if discipline.get(key) is not True:
            add(result, f"Alpha release discipline requirement not met: {key}")
            ok = False
    if discipline.get("knownBlockingIssues") != []:
        add(result, "Alpha release cannot proceed with known blocking issues")
        ok = False
    safety = data.get("playtestSafety") or {}
    if safety.get("authorizedAdultFlowDefined") is not True:
        add(result, "External child testing requires an authorized adult/guardian flow")
        ok = False
    for key in ("realMoneyOrTokenEarningEnabled", "openChatEnabled", "feedbackRequestsChildPii"):
        if safety.get(key) is not False:
            add(result, f"Alpha playtest safety requires {key}=false")
            ok = False
    if int(safety.get("minimumExternalTesterTarget", 0)) < 5:
        add(result, "Alpha external tester target must be at least 5")
        ok = False
    return ok


def assess(root: Path, expected_commit: str, g4_path: Path) -> dict:
    contract = load(CONTRACT_PATH)
    result = {
        "schema": "worldmakers.production-alpha-readiness-result.v1",
        "status": "BLOCKED",
        "certified": False,
        "repositoryCommit": expected_commit,
        "targets": {},
        "g4Certified": False,
        "opsReady": False,
        "reasons": [],
        "nextPhase": None,
    }
    if not full_sha(expected_commit):
        add(result, "Expected commit must be a full lowercase SHA")
        return result

    result["g4Certified"] = validate_g4(g4_path, expected_commit, result)
    build_dir = root / "builds"
    smoke_dir = root / "smoke"
    required_targets = contract["requiredBuildTargets"]
    found_manifest_ids = {p.stem for p in build_dir.glob("*.json")} if build_dir.is_dir() else set()
    found_smoke_ids = {p.stem for p in smoke_dir.glob("*.json")} if smoke_dir.is_dir() else set()
    if found_manifest_ids != TARGET_IDS:
        add(result, "Exactly three canonical build manifests are required: " + ", ".join(sorted(TARGET_IDS)))
    if found_smoke_ids != TARGET_IDS:
        add(result, "Exactly three canonical smoke evidence files are required: " + ", ".join(sorted(TARGET_IDS)))

    all_targets_ok = found_manifest_ids == TARGET_IDS and found_smoke_ids == TARGET_IDS
    for target in required_targets:
        target_id = target["id"]
        manifest_path = build_dir / f"{target_id}.json"
        smoke_path = smoke_dir / f"{target_id}.json"
        target_result = {"manifest": False, "smoke": False}
        if manifest_path.is_file():
            manifest_ok, manifest = validate_manifest(manifest_path, target, expected_commit, result)
            target_result["manifest"] = manifest_ok
            if manifest is not None and smoke_path.is_file():
                target_result["smoke"] = validate_smoke(smoke_path, manifest, target_id, expected_commit, result)
        all_targets_ok = all_targets_ok and target_result["manifest"] and target_result["smoke"]
        result["targets"][target_id] = target_result

    ops_path = root / "alpha-ops-readiness.json"
    result["opsReady"] = validate_ops(ops_path, expected_commit, result)
    certified = result["g4Certified"] and all_targets_ok and result["opsReady"] and not result["reasons"]
    result["certified"] = certified
    result["status"] = "CERTIFIED" if certified else "BLOCKED"
    result["nextPhase"] = contract["exit"]["nextPhase"] if certified else None
    return result


def write_json(path: Path, payload: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def synthetic_manifest(root: Path, target: dict, commit: str) -> dict:
    archive = root / "package" / target["id"]
    archive.mkdir(parents=True, exist_ok=True)
    artifact = archive / "WorldMakers.bin"
    artifact.write_bytes(("world-makers-alpha-" + target["id"]).encode())
    digest = sha256_file(artifact)
    line = f"WorldMakers.bin|{artifact.stat().st_size}|{digest}\n"
    aggregate = hashlib.sha256(line.encode()).hexdigest()
    signing = "verified" if target["id"] in MOBILE_TARGETS else "internal-not-required"
    return {
        "schema": "worldmakers.alpha-build-manifest.v1",
        "status": "built",
        "buildId": f"wm-alpha-{target['id']}",
        "releaseChannel": "internal-alpha",
        "repositoryCommit": commit,
        "engineVersion": "5.8.2",
        "targetId": target["id"],
        "unrealPlatform": target["unrealPlatform"],
        "configuration": target["configuration"],
        "buildHost": {"os": "synthetic", "runner": "self-test", "machineClass": "x64"},
        "builtAtUtc": "2026-01-01T00:00:00Z",
        "archiveRoot": str(archive),
        "artifactFiles": [{"path": "WorldMakers.bin", "bytes": artifact.stat().st_size, "sha256": digest}],
        "aggregateArtifactSha256": aggregate,
        "aggregateArtifactBytes": artifact.stat().st_size,
        "signing": {"requiredForExternalDistribution": target["id"] in MOBILE_TARGETS, "status": signing, "identity": "self-test" if signing == "verified" else ""},
        "source": {"branch": "main", "originMainCommit": commit, "cleanWorktree": True, "certifyingContext": True},
    }


def self_test() -> int:
    contract = load(CONTRACT_PATH)
    commit = "1" * 40
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        g4 = root / "g4.json"
        write_json(g4, {"status": "CERTIFIED", "certified": True, "repositoryCommit": commit})
        for target in contract["requiredBuildTargets"]:
            manifest = synthetic_manifest(root, target, commit)
            write_json(root / "builds" / f"{target['id']}.json", manifest)
            smoke = {
                "schema": "worldmakers.alpha-smoke-evidence.v1",
                "status": "passed",
                "buildId": manifest["buildId"],
                "repositoryCommit": commit,
                "targetId": target["id"],
                "platform": target["unrealPlatform"],
                "device": "synthetic-device",
                "testedAtUtc": "2026-01-01T00:00:00Z",
                "tester": "self-test",
                "checks": {key: True for key in REQUIRED_SMOKE | {"backgroundResume"}},
                "crashes": 0,
                "fatalErrors": 0,
                "blockingIssues": [],
                "evidenceFiles": [],
                "notes": "synthetic self-test only",
            }
            write_json(root / "smoke" / f"{target['id']}.json", smoke)
        ops = {
            "schema": "worldmakers.alpha-ops-readiness.v1",
            "status": "passed",
            "repositoryCommit": commit,
            "reviewedAtUtc": "2026-01-01T00:00:00Z",
            "reviewer": "self-test",
            "crashTelemetry": {"collectorConfigured": True, "symbolsRetained": True, "buildCommitCorrelation": True, "platformVersionCorrelation": True, "childPiiCollected": False, "freeTextChildDataCollected": False},
            "releaseDiscipline": {"internalAlphaChannelDefined": True, "rollbackOrWithdrawalProcedureDefined": True, "knownBlockingIssues": [], "releaseNotesPrepared": True},
            "playtestSafety": {"authorizedAdultFlowDefined": True, "realMoneyOrTokenEarningEnabled": False, "openChatEnabled": False, "feedbackRequestsChildPii": False, "minimumExternalTesterTarget": 5},
        }
        write_json(root / "alpha-ops-readiness.json", ops)
        good = assess(root, commit, g4)
        if good["status"] != "CERTIFIED":
            raise SystemExit("Complete compliant synthetic alpha evidence should certify: " + "; ".join(good["reasons"]))
        artifact = root / "package" / "android-alpha" / "WorldMakers.bin"
        artifact.write_bytes(b"tampered")
        tampered = assess(root, commit, g4)
        if tampered["status"] != "BLOCKED":
            raise SystemExit("Tampered packaged artifact must block Production Alpha certification")
        artifact.write_bytes(b"world-makers-alpha-android-alpha")
        ops["playtestSafety"]["openChatEnabled"] = True
        write_json(root / "alpha-ops-readiness.json", ops)
        unsafe = assess(root, commit, g4)
        if unsafe["status"] != "BLOCKED":
            raise SystemExit("Unsafe open chat must block Production Alpha certification")
    print("Production Alpha assessor self-test passed: complete evidence certifies; artifact tampering and unsafe playtest configuration fail closed.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-root", type=Path)
    parser.add_argument("--expected-commit")
    parser.add_argument("--g4-result", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if not all((args.evidence_root, args.expected_commit, args.g4_result, args.output)):
        parser.error("--evidence-root, --expected-commit, --g4-result and --output are required unless --self-test is used")
    result = assess(args.evidence_root, args.expected_commit, args.g4_result)
    write_json(args.output, result)
    print(json.dumps(result, indent=2))
    return 0 if result["certified"] else 2


if __name__ == "__main__":
    raise SystemExit(main())
