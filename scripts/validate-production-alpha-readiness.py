#!/usr/bin/env python3
"""Validate Production Alpha readiness source infrastructure without claiming native packaging evidence."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "content/production/production-alpha-readiness-v1.json"
G4 = ROOT / "content/production/g4-device-performance-stability-v1.json"
BUILD_TEMPLATE = ROOT / "content/production/alpha-build-manifest-template.json"
SMOKE_TEMPLATE = ROOT / "content/production/alpha-smoke-evidence-template.json"
OPS_TEMPLATE = ROOT / "content/production/alpha-ops-readiness-template.json"
BUILD_SCRIPT = ROOT / "scripts/build-production-alpha.ps1"
ASSESSOR = ROOT / "scripts/assess-production-alpha-readiness.py"
RUNNER = ROOT / "scripts/run-production-alpha-readiness.ps1"
WORKFLOW = ROOT / ".github/workflows/production-alpha-readiness.yml"
DOC = ROOT / "docs/production-alpha-readiness.md"
LAUNCHER = ROOT / "WorldMakers-ProductionAlpha-Certify.cmd"
BUILD_LAUNCHER = ROOT / "WorldMakers-ProductionAlpha-Build.cmd"


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit("Production Alpha source validation FAILED: " + message)


def contains(path: Path, tokens: list[str]) -> None:
    text = path.read_text(encoding="utf-8")
    for token in tokens:
        require(token in text, f"{path.relative_to(ROOT)} missing required token: {token}")


def main() -> int:
    required_files = [
        CONTRACT, G4, BUILD_TEMPLATE, SMOKE_TEMPLATE, OPS_TEMPLATE,
        BUILD_SCRIPT, ASSESSOR, RUNNER, WORKFLOW, DOC, LAUNCHER, BUILD_LAUNCHER,
        ROOT / "game/WorldMakers.uproject", ROOT / "game/UNREAL_ENGINE_VERSION",
        ROOT / "game/Source/WorldMakers.Target.cs",
    ]
    for path in required_files:
        require(path.is_file(), f"missing required file: {path.relative_to(ROOT)}")

    contract = load(CONTRACT)
    g4 = load(G4)
    build_template = load(BUILD_TEMPLATE)
    smoke_template = load(SMOKE_TEMPLATE)
    ops_template = load(OPS_TEMPLATE)

    require(contract.get("schema") == "worldmakers.production-alpha-readiness.v1", "contract schema mismatch")
    require(contract.get("status") == "source-ready-native-packaging-and-playtest-evidence-required", "contract must remain non-certifying at source level")
    require(contract["engine"] == {"expectedVersion": "5.8.2", "project": "game/WorldMakers.uproject", "gameTarget": "WorldMakers"}, "engine/game target contract mismatch")
    require((ROOT / "game/UNREAL_ENGINE_VERSION").read_text().strip() == "5.8.2", "repository engine lock must remain UE 5.8.2")
    require(contract["dependencies"]["g4Contract"] == "content/production/g4-device-performance-stability-v1.json", "G4 dependency mismatch")
    require(g4.get("schema") == "worldmakers.g4-device-performance-stability.v1", "G4 source contract mismatch")

    targets = contract.get("requiredBuildTargets") or []
    expected = {
        "windows-reference": ("Win64", "Development"),
        "android-alpha": ("Android", "Development"),
        "ipados-alpha": ("IOS", "Development"),
    }
    require({row.get("id") for row in targets} == set(expected), "exact Win64/Android/iPadOS alpha targets are required")
    require(len(targets) == 3, "exactly three build targets are required")
    for row in targets:
        require((row.get("unrealPlatform"), row.get("configuration")) == expected[row["id"]], f"target mapping mismatch for {row['id']}")

    packaging = contract["packaging"]
    for key in ("buildCookRunRequired", "cookRequired", "stageRequired", "pakRequired", "archiveRequired", "singleBuildCommitRequired", "artifactSha256Required", "manifestSha256Required"):
        require(packaging.get(key) is True, f"packaging.{key} must be true")
    require(packaging.get("distributionSigningRequiredForExternalDistribution") is True, "external mobile distribution must require signing")

    smoke = contract["smokeTest"]
    for key in ("installLaunchRequired", "certificationMapLoadRequired", "firstPersonControlRequired", "observeScanRequired", "scienceInteractionRequired", "buildPlaceRequired", "saveReloadRequired", "backgroundResumeRequiredOnMobile", "cleanExitRequired"):
        require(smoke.get(key) is True, f"smokeTest.{key} must be true")
    require(smoke.get("crashesAllowed") == 0 and smoke.get("fatalErrorsAllowed") == 0 and smoke.get("blockingIssuesAllowed") == 0, "alpha smoke tolerance must be zero for fatal/blocking failures")

    crash = contract["crashTelemetry"]
    require(crash.get("childPiiAllowed") is False and crash.get("freeTextChildDataCollectionAllowed") is False, "crash telemetry must prohibit child PII/free text")
    require(set(crash.get("minimumFields", [])) == {"buildCommit", "buildVersion", "platform", "engineVersion", "crashSignature", "timestampUtc"}, "crash telemetry minimum fields mismatch")

    playtest = contract["playtest"]
    require(playtest.get("minimumExternalTestersBeforeExit") >= 5, "external tester floor must be at least 5")
    require(playtest.get("guardianOrAuthorizedAdultFlowRequiredForChildTesting") is True, "child testing must require authorized adult/guardian flow")
    require(playtest.get("noRealMoneyOrTokenEarningRequired") is True and playtest.get("noOpenChatRequired") is True and playtest.get("feedbackMustNotRequestChildPii") is True, "alpha child-safety boundaries are incomplete")

    require(build_template.get("status") == "pending", "build manifest template must not pre-approve a build")
    require(smoke_template.get("status") == "pending", "smoke template must not pre-approve a build")
    require(ops_template.get("status") == "pending", "ops template must not pre-approve release readiness")
    require(ops_template["crashTelemetry"]["childPiiCollected"] is False, "ops template must default to no child PII")
    require(ops_template["playtestSafety"]["openChatEnabled"] is False, "ops template must default to open chat disabled")
    require(ops_template["playtestSafety"]["realMoneyOrTokenEarningEnabled"] is False, "ops template must default to real-money/token earning disabled")

    contains(BUILD_SCRIPT, [
        "BuildCookRun", "-build", "-cook", "-stage", "-pak", "-package", "-archive",
        "-cookflavor=ASTC", "iPadOS alpha packaging requires the macOS/Xcode Unreal build lane",
        "aggregateArtifactSha256", "Get-FileHash -Algorithm SHA256", "clean worktree",
    ])
    contains(ASSESSOR, [
        "G4 must be CERTIFIED", "mobile alpha package requires verified signing identity",
        "artifact SHA-256 mismatch", "background/resume smoke check must pass",
        "authorized adult/guardian flow", "Unsafe open chat must block Production Alpha certification",
    ])
    contains(RUNNER, ["production-alpha-readiness.json", "alpha-ops-readiness.json", "NON_CERTIFYING_PASS", "origin/main"])
    contains(WORKFLOW, ["Production Alpha Readiness", "--self-test", "validate-production-alpha-readiness.py", "Parser]::ParseFile"])
    contains(DOC, ["Production Alpha", "G4", "Win64", "Android", "iPadOS", "crash", "playtest"])
    contains(LAUNCHER, ["run-production-alpha-readiness.ps1"])
    contains(BUILD_LAUNCHER, ["build-production-alpha.ps1"])

    print("Production Alpha source validation PASS: packaging/provenance/smoke/ops contracts are coherent and remain fail-closed pending native build evidence.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
