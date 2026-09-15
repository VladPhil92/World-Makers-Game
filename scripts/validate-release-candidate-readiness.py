#!/usr/bin/env python3
"""Validate Release Candidate source infrastructure without claiming native/release evidence."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "content/production/release-candidate-readiness-v1.json"
EXTERNAL_ALPHA = ROOT / "content/production/external-alpha-playtest-v1.json"
TEMPLATES = [
    ROOT / "content/production/rc-freeze-manifest-template.json",
    ROOT / "content/production/rc-reproducibility-template.json",
    ROOT / "content/production/rc-regression-template.json",
    ROOT / "content/production/rc-operations-template.json",
    ROOT / "content/production/rc-release-decision-template.json",
]
ASSESSOR = ROOT / "scripts/assess-release-candidate-readiness.py"
SBOM = ROOT / "scripts/generate-release-candidate-sbom.py"
RUNNER = ROOT / "scripts/run-release-candidate-readiness.ps1"
WORKFLOW = ROOT / ".github/workflows/release-candidate-readiness.yml"
DOC = ROOT / "docs/release-candidate-readiness.md"
LAUNCHER = ROOT / "WorldMakers-ReleaseCandidate-Certify.cmd"


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit("Release Candidate source validation FAILED: " + message)


def contains(path: Path, tokens: list[str]) -> None:
    text = path.read_text(encoding="utf-8")
    for token in tokens:
        require(token in text, f"{path.relative_to(ROOT)} missing required token: {token}")


def main() -> int:
    files = [CONTRACT, EXTERNAL_ALPHA, ASSESSOR, SBOM, RUNNER, WORKFLOW, DOC, LAUNCHER, ROOT / "game/UNREAL_ENGINE_VERSION", ROOT / "game/WorldMakers.uproject", *TEMPLATES]
    for path in files:
        require(path.is_file(), f"missing required file: {path.relative_to(ROOT)}")

    contract = load(CONTRACT)
    ext = load(EXTERNAL_ALPHA)
    require(contract.get("schema") == "worldmakers.release-candidate-readiness.v1", "contract schema mismatch")
    require(contract.get("status") == "source-ready-release-evidence-required", "source contract must remain non-certifying")
    require(contract["engine"]["expectedVersion"] == "5.8.2", "engine version must remain UE 5.8.2")
    require((ROOT / "game/UNREAL_ENGINE_VERSION").read_text().strip() == "5.8.2", "repository engine lock mismatch")
    require(ext.get("schema") == "worldmakers.external-alpha-playtest.v1", "External Alpha dependency schema mismatch")
    require(contract["dependencies"]["externalAlphaResult"] == "artifacts/external-alpha/external-alpha-readiness.json", "External Alpha result path mismatch")
    require(set(contract["requiredTargets"]) == {"windows-reference", "android-alpha", "ipados-alpha"}, "canonical RC target matrix mismatch")

    for key in ("semanticVersionRequired", "releaseCandidatePrereleaseRequired", "singleBuildCommitRequired", "cleanMainRequired", "originMainMatchRequired", "contentFreezeRequired", "dependencyFreezeRequired", "configurationFreezeRequired"):
        require(contract["freeze"].get(key) is True, f"freeze.{key} must be true")
    for key in ("independentRebuildRequired", "normalizedPayloadDigestRequired", "payloadDigestMustMatch", "allTargetsRequired", "sameEngineVersionRequired"):
        require(contract["reproducibility"].get(key) is True, f"reproducibility.{key} must be true")
    require(contract["supplyChain"]["knownCriticalVulnerabilitiesAllowed"] == 0, "critical vulnerability tolerance must be zero")
    require(contract["supplyChain"]["knownHighVulnerabilitiesAllowed"] == 0, "high vulnerability tolerance must be zero")
    require(set(contract["supplyChain"]["allowedSbomFormats"]) == {"SPDX-2.3", "CycloneDX-1.6"}, "SBOM formats mismatch")
    for key in ("fullCriticalJourneyRequired", "saveUpgradeCompatibilityRequired", "freshInstallRequired", "upgradeInstallRequired", "offlineRecoveryRequired"):
        require(contract["regression"].get(key) is True, f"regression.{key} must be true")
    require(contract["regression"]["crashesAllowed"] == 0 and contract["regression"]["fatalErrorsAllowed"] == 0 and contract["regression"]["dataLossAllowed"] == 0, "RC reliability tolerance must be zero")
    require(contract["regression"]["openP0Allowed"] == 0 and contract["regression"]["openP1Allowed"] == 0, "RC blocker tolerance must be zero")
    for key in ("releaseNotesRequired", "rollbackPackageRequired", "rollbackProcedureVerified", "crashSymbolsRequired", "symbolUploadVerified", "distributionSigningVerifiedForMobile", "productionTelemetryRouteVerified"):
        require(contract["operations"].get(key) is True, f"operations.{key} must be true")
    require(contract["operations"]["childPiiAllowed"] is False and contract["operations"]["openChatAllowed"] is False and contract["operations"]["realMoneyOrTokenEarningAllowed"] is False, "RC safety boundaries are incomplete")
    for key in ("humanGoNoGoRequired", "qaApprovalRequired", "engineeringApprovalRequired", "childSafetyApprovalRequired", "productApprovalRequired", "securitySupplyChainApprovalRequired", "decisionMustMatchBuildCommit", "decisionMustMatchReleaseVersion"):
        require(contract["promotion"].get(key) is True, f"promotion.{key} must be true")
    require(contract["exit"]["nextPhase"] == "production-release-readiness", "RC exit phase mismatch")

    for template in TEMPLATES:
        require(load(template).get("status") == "pending", f"{template.name} must start pending")

    contains(ASSESSOR, [
        "External Alpha must be CERTIFIED", "independent rebuild payload digest does not reproduce canonical payload",
        "saveUpgradeCompatibilityPassed", "Known critical/high vulnerabilities must be zero",
        "Release notes SHA-256 mismatch", "Rollback package SHA-256 mismatch", "unsafe open chat did not block",
    ])
    contains(SBOM, ["SPDX-2.3", "package-lock.json", "WorldMakers.uproject", "UNREAL_ENGINE_VERSION"])
    contains(RUNNER, ["release-candidate-readiness.json", "external-alpha-readiness.json", "NON_CERTIFYING_PASS", "origin/main"])
    contains(WORKFLOW, ["Release Candidate Readiness", "--self-test", "generate-release-candidate-sbom.py", "Parser]::ParseFile"])
    contains(DOC, ["Release Candidate", "External Alpha", "SBOM", "reproducibility", "save", "rollback", "Production Release"])
    contains(LAUNCHER, ["run-release-candidate-readiness.ps1"])

    print("Release Candidate source validation PASS: freeze/reproducibility/SBOM/regression/operations/promotion contracts are coherent and fail closed pending real RC evidence.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
