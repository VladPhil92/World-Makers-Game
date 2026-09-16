#!/usr/bin/env python3
"""Run the authorized, secret-safe World Makers pre-Unreal cloud E2E probe.

The CTG One Bearer token is read ONLY from WORLD_MAKERS_E2E_BEARER and is never
printed or written to evidence. Use a dedicated adult-owned test identity, never
a child account. This probe makes a reversible semantic marker write and removes
that marker before it exits.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import socket
import ssl
import sys
import urllib.error
import urllib.request
import uuid
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

DEFAULT_OUTPUT = Path("artifacts/pre-unreal-handoff/cloud-e2e.json")
DOMAINS = [
    ("worldmakers-api", "api.worldmakers.ctgone.com", "/api/ready"),
    ("player-dashboard", "play.worldmakers.ctgone.com", "/api/health"),
    ("parent-portal", "parents.worldmakers.ctgone.com", "/api/health"),
]
PLAYER_STATE_URL = "https://api.worldmakers.ctgone.com/api/worldmakers/player-state"


class ProbeError(RuntimeError):
    pass


def utc_now() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def sha256_text(value: str) -> str:
    return hashlib.sha256(value.encode("utf-8")).hexdigest()


def request_json(url: str, *, method: str = "GET", bearer: str | None = None, body: dict[str, Any] | None = None, idempotency_key: str | None = None, expected_statuses: set[int] | None = None) -> tuple[int, dict[str, Any], str]:
    headers = {"Accept": "application/json", "User-Agent": "worldmakers-pre-unreal-e2e/1"}
    data = None
    if bearer:
        headers["Authorization"] = f"Bearer {bearer}"
    if body is not None:
        headers["Content-Type"] = "application/json"
        data = json.dumps(body, separators=(",", ":")).encode("utf-8")
    if idempotency_key:
        headers["Idempotency-Key"] = idempotency_key
    req = urllib.request.Request(url, method=method, headers=headers, data=data)
    try:
        with urllib.request.urlopen(req, timeout=15, context=ssl.create_default_context()) as response:
            status = response.status
            raw = response.read().decode("utf-8", errors="replace")
    except urllib.error.HTTPError as exc:
        status = exc.code
        raw = exc.read().decode("utf-8", errors="replace")
    allowed = expected_statuses or {200}
    if status not in allowed:
        raise ProbeError(f"unexpected HTTP status {status} from {url}")
    try:
        parsed = json.loads(raw) if raw else {}
    except json.JSONDecodeError as exc:
        raise ProbeError(f"non-JSON response from {url}") from exc
    if not isinstance(parsed, dict):
        raise ProbeError(f"non-object response from {url}")
    return status, parsed, sha256_text(raw)


def extract_state(payload: dict[str, Any]) -> tuple[int, dict[str, list[Any]]]:
    if payload.get("schemaVersion") != 2:
        raise ProbeError("player-state response schemaVersion is not 2")
    profile = payload.get("profile")
    if not isinstance(profile, dict) or not isinstance(profile.get("revision"), int) or profile["revision"] < 0:
        raise ProbeError("player-state response has no valid revision")
    collections: dict[str, list[Any]] = {}
    for key in ("saves", "missions", "discoveries", "achievements"):
        value = payload.get(key)
        if not isinstance(value, list):
            raise ProbeError(f"player-state response collection is not an array: {key}")
        collections[key] = value
    return profile["revision"], collections


def health_domain(service: str, host: str, path: str) -> dict[str, Any]:
    dns_resolved = False
    https_healthy = False
    status_code: int | None = None
    try:
        socket.getaddrinfo(host, 443, type=socket.SOCK_STREAM)
        dns_resolved = True
    except OSError:
        pass
    if dns_resolved:
        try:
            req = urllib.request.Request(f"https://{host}{path}", headers={"Accept": "application/json", "User-Agent": "worldmakers-pre-unreal-e2e/1"})
            with urllib.request.urlopen(req, timeout=15, context=ssl.create_default_context()) as response:
                status_code = response.status
                response.read(4096)
            https_healthy = 200 <= status_code < 300
        except urllib.error.HTTPError as exc:
            status_code = exc.code
        except OSError:
            pass
    return {
        "service": service,
        "host": host,
        "path": path,
        "dnsResolved": dns_resolved,
        "httpsHealthy": https_healthy,
        "statusCode": status_code,
    }


def parse_railway_status(items: list[str]) -> list[dict[str, str]]:
    statuses: dict[str, str] = {}
    for item in items:
        if "=" not in item:
            raise ProbeError("--railway-status must use service=STATUS")
        name, status = item.split("=", 1)
        statuses[name.strip()] = status.strip()
    required = {"parent-portal", "player-dashboard", "worldmakers-api"}
    if set(statuses) != required:
        raise ProbeError("--railway-status must provide exactly parent-portal, player-dashboard and worldmakers-api")
    return [{"name": name, "deploymentStatus": statuses[name]} for name in sorted(required)]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repository-commit", required=True)
    parser.add_argument("--observer", required=True, help="Non-PII operator/team label, e.g. release-ops")
    parser.add_argument("--output", default=str(DEFAULT_OUTPUT))
    parser.add_argument("--railway-status", action="append", default=[], help="Repeat service=STATUS for the three production services")
    parser.add_argument("--allow-mutating-dedicated-test-account", action="store_true")
    args = parser.parse_args()

    bearer = os.environ.get("WORLD_MAKERS_E2E_BEARER", "").strip()
    if not bearer:
        raise SystemExit("WORLD_MAKERS_E2E_BEARER is required; the token is never written to disk")
    if not args.allow_mutating_dedicated_test_account:
        raise SystemExit("Refusing to write player state. Use a dedicated adult-owned test account and pass --allow-mutating-dedicated-test-account.")

    evidence: dict[str, Any] = {
        "schema": "worldmakers.pre-unreal-cloud-e2e-evidence.v1",
        "status": "pending",
        "repositoryCommit": args.repository_commit,
        "observedAtUtc": utc_now(),
        "observer": args.observer,
        "railway": {
            "projectName": "World Makers Web",
            "environment": "production",
            "services": parse_railway_status(args.railway_status),
        },
        "domains": [health_domain(*item) for item in DOMAINS],
        "ctgOneE2E": {
            "realBearerUsed": True,
            "bearerPersisted": False,
            "getPlayerStatePassed": False,
            "initialRevision": None,
            "putPlayerStatePassed": False,
            "writeRevision": None,
            "idempotentRetryPassed": False,
            "retryRevision": None,
            "revisionConflictPassed": False,
            "supabasePersistenceObserved": False,
            "temporaryProbeDataCleaned": False,
        },
        "security": {
            "secretValuesRecorded": False,
            "bearerValueRecorded": False,
            "childPiiRecorded": False,
            "openChatEnabled": False,
            "userVoiceCaptureEnabled": False,
            "runtimeClientContainsPrivilegedCredential": False,
        },
        "evidence": {"sanitizedResponseHashes": [], "notes": []},
    }

    hashes: list[str] = evidence["evidence"]["sanitizedResponseHashes"]
    probe_id = uuid.uuid4().hex
    marker = {"kind": "pre-unreal-e2e-probe", "probeId": probe_id}
    cleanup_needed = False
    current_revision: int | None = None
    baseline: dict[str, list[Any]] | None = None

    try:
        _, initial, digest = request_json(PLAYER_STATE_URL, bearer=bearer)
        hashes.append(digest)
        initial_revision, baseline = extract_state(initial)
        current_revision = initial_revision
        evidence["ctgOneE2E"]["getPlayerStatePassed"] = True
        evidence["ctgOneE2E"]["initialRevision"] = initial_revision

        probe_collections = {key: list(values) for key, values in baseline.items()}
        probe_collections["achievements"].append(marker)
        write_body = {"schemaVersion": 1, "expectedRevision": initial_revision, **probe_collections}
        write_key = f"preunreal.{probe_id}.write"
        _, written, digest = request_json(PLAYER_STATE_URL, method="PUT", bearer=bearer, body=write_body, idempotency_key=write_key)
        hashes.append(digest)
        write_revision, _ = extract_state(written)
        if write_revision <= initial_revision:
            raise ProbeError("player-state write did not advance revision")
        current_revision = write_revision
        cleanup_needed = True
        evidence["ctgOneE2E"]["putPlayerStatePassed"] = True
        evidence["ctgOneE2E"]["writeRevision"] = write_revision

        _, retried, digest = request_json(PLAYER_STATE_URL, method="PUT", bearer=bearer, body=write_body, idempotency_key=write_key)
        hashes.append(digest)
        retry_revision, _ = extract_state(retried)
        evidence["ctgOneE2E"]["retryRevision"] = retry_revision
        evidence["ctgOneE2E"]["idempotentRetryPassed"] = retry_revision == write_revision

        stale_key = f"preunreal.{probe_id}.stale"
        stale_status, stale_payload, digest = request_json(
            PLAYER_STATE_URL,
            method="PUT",
            bearer=bearer,
            body=write_body,
            idempotency_key=stale_key,
            expected_statuses={409},
        )
        hashes.append(digest)
        evidence["ctgOneE2E"]["revisionConflictPassed"] = stale_status == 409 and stale_payload.get("error") == "revision_conflict"

        _, persisted, digest = request_json(PLAYER_STATE_URL, bearer=bearer)
        hashes.append(digest)
        persisted_revision, persisted_collections = extract_state(persisted)
        current_revision = persisted_revision
        marker_present = any(isinstance(item, dict) and item.get("probeId") == probe_id for item in persisted_collections["achievements"])
        evidence["ctgOneE2E"]["supabasePersistenceObserved"] = marker_present and persisted_revision == write_revision

        cleanup_body = {"schemaVersion": 1, "expectedRevision": current_revision, **baseline}
        cleanup_key = f"preunreal.{probe_id}.cleanup"
        _, cleaned, digest = request_json(PLAYER_STATE_URL, method="PUT", bearer=bearer, body=cleanup_body, idempotency_key=cleanup_key)
        hashes.append(digest)
        cleanup_revision, _ = extract_state(cleaned)
        current_revision = cleanup_revision
        cleanup_needed = False

        _, final_state, digest = request_json(PLAYER_STATE_URL, bearer=bearer)
        hashes.append(digest)
        _, final_collections = extract_state(final_state)
        marker_absent = not any(isinstance(item, dict) and item.get("probeId") == probe_id for item in final_collections["achievements"])
        evidence["ctgOneE2E"]["temporaryProbeDataCleaned"] = marker_absent

        domains_ok = all(row["dnsResolved"] and row["httpsHealthy"] and isinstance(row["statusCode"], int) and 200 <= row["statusCode"] < 300 for row in evidence["domains"])
        railway_ok = all(row["deploymentStatus"] == "SUCCESS" for row in evidence["railway"]["services"])
        e2e = evidence["ctgOneE2E"]
        e2e_ok = all(e2e[key] is True for key in (
            "realBearerUsed", "getPlayerStatePassed", "putPlayerStatePassed", "idempotentRetryPassed",
            "revisionConflictPassed", "supabasePersistenceObserved", "temporaryProbeDataCleaned"
        )) and e2e["bearerPersisted"] is False
        evidence["status"] = "passed" if domains_ok and railway_ok and e2e_ok else "blocked"
    except Exception as exc:
        evidence["status"] = "blocked"
        evidence["evidence"]["notes"].append(f"probe failed: {type(exc).__name__}: {exc}")
        if cleanup_needed and baseline is not None and current_revision is not None:
            try:
                cleanup_body = {"schemaVersion": 1, "expectedRevision": current_revision, **baseline}
                cleanup_key = f"preunreal.{probe_id}.emergency-cleanup"
                _, _, digest = request_json(PLAYER_STATE_URL, method="PUT", bearer=bearer, body=cleanup_body, idempotency_key=cleanup_key)
                hashes.append(digest)
                evidence["ctgOneE2E"]["temporaryProbeDataCleaned"] = True
                evidence["evidence"]["notes"].append("emergency semantic cleanup succeeded")
            except Exception as cleanup_exc:
                evidence["evidence"]["notes"].append(f"EMERGENCY CLEANUP REQUIRED: {type(cleanup_exc).__name__}: {cleanup_exc}")
    finally:
        bearer = ""
        os.environ.pop("WORLD_MAKERS_E2E_BEARER", None)

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(f"Pre-Unreal cloud E2E evidence written to {output} with status={evidence['status']}; bearer value was not persisted.")
    return 0 if evidence["status"] == "passed" else 2


if __name__ == "__main__":
    raise SystemExit(main())
