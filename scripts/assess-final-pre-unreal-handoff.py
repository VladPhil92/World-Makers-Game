#!/usr/bin/env python3
"""Assess the final World Makers pre-Unreal handoff gate.

This assessor never performs network authentication and never consumes secret values.
It evaluates sanitized external evidence produced by an authorized operator/probe.
"""
from __future__ import annotations

import argparse
import copy
import json
from datetime import datetime
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
CONTRACT_PATH = ROOT / "content/production/final-pre-unreal-handoff-v1.json"
DEFAULT_EVIDENCE = ROOT / "artifacts/pre-unreal-handoff/cloud-e2e.json"
DEFAULT_OUTPUT = ROOT / "artifacts/pre-unreal-handoff/final-pre-unreal-handoff.json"


class GateError(RuntimeError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise GateError(message)


def load_json(path: Path) -> dict[str, Any]:
    require(path.is_file(), f"missing evidence: {path}")
    with path.open("r", encoding="utf-8") as handle:
        value = json.load(handle)
    require(isinstance(value, dict), f"expected JSON object: {path}")
    return value


def validate_contract(contract: dict[str, Any]) -> None:
    require(contract.get("schema") == "worldmakers.final-pre-unreal-handoff.v1", "final handoff contract schema mismatch")
    require(contract.get("status") == "audit-gate-active-external-evidence-required", "final handoff contract status mismatch")
    certification = contract.get("certification", {})
    require(certification.get("officialState") == "PRE_UNREAL_READY", "official handoff state mismatch")
    require(certification.get("sourceCiMayCertifyOfficialState") is False, "source CI must not self-certify PRE_UNREAL_READY")
    require(certification.get("requiresCleanCurrentMain") is True, "clean-main requirement missing")
    require(certification.get("requiresSameRepositoryCommit") is True, "same-commit requirement missing")
    require(contract.get("exitCriteria", {}).get("nextPhase") == "native-unreal-materialization", "handoff next phase mismatch")


def validate_external_evidence(contract: dict[str, Any], evidence: dict[str, Any], expected_commit: str) -> list[str]:
    errors: list[str] = []

    def check(condition: bool, message: str) -> None:
        if not condition:
            errors.append(message)

    check(evidence.get("schema") == "worldmakers.pre-unreal-cloud-e2e-evidence.v1", "external evidence schema mismatch")
    check(evidence.get("status") == "passed", "external evidence status must be passed")
    check(bool(expected_commit), "expected repository commit is required")
    check(evidence.get("repositoryCommit") == expected_commit, "external evidence repository commit mismatch")
    check(bool(evidence.get("observedAtUtc")), "external evidence observation timestamp missing")
    check(bool(evidence.get("observer")), "external evidence observer missing")

    railway_contract = contract.get("requiredExternalEvidence", {}).get("railway", {})
    railway = evidence.get("railway", {})
    check(railway.get("projectName") == railway_contract.get("projectName"), "Railway project mismatch")
    check(railway.get("environment") == railway_contract.get("environment"), "Railway environment mismatch")
    services = railway.get("services", [])
    service_map = {item.get("name"): item for item in services if isinstance(item, dict)}
    required_services = railway_contract.get("requiredServices", [])
    check(set(service_map) == set(required_services), "Railway service inventory mismatch")
    for name in required_services:
        check(service_map.get(name, {}).get("deploymentStatus") == railway_contract.get("requiredDeploymentStatus"), f"Railway service not healthy: {name}")

    required_domains = contract.get("requiredExternalEvidence", {}).get("customDomains", [])
    domain_map = {(item.get("service"), item.get("host")): item for item in evidence.get("domains", []) if isinstance(item, dict)}
    check(len(domain_map) == len(required_domains), "custom domain evidence inventory mismatch")
    for required_domain in required_domains:
        key = (required_domain.get("service"), required_domain.get("host"))
        row = domain_map.get(key, {})
        check(bool(row), f"missing custom domain evidence: {required_domain.get('host')}")
        check(row.get("path") == required_domain.get("requiredPath"), f"custom domain path mismatch: {required_domain.get('host')}")
        check(row.get("dnsResolved") is True, f"DNS not resolved: {required_domain.get('host')}")
        check(row.get("httpsHealthy") is True, f"HTTPS health failed: {required_domain.get('host')}")
        status_code = row.get("statusCode")
        check(isinstance(status_code, int) and 200 <= status_code < 300, f"non-2xx health response: {required_domain.get('host')}")

    e2e = evidence.get("ctgOneE2E", {})
    check(e2e.get("realBearerUsed") is True, "real CTG One bearer was not used")
    check(e2e.get("bearerPersisted") is False, "bearer token must not be persisted")
    check(e2e.get("getPlayerStatePassed") is True, "player-state GET did not pass")
    check(isinstance(e2e.get("initialRevision"), int) and e2e.get("initialRevision") >= 0, "initial player-state revision missing")
    check(e2e.get("putPlayerStatePassed") is True, "player-state PUT did not pass")
    check(isinstance(e2e.get("writeRevision"), int) and e2e.get("writeRevision") >= 0, "write revision missing")
    check(e2e.get("idempotentRetryPassed") is True, "idempotent retry did not pass")
    check(e2e.get("retryRevision") == e2e.get("writeRevision"), "idempotent retry changed revision")
    check(e2e.get("revisionConflictPassed") is True, "stale expectedRevision did not produce a conflict")
    check(e2e.get("supabasePersistenceObserved") is True, "Supabase persistence was not observed")
    check(e2e.get("temporaryProbeDataCleaned") is True, "temporary E2E probe data was not cleaned")

    security = evidence.get("security", {})
    check(security.get("secretValuesRecorded") is False, "secret values recorded in evidence")
    check(security.get("bearerValueRecorded") is False, "bearer value recorded in evidence")
    check(security.get("childPiiRecorded") is False, "child PII recorded in evidence")
    check(security.get("openChatEnabled") is False, "open chat enabled")
    check(security.get("userVoiceCaptureEnabled") is False, "user voice capture enabled")
    check(security.get("runtimeClientContainsPrivilegedCredential") is False, "privileged runtime client credential detected")

    serialized = json.dumps(evidence, sort_keys=True).lower()
    forbidden_markers = [
        "service_role_key",
        "database_password",
        "bridge_hmac_secret",
        "authorization: bearer",
        "\"bearer\":",
        "\"token\":",
    ]
    for marker in forbidden_markers:
        check(marker not in serialized, f"forbidden secret marker present in evidence: {marker}")

    return errors


def build_result(expected_commit: str, errors: list[str], certifying_context: bool) -> dict[str, Any]:
    evidence_passes = not errors
    if evidence_passes and certifying_context:
        state = "PRE_UNREAL_READY"
    elif evidence_passes:
        state = "NON_CERTIFYING_PASS"
    else:
        state = "BLOCKED"
    return {
        "schema": "worldmakers.final-pre-unreal-handoff-result.v1",
        "status": state,
        "preUnrealReady": state == "PRE_UNREAL_READY",
        "repositoryCommit": expected_commit,
        "certifyingContext": certifying_context,
        "externalEvidencePasses": evidence_passes,
        "blockingReasons": errors,
        "assessedAtUtc": datetime.utcnow().replace(microsecond=0).isoformat() + "Z",
        "nextPhase": "native-unreal-materialization" if state == "PRE_UNREAL_READY" else None,
        "truthBoundary": "PRE_UNREAL_READY closes pre-native source/cloud handoff only; it is not native Unreal certification."
    }


def assess(contract: dict[str, Any], evidence: dict[str, Any], expected_commit: str, certifying_context: bool) -> dict[str, Any]:
    validate_contract(contract)
    errors = validate_external_evidence(contract, evidence, expected_commit)
    return build_result(expected_commit, errors, certifying_context)


def compliant_fixture(commit: str) -> dict[str, Any]:
    return {
        "schema": "worldmakers.pre-unreal-cloud-e2e-evidence.v1",
        "status": "passed",
        "repositoryCommit": commit,
        "observedAtUtc": "2026-09-15T23:00:00Z",
        "observer": "synthetic-self-test",
        "railway": {
            "projectName": "World Makers Web",
            "environment": "production",
            "services": [
                {"name": "parent-portal", "deploymentStatus": "SUCCESS"},
                {"name": "player-dashboard", "deploymentStatus": "SUCCESS"},
                {"name": "worldmakers-api", "deploymentStatus": "SUCCESS"}
            ]
        },
        "domains": [
            {"service": "worldmakers-api", "host": "api.worldmakers.ctgone.com", "path": "/api/ready", "dnsResolved": True, "httpsHealthy": True, "statusCode": 200},
            {"service": "player-dashboard", "host": "play.worldmakers.ctgone.com", "path": "/api/health", "dnsResolved": True, "httpsHealthy": True, "statusCode": 200},
            {"service": "parent-portal", "host": "parents.worldmakers.ctgone.com", "path": "/api/health", "dnsResolved": True, "httpsHealthy": True, "statusCode": 200}
        ],
        "ctgOneE2E": {
            "realBearerUsed": True,
            "bearerPersisted": False,
            "getPlayerStatePassed": True,
            "initialRevision": 4,
            "putPlayerStatePassed": True,
            "writeRevision": 5,
            "idempotentRetryPassed": True,
            "retryRevision": 5,
            "revisionConflictPassed": True,
            "supabasePersistenceObserved": True,
            "temporaryProbeDataCleaned": True
        },
        "security": {
            "secretValuesRecorded": False,
            "bearerValueRecorded": False,
            "childPiiRecorded": False,
            "openChatEnabled": False,
            "userVoiceCaptureEnabled": False,
            "runtimeClientContainsPrivilegedCredential": False
        },
        "evidence": {"sanitizedResponseHashes": [], "notes": ["synthetic self-test only"]}
    }


def self_test() -> None:
    contract = load_json(CONTRACT_PATH)
    commit = "a" * 40
    fixture = compliant_fixture(commit)
    require(assess(contract, fixture, commit, True)["status"] == "PRE_UNREAL_READY", "compliant fixture should pass")

    cases: list[tuple[str, dict[str, Any]]] = []
    bad_domain = copy.deepcopy(fixture)
    bad_domain["domains"][0]["httpsHealthy"] = False
    cases.append(("unhealthy custom domain", bad_domain))
    bad_retry = copy.deepcopy(fixture)
    bad_retry["ctgOneE2E"]["idempotentRetryPassed"] = False
    cases.append(("failed idempotent retry", bad_retry))
    bad_conflict = copy.deepcopy(fixture)
    bad_conflict["ctgOneE2E"]["revisionConflictPassed"] = False
    cases.append(("missing revision conflict", bad_conflict))
    bad_secret = copy.deepcopy(fixture)
    bad_secret["security"]["secretValuesRecorded"] = True
    cases.append(("secret evidence", bad_secret))
    bad_commit = copy.deepcopy(fixture)
    bad_commit["repositoryCommit"] = "b" * 40
    cases.append(("commit mismatch", bad_commit))

    for label, broken in cases:
        result = assess(contract, broken, commit, True)
        require(result["status"] == "BLOCKED", f"self-test did not fail closed: {label}")

    non_certifying = assess(contract, fixture, commit, False)
    require(non_certifying["status"] == "NON_CERTIFYING_PASS", "off-main evidence must not certify")
    print("Final Pre-Unreal assessor self-test passed: domain failure, idempotency failure, missing conflict, secret evidence and commit mismatch all fail closed.")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--contract", default=str(CONTRACT_PATH))
    parser.add_argument("--evidence", default=str(DEFAULT_EVIDENCE))
    parser.add_argument("--expected-commit", default="")
    parser.add_argument("--output", default=str(DEFAULT_OUTPUT))
    parser.add_argument("--certifying-context", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    if args.self_test:
        self_test()
        return 0

    contract = load_json(Path(args.contract))
    expected_commit = args.expected_commit.strip()
    require(expected_commit, "--expected-commit is required")
    try:
        evidence = load_json(Path(args.evidence))
        result = assess(contract, evidence, expected_commit, args.certifying_context)
    except GateError as exc:
        result = build_result(expected_commit, [str(exc)], args.certifying_context)

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))
    return 0 if result["status"] in {"PRE_UNREAL_READY", "NON_CERTIFYING_PASS"} else 2


if __name__ == "__main__":
    raise SystemExit(main())
