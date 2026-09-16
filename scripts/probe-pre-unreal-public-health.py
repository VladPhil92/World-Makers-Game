#!/usr/bin/env python3
"""Collect secret-free public health evidence for the final Pre-Unreal handoff.

This probe deliberately does not use CTG One credentials and cannot emit
PRE_UNREAL_READY. It distinguishes custom-domain/DNS failures from underlying
Railway service reachability by checking both surfaces.
"""
from __future__ import annotations

import argparse
import json
import socket
import ssl
import urllib.error
import urllib.request
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CONFIG = ROOT / "content/production/pre-unreal-public-endpoints-v1.json"
DEFAULT_OUTPUT = ROOT / "artifacts/pre-unreal-handoff/public-health.json"


def utc_now() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def resolve_host(host: str) -> tuple[bool, list[str]]:
    try:
        rows = socket.getaddrinfo(host, 443, type=socket.SOCK_STREAM)
    except OSError:
        return False, []
    addresses = sorted({row[4][0] for row in rows if row and row[4]})
    return bool(addresses), addresses


def probe_https(host: str, path: str) -> dict[str, Any]:
    dns_resolved, addresses = resolve_host(host)
    result: dict[str, Any] = {
        "host": host,
        "path": path,
        "dnsResolved": dns_resolved,
        "resolvedAddressCount": len(addresses),
        "httpsReachable": False,
        "statusCode": None,
        "healthy2xx": False,
        "errorClass": None,
    }
    if not dns_resolved:
        result["errorClass"] = "dns_unresolved"
        return result

    request = urllib.request.Request(
        f"https://{host}{path}",
        headers={"Accept": "application/json", "User-Agent": "worldmakers-pre-unreal-public-health/1"},
    )
    try:
        with urllib.request.urlopen(request, timeout=15, context=ssl.create_default_context()) as response:
            result["statusCode"] = response.status
            response.read(4096)
            result["httpsReachable"] = True
            result["healthy2xx"] = 200 <= response.status < 300
    except urllib.error.HTTPError as exc:
        result["statusCode"] = exc.code
        result["httpsReachable"] = True
        result["healthy2xx"] = 200 <= exc.code < 300
        result["errorClass"] = "http_non_2xx"
    except ssl.SSLError:
        result["errorClass"] = "tls_error"
    except OSError:
        result["errorClass"] = "network_error"
    return result


def classify(custom: dict[str, Any], railway: dict[str, Any]) -> str:
    if custom["healthy2xx"]:
        return "healthy"
    if railway["healthy2xx"] and not custom["dnsResolved"]:
        return "custom_dns_unresolved"
    if railway["healthy2xx"] and custom["dnsResolved"]:
        return "custom_domain_routing_or_verification"
    if not railway["dnsResolved"]:
        return "public_dns_observer_failure_or_railway_dns_unresolved"
    return "service_or_public_networking_unhealthy"


def load_config(path: Path) -> dict[str, Any]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if data.get("schema") != "worldmakers.pre-unreal-public-endpoints.v1":
        raise ValueError("public endpoint config schema mismatch")
    services = data.get("services")
    if not isinstance(services, list) or len(services) != 3:
        raise ValueError("public endpoint inventory must contain exactly three services")
    required = {"worldmakers-api", "player-dashboard", "parent-portal"}
    if {row.get("service") for row in services} != required:
        raise ValueError("public endpoint service inventory drift")
    return data


def collect(config: dict[str, Any], repository_commit: str, observer: str) -> dict[str, Any]:
    rows = []
    for item in config["services"]:
        custom = probe_https(item["customHost"], item["healthPath"])
        railway = probe_https(item["railwayHost"], item["healthPath"])
        rows.append({
            "service": item["service"],
            "custom": custom,
            "railwayFallback": railway,
            "classification": classify(custom, railway),
        })

    custom_ok = all(row["custom"]["healthy2xx"] for row in rows)
    railway_ok = all(row["railwayFallback"]["healthy2xx"] for row in rows)
    return {
        "schema": "worldmakers.pre-unreal-public-health-evidence.v1",
        "status": "passed" if custom_ok else "blocked",
        "repositoryCommit": repository_commit,
        "observedAtUtc": utc_now(),
        "observer": observer,
        "project": config["project"],
        "environment": config["environment"],
        "services": rows,
        "summary": {
            "customDomainsHealthy": custom_ok,
            "railwayFallbacksHealthy": railway_ok,
            "authenticatedE2ERun": False,
            "canIssuePreUnrealReady": False,
        },
        "security": {
            "credentialsUsed": False,
            "secretValuesRecorded": False,
            "childPiiRecorded": False,
        },
    }


def self_test() -> None:
    healthy = {"healthy2xx": True, "dnsResolved": True}
    dns_down = {"healthy2xx": False, "dnsResolved": False}
    routed_down = {"healthy2xx": False, "dnsResolved": True}
    assert classify(healthy, healthy) == "healthy"
    assert classify(dns_down, healthy) == "custom_dns_unresolved"
    assert classify(routed_down, healthy) == "custom_domain_routing_or_verification"
    assert classify(dns_down, dns_down) == "public_dns_observer_failure_or_railway_dns_unresolved"
    print("Public Pre-Unreal health classifier self-test: PASS")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", default=str(DEFAULT_CONFIG))
    parser.add_argument("--output", default=str(DEFAULT_OUTPUT))
    parser.add_argument("--repository-commit", default="unknown")
    parser.add_argument("--observer", default="external-health-probe")
    parser.add_argument("--soft-fail", action="store_true", help="Write BLOCKED evidence but return 0 for diagnostic CI.")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    if args.self_test:
        self_test()
        return 0

    config = load_config(Path(args.config))
    evidence = collect(config, args.repository_commit, args.observer)
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(evidence["summary"], sort_keys=True))
    print(f"Public Pre-Unreal health evidence: {evidence['status']} -> {output}")
    if evidence["status"] == "passed" or args.soft_fail:
        return 0
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
