#!/usr/bin/env python3
"""Validate M3.8 vertical-slice certification source contracts."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

REQUIRED_FILES = (
    "content/certification/m3-vertical-slice-route.schema.json",
    "content/certification/m3-tablet-device-evidence.schema.json",
    "docs/templates/m3-vertical-slice-route.example.json",
    "docs/templates/m3-tablet-device-evidence.example.json",
    "docs/m3-8-vertical-slice-certification.md",
    "scripts/assess-m3-8-certification.py",
    ".github/workflows/m3-certification.yml",
)


def fail(message: str) -> None:
    raise SystemExit(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def main() -> None:
    missing = [path for path in REQUIRED_FILES if not (ROOT / path).is_file()]
    require(not missing, f"Missing M3.8 files: {missing}")

    route_schema = json.loads(read("content/certification/m3-vertical-slice-route.schema.json"))
    device_schema = json.loads(read("content/certification/m3-tablet-device-evidence.schema.json"))
    for name, schema in (("route", route_schema), ("device", device_schema)):
        require(schema.get("type") == "object", f"M3.8 {name} schema must be an object")
        require(schema.get("additionalProperties") is False, f"M3.8 {name} schema must reject free-form fields")
        require(schema.get("properties", {}).get("schemaVersion", {}).get("const") == 1, f"M3.8 {name} schema version must be locked to 1")

    route_checks = route_schema["properties"]["checks"]
    require(route_checks.get("additionalProperties") is False, "Vertical-slice route checks must reject undeclared fields")
    required_route_checks = {
        "biomeLoaded",
        "semanticExploration",
        "deliberateObservations",
        "scienceMissionCompleted",
        "reactiveEcosystemVisible",
        "ecologicalBuildApplied",
        "creativeUnlockGranted",
        "myAdventuresProgressVisible",
        "stopWithoutPenalty",
        "resumeRestoresProgress",
        "noCommercialPressure",
    }
    require(set(route_checks.get("required", [])) == required_route_checks, "M3.8 route matrix drifted")

    device_props = device_schema.get("properties", {})
    require(device_props.get("platform", {}).get("enum") == ["iPadOS", "Android"], "Representative tablet contract must require iPadOS and Android classes")
    require(device_props.get("unrealVersion", {}).get("const") == "5.8.2", "Device evidence must lock Unreal Engine 5.8.2")
    require("serialNumber" not in device_props and "deviceId" not in device_props, "Device evidence must not collect hardware identifiers")

    route_example = json.loads(read("docs/templates/m3-vertical-slice-route.example.json"))
    device_example = json.loads(read("docs/templates/m3-tablet-device-evidence.example.json"))
    require(route_example.get("status") == "pending", "Route example must never ship as pre-passed evidence")
    require(device_example.get("status") == "pending", "Device example must never ship as pre-passed evidence")
    require(all(value == "pending" for value in route_example.get("checks", {}).values()), "Route example checks must default to pending")

    assessor = read("scripts/assess-m3-8-certification.py")
    for token in (
        '"CERTIFIED" if certified else "BLOCKED"',
        "--require-certified",
        "--self-test",
        "REQUIRED_TABLET_PLATFORMS = {\"iPadOS\", \"Android\"}",
        "MIN_FRAME_SAMPLES = 1800",
        "captureSha256",
        "sha256_file",
        "withinBudget",
        "frameTimeBudgetMs",
        "maxWorldActors",
        "maxPlacedBuildPieces",
        "maxActiveInteractables",
        "FORBIDDEN_NORMALIZED_KEYS",
        "evidenceIntegrity",
        "verticalSliceRoute",
    ):
        require(token in assessor, f"M3.8 assessor missing fail-closed contract token: {token}")

    self_test = subprocess.run(
        [sys.executable, str(ROOT / "scripts/assess-m3-8-certification.py"), "--self-test"],
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=False,
    )
    require(self_test.returncode == 0, "M3.8 assessor self-test failed:\n" + self_test.stdout + self_test.stderr)

    collector = read("scripts/collect-unreal-certification-evidence.ps1")
    for token in (
        "VerticalSliceEvidenceFile",
        "DeviceEvidenceRoot",
        "vertical-slice-route.json",
        "m3VerticalSliceEvidence",
        "deviceEvidenceFiles",
        "performance/",
        "Get-FileHash",
    ):
        require(token in collector, f"Certification collector missing M3.8 evidence token: {token}")

    repo_quality = read(".github/workflows/repo-quality.yml")
    unreal_ci = read(".github/workflows/unreal-ci.yml")
    gate = "python scripts/validate-m3-8-vertical-slice-certification.py"
    require(gate in repo_quality and gate in unreal_ci, "M3.8 source validator must run in Repository Quality and Unreal CI")
    require("assess-m3-8-certification.py --evidence-dir artifacts/certification" in unreal_ci, "Unreal CI must emit a non-certifying M3 assessment artifact")

    cert_workflow = read(".github/workflows/m3-certification.yml")
    for token in (
        "workflow_dispatch:",
        "self-hosted",
        "Windows",
        "X64",
        "unreal",
        "UNREAL_SELF_HOSTED_ENABLED",
        "UNREAL_ENGINE_ROOT",
        "M3_DEVICE_EVIDENCE_ROOT",
        "M3_VERTICAL_SLICE_EVIDENCE_FILE",
        "-RequireAuthoredMap",
        "-TestFilter 'WorldMakers.'",
        "--require-certified",
        "m3-vertical-slice-certification-${{ github.sha }}",
    ):
        require(token in cert_workflow, f"M3 certification workflow missing release gate token: {token}")
    require("pull_request:" not in cert_workflow and "push:" not in cert_workflow, "M3 certification workflow must not masquerade as ordinary push/PR CI")

    roadmap = read("docs/roadmap.md")
    require("M3.8 — vertical-slice certification infrastructure: source-complete" in roadmap, "Roadmap must mark M3.8 certification infrastructure source-complete")
    require("runtime/device certification blocked by Issue #9 and external evidence" in roadmap, "Roadmap must preserve M3.8 external certification boundary")

    prior_gate = read("scripts/validate-m3-7-tablet-performance.py")
    require("M3.8 — vertical-slice certification: next" not in prior_gate, "M3.7 validator must be forward-compatible after M3.8 starts")

    docs = read("docs/m3-8-vertical-slice-certification.md").lower()
    for token in (
        "fail-closed",
        "issue #9",
        "iPadOS".lower(),
        "android",
        "at least two",
        "p95",
        "sha-256",
        "pii",
        "worldmakers.*",
        "m4",
    ):
        require(token in docs, f"M3.8 documentation missing certification boundary: {token}")

    tdd = read("docs/TDD.md")
    for token in (
        "M3.8 certification matrix",
        "representative iPadOS",
        "representative Android",
        "fail-closed",
        "m3-vertical-slice-assessment.json",
    ):
        require(token in tdd, f"TDD missing M3.8 architecture decision: {token}")

    print("M3.8 vertical-slice certification source contract validated: fail-closed assessor, native/manual/route/device evidence matrix, integrity checks and release workflow are wired.")


if __name__ == "__main__":
    main()
