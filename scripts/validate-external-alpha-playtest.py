#!/usr/bin/env python3
"""Validate External Alpha playtest source infrastructure without claiming human/device evidence."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "content/production/external-alpha-playtest-v1.json"
PROD_ALPHA = ROOT / "content/production/production-alpha-readiness-v1.json"
COHORT = ROOT / "content/production/external-alpha-cohort-template.json"
SESSION = ROOT / "content/production/external-alpha-session-template.json"
ISSUE = ROOT / "content/production/external-alpha-issue-template.json"
DECISION = ROOT / "content/production/external-alpha-release-decision-template.json"
ASSESSOR = ROOT / "scripts/assess-external-alpha-playtest.py"
RUNNER = ROOT / "scripts/run-external-alpha-playtest-readiness.ps1"
WORKFLOW = ROOT / ".github/workflows/external-alpha-playtest.yml"
DOC = ROOT / "docs/external-alpha-playtest.md"
LAUNCHER = ROOT / "WorldMakers-ExternalAlpha-Certify.cmd"


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit("External Alpha source validation FAILED: " + message)


def contains(path: Path, tokens: list[str]) -> None:
    text = path.read_text(encoding="utf-8")
    for token in tokens:
        require(token in text, f"{path.relative_to(ROOT)} missing token: {token}")


def main() -> int:
    for path in (CONTRACT, PROD_ALPHA, COHORT, SESSION, ISSUE, DECISION, ASSESSOR, RUNNER, WORKFLOW, DOC, LAUNCHER):
        require(path.is_file(), f"missing required file: {path.relative_to(ROOT)}")

    contract = load(CONTRACT)
    prod = load(PROD_ALPHA)
    cohort = load(COHORT)
    session = load(SESSION)
    issue = load(ISSUE)
    decision = load(DECISION)

    require(contract.get("schema") == "worldmakers.external-alpha-playtest.v1", "contract schema mismatch")
    require(contract.get("status") == "source-ready-controlled-playtest-evidence-required", "source status must remain non-certifying")
    require(prod.get("schema") == "worldmakers.production-alpha-readiness.v1", "Production Alpha dependency mismatch")
    require(contract["dependencies"]["productionAlphaResult"] == "artifacts/production-alpha/production-alpha-readiness.json", "Production Alpha result path mismatch")
    require(set(contract["requiredPlatforms"]) == {"Win64", "Android", "iPadOS"}, "platform coverage mismatch")

    cohort_rules = contract["cohort"]
    require(cohort_rules["minimumUniqueTesters"] >= 10, "minimum tester cohort is too small")
    require(cohort_rules["minimumTotalSessions"] >= 20, "minimum session sample is too small")
    require(cohort_rules["minimumCompletedSessions"] >= 18, "minimum completed-session sample is too small")
    require(cohort_rules["pseudonymousTesterIdsRequired"] is True and cohort_rules["rawIdentityDataAllowed"] is False, "tester identity minimization must be enforced")
    require(cohort_rules["minorTestingRequiresAuthorizedAdult"] is True, "minor testing must require authorized adult")

    slo = contract["experienceSlo"]
    require(slo["minimumSessionCompletionRate"] >= 0.90, "session-completion SLO too weak")
    require(slo["minimumCrashFreeSessionRate"] >= 0.98, "crash-free SLO too weak")
    require(slo["minimumCoreJourneyCompletionRate"] >= 0.90, "core-journey SLO too weak")
    require(slo["minimumSaveReloadSuccessRate"] >= 0.95, "save/reload SLO too weak")
    require(slo["minimumMobileBackgroundResumeSuccessRate"] >= 0.95, "mobile resume SLO too weak")

    policy = contract["issuePolicy"]
    require(policy["openP0Allowed"] == 0 and policy["openP1Allowed"] == 0, "P0/P1 must be zero at RC promotion")
    require(policy["openSafetyIssuesAllowed"] == 0, "open safety issues must be zero")
    require(policy["childPiiInIssueEvidenceAllowed"] is False, "child PII must be prohibited in issue evidence")
    require(policy["saveCorruptionIssueForcesRollback"] is True and policy["childSafetyIncidentForcesRollback"] is True, "critical rollback triggers missing")

    safety = contract["playtestSafety"]
    require(safety["openChatAllowed"] is False, "open chat must remain disabled")
    require(safety["realMoneyOrTokenEarningAllowed"] is False, "real-money/token earning must remain disabled")
    require(safety["childPiiCollectionAllowed"] is False and safety["freeTextChildDataCollectionAllowed"] is False, "child data minimization incomplete")
    require(safety["privacyReviewRequired"] is True and safety["childSafetyReviewRequired"] is True, "human safety/privacy review required")

    promotion = contract["promotion"]
    for key in ("humanGoNoGoRequired", "qaApprovalRequired", "childSafetyApprovalRequired", "productApprovalRequired", "rollbackPlanRequired", "telemetryReviewRequired", "promotionDecisionMustMatchBuildCommit"):
        require(promotion[key] is True, f"promotion.{key} must be true")
    require(promotion["toChannel"] == "release-candidate", "External Alpha must promote only to release-candidate")

    require(cohort["status"] == "pending", "cohort template must not pre-approve testing")
    require(session["status"] == "pending", "session template must not fabricate completed evidence")
    require(issue["status"] == "pending", "issue template must not fabricate recorded evidence")
    require(decision["status"] == "pending" and decision["decision"] == "hold", "release decision template must default to hold")
    require(cohort["rawIdentityDataStored"] is False and cohort["openChatEnabled"] is False and cohort["realMoneyOrTokenEarningEnabled"] is False, "cohort template safety defaults invalid")
    require(session["privacy"]["containsRawIdentity"] is False and session["privacy"]["containsChildPii"] is False, "session template privacy defaults invalid")
    require(issue["containsChildPii"] is False and issue["containsRawIdentity"] is False, "issue template privacy defaults invalid")

    contains(ASSESSOR, [
        "Production Alpha must be CERTIFIED",
        "Open P1 issue blocks Release Candidate promotion",
        "Save corruption observed during External Alpha",
        "Confirmed child-safety incident requires rollback",
        "Telemetry tampering must block External Alpha promotion",
        "Unsafe open chat must block External Alpha promotion",
    ])
    contains(RUNNER, ["external-alpha-readiness.json", "NON_CERTIFYING_PASS", "origin/main", "production-alpha-readiness.json"])
    contains(WORKFLOW, ["External Alpha Playtest", "--self-test", "validate-external-alpha-playtest.py", "Parser]::ParseFile"])
    contains(DOC, ["External Alpha", "Release Candidate", "Win64", "Android", "iPadOS", "rollback", "child"])
    contains(LAUNCHER, ["run-external-alpha-playtest-readiness.ps1"])

    print("External Alpha source validation PASS: cohort, session, issue, safety, SLO and RC-promotion contracts are coherent and fail closed pending real playtest evidence.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
