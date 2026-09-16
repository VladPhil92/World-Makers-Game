#!/usr/bin/env python3
"""Validate Final Pre-Unreal Handoff source infrastructure without claiming live certification."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class ValidationError(RuntimeError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValidationError(message)


def read(path: str) -> str:
    target = ROOT / path
    require(target.is_file(), f"missing {path}")
    return target.read_text(encoding="utf-8")


def load_json(path: str) -> dict:
    return json.loads(read(path))


def validate() -> None:
    contract = load_json("content/production/final-pre-unreal-handoff-v1.json")
    template = load_json("content/production/pre-unreal-cloud-e2e-template.json")

    require(contract.get("schema") == "worldmakers.final-pre-unreal-handoff.v1", "final handoff schema mismatch")
    require(contract.get("status") == "audit-gate-active-external-evidence-required", "final handoff status must remain evidence-required")
    require(contract.get("engine", {}).get("requiredVersion") == "5.8.2", "engine lock drift")
    require(contract.get("engine", {}).get("project") == "game/WorldMakers.uproject", "project path drift")

    dependencies = contract.get("dependencies", {})
    expected_dependencies = {
        "preUnrealSourceContract": "content/production/pre-unreal-content-platform-readiness-v1.json",
        "cloudRuntimeValidator": "scripts/validate-cloud-runtime-readiness.py",
        "sourceReadinessValidator": "scripts/validate-pre-unreal-content-platform.py",
        "sourcePreflightValidator": "scripts/validate-unreal-source-preflight.py",
        "productionBaselineValidator": "scripts/validate-unreal-production-baseline.py",
    }
    require(dependencies == expected_dependencies, "final handoff dependency inventory drift")
    for path in expected_dependencies.values():
        require((ROOT / path).is_file(), f"handoff dependency missing: {path}")

    external = contract.get("requiredExternalEvidence", {})
    railway = external.get("railway", {})
    require(railway.get("projectName") == "World Makers Web", "Railway project mismatch")
    require(railway.get("environment") == "production", "Railway environment must be production")
    require(set(railway.get("requiredServices", [])) == {"parent-portal", "player-dashboard", "worldmakers-api"}, "Railway service inventory drift")
    require(railway.get("requiredDeploymentStatus") == "SUCCESS", "Railway success status drift")

    domains = external.get("customDomains", [])
    expected_domains = {
        ("worldmakers-api", "api.worldmakers.ctgone.com", "/api/ready"),
        ("player-dashboard", "play.worldmakers.ctgone.com", "/api/health"),
        ("parent-portal", "parents.worldmakers.ctgone.com", "/api/health"),
    }
    actual_domains = {(item.get("service"), item.get("host"), item.get("requiredPath")) for item in domains}
    require(actual_domains == expected_domains, "custom domain handoff matrix drift")

    e2e = external.get("cloudPlayerStateE2E", {})
    for flag in ("realCtgOneBearerRequired", "bearerMustNeverBePersisted", "getRequired", "putRequired", "idempotentRetryRequired", "revisionConflictRequired", "supabasePersistenceObservationRequired"):
        require(e2e.get(flag) is True, f"cloud E2E requirement disabled: {flag}")

    security = contract.get("securityInvariants", {})
    for flag in ("noChildPiiInEvidence", "noSecretValuesInEvidence", "noBearerInEvidence", "noOpenChat", "noUserVoiceCapture", "noSupabaseServiceRoleInClient", "noDatabasePasswordInClient", "noBridgeHmacSecretInClient", "httpsOnlyRuntimeApi"):
        require(security.get(flag) is True, f"security invariant disabled: {flag}")

    cert = contract.get("certification", {})
    require(cert.get("officialState") == "PRE_UNREAL_READY", "official state drift")
    require(cert.get("sourceCiMayCertifyOfficialState") is False, "source CI must never self-certify")
    require(cert.get("requiresCleanCurrentMain") is True, "clean main requirement missing")
    require(cert.get("requiresSameRepositoryCommit") is True, "same commit requirement missing")
    require(set(cert.get("states", [])) == {"PRE_UNREAL_READY", "NON_CERTIFYING_PASS", "BLOCKED"}, "handoff states drift")
    require(contract.get("exitCriteria", {}).get("nextPhase") == "native-unreal-materialization", "next phase drift")

    require(template.get("schema") == "worldmakers.pre-unreal-cloud-e2e-evidence.v1", "cloud E2E template schema mismatch")
    require(template.get("status") == "pending", "evidence template must remain pending")
    require(template.get("repositoryCommit") == "", "evidence template must not carry a commit")
    require(template.get("observedAtUtc") == "", "evidence template must not fake observation time")
    require(template.get("observer") == "", "evidence template must not fake an observer")
    template_e2e = template.get("ctgOneE2E", {})
    require(template_e2e.get("realBearerUsed") is False, "template must not claim a real bearer")
    require(template_e2e.get("bearerPersisted") is False, "template bearer persistence must remain false")
    require(template_e2e.get("getPlayerStatePassed") is False, "template must not claim GET passed")
    require(template_e2e.get("putPlayerStatePassed") is False, "template must not claim PUT passed")
    require(template_e2e.get("idempotentRetryPassed") is False, "template must not claim idempotency passed")
    require(template_e2e.get("revisionConflictPassed") is False, "template must not claim conflict passed")
    require(template_e2e.get("supabasePersistenceObserved") is False, "template must not claim persistence")
    for row in template.get("domains", []):
        require(row.get("dnsResolved") is False and row.get("httpsHealthy") is False and row.get("statusCode") is None, "template domain evidence must remain unresolved")

    assessor = read("scripts/assess-final-pre-unreal-handoff.py")
    for marker in ("PRE_UNREAL_READY", "NON_CERTIFYING_PASS", "idempotent retry changed revision", "secret values recorded in evidence", "external evidence repository commit mismatch"):
        require(marker in assessor, f"assessor fail-closed marker missing: {marker}")

    probe = read("scripts/probe-pre-unreal-cloud-e2e.py")
    for marker in ("WORLD_MAKERS_E2E_BEARER", "--allow-mutating-dedicated-test-account", "Idempotency-Key", "revision_conflict", "temporaryProbeDataCleaned"):
        require(marker in probe, f"cloud probe safety marker missing: {marker}")
    require("os.environ.pop(\"WORLD_MAKERS_E2E_BEARER\"" in probe, "cloud probe must clear bearer environment value")
    require("Authorization\"] = f\"Bearer {bearer}\"" in probe, "cloud probe bearer wiring missing")
    require("evidence[\"bearer\"]" not in probe, "cloud probe must never persist bearer")

    runner = read("scripts/run-final-pre-unreal-handoff.ps1")
    for marker in ("validate-pre-unreal-content-platform.py", "validate-cloud-runtime-readiness.py", "validate-final-pre-unreal-handoff.py", "assess-final-pre-unreal-handoff.py", "origin/main", "NON_CERTIFYING_PASS"):
        require(marker in runner, f"final runner marker missing: {marker}")

    workflow = read(".github/workflows/final-pre-unreal-handoff.yml")
    for marker in ("validate-pre-unreal-content-platform.py", "validate-cloud-runtime-readiness.py", "validate-final-pre-unreal-handoff.py", "--self-test"):
        require(marker in workflow, f"workflow marker missing: {marker}")
    require("PRE_UNREAL_READY" not in workflow or "cannot certify PRE_UNREAL_READY" in workflow, "workflow must not imply live certification")

    docs = read("docs/final-pre-unreal-handoff.md")
    for marker in ("api.worldmakers.ctgone.com", "WORLD_MAKERS_E2E_BEARER", "dedicated adult-owned test account", "PRE_UNREAL_READY", "native Unreal certification"):
        require(marker in docs, f"handoff documentation marker missing: {marker}")

    cmd = read("WorldMakers-FinalPreUnreal-Certify.cmd")
    require("run-final-pre-unreal-handoff.ps1" in cmd, "final handoff launcher not wired")

    print("Final Pre-Unreal Handoff source infrastructure: PASS")
    print("PASS does not certify PRE_UNREAL_READY; live domain and authenticated cloud E2E evidence remain mandatory.")


if __name__ == "__main__":
    try:
        validate()
    except (ValidationError, json.JSONDecodeError) as exc:
        raise SystemExit(f"Final Pre-Unreal Handoff validation FAILED: {exc}")
