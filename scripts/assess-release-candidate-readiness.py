#!/usr/bin/env python3
"""Fail-closed Release Candidate readiness assessor for World Makers."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTRACT_PATH = ROOT / "content/production/release-candidate-readiness-v1.json"
TARGETS = {"windows-reference", "android-alpha", "ipados-alpha"}
MOBILE = {"android-alpha", "ipados-alpha"}
RC_RE = re.compile(r"^\d+\.\d+\.\d+-rc\.\d+$")
HEX64_RE = re.compile(r"^[0-9a-f]{64}$")


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def write(path: Path, payload: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def full_sha(value: object) -> bool:
    return isinstance(value, str) and len(value) == 40 and all(c in "0123456789abcdef" for c in value)


def hex64(value: object) -> bool:
    return isinstance(value, str) and HEX64_RE.fullmatch(value) is not None


def safe_payload(root: Path, rel: object) -> Path | None:
    if not isinstance(rel, str) or not rel or Path(rel).is_absolute() or ".." in Path(rel).parts:
        return None
    candidate = (root / rel).resolve()
    resolved = root.resolve()
    if candidate != resolved and resolved not in candidate.parents:
        return None
    return candidate


def add(result: dict, message: str) -> None:
    result["reasons"].append(message)


def validate_external_alpha(path: Path, commit: str, result: dict) -> bool:
    if not path.is_file():
        add(result, "External Alpha canonical result is missing")
        return False
    try:
        data = load(path)
    except Exception as exc:
        add(result, f"External Alpha result is invalid JSON: {exc}")
        return False
    ok = True
    if data.get("status") != "CERTIFIED" or data.get("certified") is not True:
        add(result, "External Alpha must be CERTIFIED before Release Candidate readiness can certify")
        ok = False
    if data.get("repositoryCommit") != commit:
        add(result, "External Alpha result must reference the exact RC build commit")
        ok = False
    result["externalAlphaCertifiedSameCommit"] = ok
    return ok


def validate_freeze(path: Path, commit: str, result: dict) -> tuple[bool, dict | None]:
    if not path.is_file():
        add(result, "RC freeze manifest is missing")
        return False, None
    try:
        data = load(path)
    except Exception as exc:
        add(result, f"RC freeze manifest is invalid JSON: {exc}")
        return False, None
    ok = True
    version = data.get("releaseVersion")
    checks = {
        "schema": data.get("schema") == "worldmakers.rc-freeze-manifest.v1",
        "status": data.get("status") == "frozen",
        "releaseVersion": isinstance(version, str) and RC_RE.fullmatch(version or "") is not None,
        "repositoryCommit": data.get("repositoryCommit") == commit,
        "engineVersion": data.get("engineVersion") == "5.8.2",
    }
    for name, passed in checks.items():
        if not passed:
            add(result, f"Freeze manifest {name} check failed")
            ok = False
    source = data.get("source") or {}
    if source.get("branch") != "main" or source.get("originMainCommit") != commit or source.get("cleanWorktree") is not True:
        add(result, "RC freeze must be captured from clean main synchronized with origin/main")
        ok = False
    freeze = data.get("freeze") or {}
    for key in ("contentFrozen", "dependenciesFrozen", "configurationFrozen"):
        if freeze.get(key) is not True:
            add(result, f"RC freeze requires {key}=true")
            ok = False
    if not freeze.get("frozenAtUtc") or not freeze.get("frozenBy"):
        add(result, "RC freeze provenance is incomplete")
        ok = False

    rows = data.get("targets") or []
    ids = [row.get("targetId") for row in rows if isinstance(row, dict)]
    if len(rows) != 3 or set(ids) != TARGETS or len(set(ids)) != 3:
        add(result, "Freeze manifest requires exactly three canonical targets")
        ok = False
    target_map: dict[str, dict] = {}
    for row in rows:
        if not isinstance(row, dict) or row.get("targetId") not in TARGETS:
            continue
        tid = row["targetId"]
        target_map[tid] = row
        if not hex64(row.get("normalizedPayloadSha256")) or not hex64(row.get("signedArtifactSha256")):
            add(result, f"{tid}: normalized and signed artifact SHA-256 values are required")
            ok = False
        sig = row.get("signatureStatus")
        if tid in MOBILE and sig != "verified":
            add(result, f"{tid}: mobile distribution signature must be verified")
            ok = False
        if tid == "windows-reference" and sig not in {"verified", "internal-not-required"}:
            add(result, "windows-reference: signature status is invalid")
            ok = False
    data["_targetMap"] = target_map
    result["releaseVersion"] = version if isinstance(version, str) else None
    return ok, data


def validate_repro(path: Path, commit: str, freeze: dict | None, result: dict) -> bool:
    if freeze is None or not path.is_file():
        add(result, "RC reproducibility evidence is missing")
        return False
    try:
        data = load(path)
    except Exception as exc:
        add(result, f"RC reproducibility evidence is invalid JSON: {exc}")
        return False
    ok = True
    if data.get("schema") != "worldmakers.rc-reproducibility.v1" or data.get("status") != "passed":
        add(result, "RC reproducibility evidence must use v1 schema and status=passed")
        ok = False
    if data.get("releaseVersion") != freeze.get("releaseVersion") or data.get("repositoryCommit") != commit or data.get("engineVersion") != "5.8.2":
        add(result, "RC reproducibility provenance does not match frozen candidate")
        ok = False
    rows = data.get("targets") or []
    ids = [row.get("targetId") for row in rows if isinstance(row, dict)]
    if len(rows) != 3 or set(ids) != TARGETS or len(set(ids)) != 3:
        add(result, "RC reproducibility requires exactly three canonical targets")
        ok = False
    frozen = freeze.get("_targetMap", {})
    for row in rows:
        if not isinstance(row, dict) or row.get("targetId") not in TARGETS:
            continue
        tid = row["targetId"]
        canonical = row.get("canonicalPayloadSha256")
        rebuilt = row.get("independentRebuildPayloadSha256")
        if not hex64(canonical) or not hex64(rebuilt) or canonical != rebuilt:
            add(result, f"{tid}: independent rebuild payload digest does not reproduce canonical payload")
            ok = False
        if canonical != (frozen.get(tid) or {}).get("normalizedPayloadSha256"):
            add(result, f"{tid}: reproducibility digest does not match freeze manifest")
            ok = False
        if not row.get("rebuildHost") or not row.get("verifiedAtUtc"):
            add(result, f"{tid}: independent rebuild provenance is incomplete")
            ok = False
    return ok


def validate_regression(path: Path, commit: str, version: str | None, result: dict) -> bool:
    if not path.is_file():
        add(result, "RC regression evidence is missing")
        return False
    try:
        data = load(path)
    except Exception as exc:
        add(result, f"RC regression evidence is invalid JSON: {exc}")
        return False
    ok = True
    if data.get("schema") != "worldmakers.rc-regression.v1" or data.get("status") != "passed":
        add(result, "RC regression evidence must use v1 schema and status=passed")
        ok = False
    if data.get("repositoryCommit") != commit or data.get("releaseVersion") != version or not data.get("testedAtUtc"):
        add(result, "RC regression provenance is incomplete or references another candidate")
        ok = False
    rows = data.get("targets") or []
    ids = [row.get("targetId") for row in rows if isinstance(row, dict)]
    if len(rows) != 3 or set(ids) != TARGETS or len(set(ids)) != 3:
        add(result, "RC regression requires exactly three canonical targets")
        ok = False
    for row in rows:
        if not isinstance(row, dict) or row.get("targetId") not in TARGETS:
            continue
        tid = row["targetId"]
        for key in ("freshInstallPassed", "upgradeInstallPassed", "criticalJourneyPassed", "saveUpgradeCompatibilityPassed", "offlineRecoveryPassed"):
            if row.get(key) is not True:
                add(result, f"{tid}: regression requirement failed: {key}")
                ok = False
        if tid in MOBILE and row.get("backgroundResumePassed") is not True:
            add(result, f"{tid}: mobile background/resume regression must pass")
            ok = False
        for key in ("crashes", "fatalErrors", "dataLossEvents"):
            if row.get(key) != 0:
                add(result, f"{tid}: {key} must be zero")
                ok = False
    issues = data.get("openIssues") or {}
    if issues.get("P0") != 0 or issues.get("P1") != 0:
        add(result, "RC regression cannot pass with open P0 or P1 issues")
        ok = False
    return ok


def verify_hashed_file(root: Path, rel: object, expected: object, label: str, result: dict) -> bool:
    path = safe_payload(root, rel)
    if path is None or not path.is_file():
        add(result, f"{label} file is missing or unsafe")
        return False
    if not hex64(expected) or sha256_file(path) != expected:
        add(result, f"{label} SHA-256 mismatch")
        return False
    return True


def validate_operations(path: Path, root: Path, commit: str, version: str | None, contract: dict, result: dict) -> bool:
    if not path.is_file():
        add(result, "RC operations evidence is missing")
        return False
    try:
        data = load(path)
    except Exception as exc:
        add(result, f"RC operations evidence is invalid JSON: {exc}")
        return False
    ok = True
    if data.get("schema") != "worldmakers.rc-operations.v1" or data.get("status") != "passed":
        add(result, "RC operations evidence must use v1 schema and status=passed")
        ok = False
    if data.get("repositoryCommit") != commit or data.get("releaseVersion") != version or not data.get("reviewedAtUtc") or not data.get("reviewer"):
        add(result, "RC operations provenance is incomplete or references another candidate")
        ok = False

    sbom = data.get("sbom") or {}
    allowed_formats = set(contract["supplyChain"]["allowedSbomFormats"])
    if sbom.get("format") not in allowed_formats:
        add(result, "RC SBOM format is not allowed")
        ok = False
    if not verify_hashed_file(root, sbom.get("file"), sbom.get("sha256"), "SBOM", result):
        ok = False
    else:
        try:
            sbom_doc = load(safe_payload(root, sbom.get("file")))
            if sbom.get("format") == "SPDX-2.3" and sbom_doc.get("spdxVersion") != "SPDX-2.3":
                add(result, "SBOM claims SPDX-2.3 but document spdxVersion differs")
                ok = False
            if sbom.get("format") == "CycloneDX-1.6" and not (sbom_doc.get("bomFormat") == "CycloneDX" and str(sbom_doc.get("specVersion")) == "1.6"):
                add(result, "SBOM claims CycloneDX-1.6 but document metadata differs")
                ok = False
        except Exception as exc:
            add(result, f"SBOM document is invalid JSON: {exc}")
            ok = False
    if sbom.get("dependencyProvenanceVerified") is not True:
        add(result, "Dependency provenance must be verified")
        ok = False
    if sbom.get("criticalVulnerabilities") != 0 or sbom.get("highVulnerabilities") != 0:
        add(result, "Known critical/high vulnerabilities must be zero for RC promotion")
        ok = False

    diag = data.get("crashDiagnostics") or {}
    for key in ("symbolsRetained", "symbolsUploaded", "productionTelemetryRouteVerified"):
        if diag.get(key) is not True:
            add(result, f"RC crash diagnostics requirement not met: {key}")
            ok = False
    signing = data.get("signing") or {}
    if signing.get("androidVerified") is not True or signing.get("ipadosVerified") is not True:
        add(result, "Android and iPadOS distribution signing must be verified")
        ok = False

    discipline = data.get("releaseDiscipline") or {}
    for key in ("releaseNotesPrepared", "rollbackPackagePrepared", "rollbackProcedureVerified"):
        if discipline.get(key) is not True:
            add(result, f"RC release discipline requirement not met: {key}")
            ok = False
    if not verify_hashed_file(root, discipline.get("releaseNotesFile"), discipline.get("releaseNotesSha256"), "Release notes", result):
        ok = False
    if not verify_hashed_file(root, discipline.get("rollbackPackageFile"), discipline.get("rollbackPackageSha256"), "Rollback package", result):
        ok = False

    safety = data.get("safety") or {}
    if safety.get("childPiiCollected") is not False or safety.get("openChatEnabled") is not False or safety.get("realMoneyOrTokenEarningEnabled") is not False:
        add(result, "RC safety boundary failed: child PII, open chat and real-money/token earning must remain disabled")
        ok = False
    return ok


def validate_decision(path: Path, commit: str, version: str | None, result: dict) -> bool:
    if not path.is_file():
        add(result, "RC release decision is missing")
        return False
    try:
        data = load(path)
    except Exception as exc:
        add(result, f"RC release decision is invalid JSON: {exc}")
        return False
    ok = True
    if data.get("schema") != "worldmakers.rc-release-decision.v1" or data.get("status") != "approved" or data.get("decision") != "PROMOTE":
        add(result, "RC human decision must be status=approved and decision=PROMOTE")
        ok = False
    if data.get("repositoryCommit") != commit or data.get("releaseVersion") != version or not data.get("decidedAtUtc"):
        add(result, "RC release decision provenance is incomplete or references another candidate")
        ok = False
    approvals = data.get("approvals") or {}
    reviewers = data.get("reviewers") or {}
    for role in ("qa", "engineering", "childSafety", "product", "securitySupplyChain"):
        if approvals.get(role) is not True or not reviewers.get(role):
            add(result, f"RC promotion requires explicit {role} approval and reviewer")
            ok = False
    if data.get("knownBlockingIssues") != []:
        add(result, "RC promotion decision cannot contain known blocking issues")
        ok = False
    return ok


def assess(root: Path, external_alpha_result: Path, commit: str) -> dict:
    contract = load(CONTRACT_PATH)
    result = {
        "schema": "worldmakers.release-candidate-readiness-result.v1",
        "status": "BLOCKED",
        "certified": False,
        "repositoryCommit": commit,
        "releaseVersion": None,
        "externalAlphaCertifiedSameCommit": False,
        "checks": {"freeze": False, "reproducibility": False, "regression": False, "operations": False, "decision": False},
        "reasons": [],
        "nextPhase": None,
    }
    if not full_sha(commit):
        add(result, "Expected commit must be a full lowercase SHA")
        return result

    ext_ok = validate_external_alpha(external_alpha_result, commit, result)
    freeze_ok, freeze = validate_freeze(root / "freeze-manifest.json", commit, result)
    result["checks"]["freeze"] = freeze_ok
    version = freeze.get("releaseVersion") if freeze else None
    result["releaseVersion"] = version
    repro_ok = validate_repro(root / "reproducibility.json", commit, freeze, result)
    regression_ok = validate_regression(root / "regression.json", commit, version, result)
    ops_ok = validate_operations(root / "operations.json", root, commit, version, contract, result)
    decision_ok = validate_decision(root / "release-decision.json", commit, version, result)
    result["checks"].update({"reproducibility": repro_ok, "regression": regression_ok, "operations": ops_ok, "decision": decision_ok})
    certified = ext_ok and freeze_ok and repro_ok and regression_ok and ops_ok and decision_ok and not result["reasons"]
    result["certified"] = certified
    result["status"] = "CERTIFIED" if certified else "BLOCKED"
    result["nextPhase"] = contract["exit"]["nextPhase"] if certified else None
    return result


def self_test() -> int:
    commit = "a" * 40
    version = "0.1.0-rc.1"
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        external = root / "external-alpha.json"
        write(external, {"status": "CERTIFIED", "certified": True, "repositoryCommit": commit})
        payloads = {tid: hashlib.sha256(tid.encode()).hexdigest() for tid in TARGETS}
        signed = {tid: hashlib.sha256((tid + "-signed").encode()).hexdigest() for tid in TARGETS}
        freeze = {
            "schema": "worldmakers.rc-freeze-manifest.v1", "status": "frozen", "releaseVersion": version,
            "repositoryCommit": commit, "engineVersion": "5.8.2",
            "source": {"branch": "main", "originMainCommit": commit, "cleanWorktree": True},
            "freeze": {"contentFrozen": True, "dependenciesFrozen": True, "configurationFrozen": True, "frozenAtUtc": "2026-01-01T00:00:00Z", "frozenBy": "self-test"},
            "targets": [
                {"targetId": tid, "normalizedPayloadSha256": payloads[tid], "signedArtifactSha256": signed[tid], "signatureStatus": "verified" if tid in MOBILE else "internal-not-required"}
                for tid in sorted(TARGETS)
            ],
        }
        write(root / "freeze-manifest.json", freeze)
        write(root / "reproducibility.json", {
            "schema": "worldmakers.rc-reproducibility.v1", "status": "passed", "releaseVersion": version,
            "repositoryCommit": commit, "engineVersion": "5.8.2",
            "targets": [{"targetId": tid, "canonicalPayloadSha256": payloads[tid], "independentRebuildPayloadSha256": payloads[tid], "rebuildHost": "independent-self-test", "verifiedAtUtc": "2026-01-01T01:00:00Z"} for tid in sorted(TARGETS)],
        })
        rows = []
        for tid in sorted(TARGETS):
            rows.append({"targetId": tid, "freshInstallPassed": True, "upgradeInstallPassed": True, "criticalJourneyPassed": True, "saveUpgradeCompatibilityPassed": True, "offlineRecoveryPassed": True, "backgroundResumePassed": True, "crashes": 0, "fatalErrors": 0, "dataLossEvents": 0})
        write(root / "regression.json", {"schema": "worldmakers.rc-regression.v1", "status": "passed", "releaseVersion": version, "repositoryCommit": commit, "testedAtUtc": "2026-01-01T02:00:00Z", "targets": rows, "openIssues": {"P0": 0, "P1": 0, "P2": 1, "P3": 2}})
        sbom = root / "payloads" / "sbom.spdx.json"
        write(sbom, {"spdxVersion": "SPDX-2.3", "name": "World Makers self-test"})
        notes = root / "payloads" / "release-notes.md"
        notes.write_text("World Makers RC self-test\n", encoding="utf-8")
        rollback = root / "payloads" / "rollback.bin"
        rollback.write_bytes(b"world-makers-rollback")
        write(root / "operations.json", {
            "schema": "worldmakers.rc-operations.v1", "status": "passed", "releaseVersion": version, "repositoryCommit": commit,
            "reviewedAtUtc": "2026-01-01T03:00:00Z", "reviewer": "self-test",
            "sbom": {"format": "SPDX-2.3", "file": "payloads/sbom.spdx.json", "sha256": sha256_file(sbom), "dependencyProvenanceVerified": True, "criticalVulnerabilities": 0, "highVulnerabilities": 0},
            "crashDiagnostics": {"symbolsRetained": True, "symbolsUploaded": True, "productionTelemetryRouteVerified": True},
            "signing": {"androidVerified": True, "ipadosVerified": True},
            "releaseDiscipline": {"releaseNotesPrepared": True, "releaseNotesFile": "payloads/release-notes.md", "releaseNotesSha256": sha256_file(notes), "rollbackPackagePrepared": True, "rollbackPackageFile": "payloads/rollback.bin", "rollbackPackageSha256": sha256_file(rollback), "rollbackProcedureVerified": True},
            "safety": {"childPiiCollected": False, "openChatEnabled": False, "realMoneyOrTokenEarningEnabled": False},
        })
        write(root / "release-decision.json", {
            "schema": "worldmakers.rc-release-decision.v1", "status": "approved", "releaseVersion": version, "repositoryCommit": commit,
            "decision": "PROMOTE", "decidedAtUtc": "2026-01-01T04:00:00Z",
            "approvals": {"qa": True, "engineering": True, "childSafety": True, "product": True, "securitySupplyChain": True},
            "reviewers": {"qa": "qa", "engineering": "eng", "childSafety": "safety", "product": "product", "securitySupplyChain": "security"},
            "knownBlockingIssues": [], "notes": "self-test",
        })
        good = assess(root, external, commit)
        if good["status"] != "CERTIFIED":
            print(json.dumps(good, indent=2))
            raise SystemExit("Self-test failed: complete RC evidence did not certify")

        repro = load(root / "reproducibility.json")
        repro["targets"][0]["independentRebuildPayloadSha256"] = "0" * 64
        write(root / "reproducibility.json", repro)
        if assess(root, external, commit)["status"] != "BLOCKED":
            raise SystemExit("Self-test failed: non-reproducible payload did not block")
        repro["targets"][0]["independentRebuildPayloadSha256"] = repro["targets"][0]["canonicalPayloadSha256"]
        write(root / "reproducibility.json", repro)

        regression = load(root / "regression.json")
        regression["targets"][0]["saveUpgradeCompatibilityPassed"] = False
        write(root / "regression.json", regression)
        if assess(root, external, commit)["status"] != "BLOCKED":
            raise SystemExit("Self-test failed: save upgrade incompatibility did not block")
        regression["targets"][0]["saveUpgradeCompatibilityPassed"] = True
        write(root / "regression.json", regression)

        ops = load(root / "operations.json")
        ops["safety"]["openChatEnabled"] = True
        write(root / "operations.json", ops)
        if assess(root, external, commit)["status"] != "BLOCKED":
            raise SystemExit("Self-test failed: unsafe open chat did not block")

    print("Release Candidate assessor self-test passed: valid evidence certifies; reproducibility drift, save incompatibility and unsafe chat fail closed.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-root", type=Path)
    parser.add_argument("--external-alpha-result", type=Path)
    parser.add_argument("--expected-commit")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if not all((args.evidence_root, args.external_alpha_result, args.expected_commit, args.output)):
        parser.error("--evidence-root, --external-alpha-result, --expected-commit and --output are required")
    result = assess(args.evidence_root, args.external_alpha_result, args.expected_commit)
    write(args.output, result)
    print(json.dumps(result, indent=2))
    return 0 if result["status"] == "CERTIFIED" else 1


if __name__ == "__main__":
    raise SystemExit(main())
