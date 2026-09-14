#!/usr/bin/env python3
"""Validate the World Makers G1 native Unreal certification contract.

This is source governance only. It verifies that the repository cannot silently
weaken or conflate G1 with authored-map/device certification. It never claims a
native Unreal pass on a hosted runner.
"""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "content/production/g1-native-certification-v1.json"
DOC = ROOT / "docs/g1-native-unreal-certification.md"
RUNNER = ROOT / "scripts/run-g1-native-certification.ps1"
READINESS = ROOT / "scripts/run-unreal-readiness-gate.ps1"
BUILD = ROOT / "scripts/build-unreal.ps1"
TEST = ROOT / "scripts/test-unreal.ps1"
CMD = ROOT / "WorldMakers-G1-Certify.cmd"
WORKFLOW = ROOT / ".github/workflows/g1-native-certification.yml"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"ERROR: {message}")


def text(path: Path) -> str:
    require(path.is_file(), f"missing {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8")


contract = json.loads(text(CONTRACT))
require(contract["schema"] == "worldmakers.g1-native-certification.v1", "G1 schema drift")
require(contract["engine"] == {
    "expectedVersion": "5.8.2",
    "project": "game/WorldMakers.uproject",
    "target": "WorldMakersEditor",
    "platform": "Win64",
    "configuration": "Development",
}, "locked G1 engine/build contract drift")
require(contract["automation"] == {"required": True, "testFilter": "WorldMakers."}, "G1 automation scope drift")
require(contract["canonicalResult"] == "artifacts/g1-native/g1-native-readiness.json", "canonical G1 result path drift")
require(set(contract["resultStates"]) == {"CERTIFIED", "BLOCKED", "NON_CERTIFYING_PASS"}, "G1 result states drift")

criteria = contract["passCriteria"]
for key, expected in {
    "readinessStatus": "ready",
    "workstationDoctorStatus": "ready",
    "sourcePreflightStatus": "passed",
    "nativeBuildStatus": "passed",
    "nativeAutomationStatus": "passed",
    "expectedUnrealVersion": "5.8.2",
    "actualUnrealVersion": "5.8.2",
    "requireAuthoredMap": False,
    "runAutomation": True,
    "testFilter": "WorldMakers.",
    "certificationBranch": "main",
    "requiresOriginMainCommitMatch": True,
    "requiresCleanWorktree": True,
}.items():
    require(criteria.get(key) == expected, f"G1 pass criterion drift: {key}")

separation = contract["separationOfGates"]
require(separation["authoredCertificationMapRequiredAtG1"] is False, "G1 must not require authored map")
require(separation["authoredCertificationMapFirstRequiredAt"] == "G2", "authored map must first enter at G2")
require(separation["manualSmokeRequiredAtG1"] is False, "manual smoke must not be a G1 prerequisite")
require(separation["representativeDeviceEvidenceRequiredAtG1"] is False, "device evidence must not be a G1 prerequisite")

runner = text(RUNNER)
for token in (
    "run-unreal-readiness-gate.ps1",
    "WorldMakers.",
    "WorldMakersEditor",
    "NON_CERTIFYING_PASS",
    "CERTIFIED",
    "BLOCKED",
    "g1-native-readiness.json",
    "nativeBuildStatus -eq 'passed'",
    "nativeAutomationStatus -eq 'passed'",
    "branch -eq 'main'",
    "repositoryCommit -eq $Readiness.originMainCommit",
    "authoredMapRequired = $false",
    "authoredMapGate = 'G2'",
):
    require(token in runner, f"G1 orchestrator missing contract token: {token}")
require("RequireAuthoredMap" not in runner, "G1 orchestrator must not request the authored-map gate")

readiness = text(READINESS)
for token in ("[switch]$RunAutomation", "nativeBuildStatus", "nativeAutomationStatus", "[string]$TestFilter = 'WorldMakers.'"):
    require(token in readiness, f"readiness gate no longer supports G1 requirement: {token}")

build = text(BUILD)
for token in ("WorldMakersEditor", "Win64", "Development", "build-result.json"):
    require(token in build, f"native build contract missing token: {token}")

test = text(TEST)
for token in ("[string]$TestFilter = 'WorldMakers.'", "Automation RunTests $TestFilter", "automation-result.json"):
    require(token in test, f"native automation contract missing token: {token}")

cmd = text(CMD)
require("run-g1-native-certification.ps1" in cmd, "Windows G1 launcher is not wired to the orchestrator")
require("-CleanIntermediate" in cmd, "Windows G1 launcher must request a clean build")

workflow = text(WORKFLOW)
for token in (
    "Validate G1 source contract",
    "UNREAL_SELF_HOSTED_ENABLED",
    "self-hosted",
    "Windows",
    "X64",
    "unreal",
    "run-g1-native-certification.ps1",
    "artifacts/g1-native/**",
):
    require(token in workflow, f"G1 workflow missing token: {token}")

for token in (
    "G1 proves",
    "G2",
    "WorldMakers-G1-Certify.cmd",
    "NON_CERTIFYING_PASS",
    "CERTIFIED",
    "does not require final art",
):
    require(token in text(DOC), f"G1 documentation missing token: {token}")

print("World Makers G1 Native Unreal Certification contract: PASS")
print("Hosted validation proves orchestration integrity only; native UE 5.8.2 evidence remains required.")
