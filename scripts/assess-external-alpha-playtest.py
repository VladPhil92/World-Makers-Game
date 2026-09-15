#!/usr/bin/env python3
"""Fail-closed External Alpha playtest readiness assessor."""
from __future__ import annotations

import argparse
import hashlib
import json
import tempfile
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "content/production/external-alpha-playtest-v1.json"


def load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def is_sha256(value: object) -> bool:
    return isinstance(value, str) and len(value) == 64 and all(c in "0123456789abcdef" for c in value)


def is_commit(value: object) -> bool:
    return isinstance(value, str) and len(value) == 40 and all(c in "0123456789abcdef" for c in value)


def safe_payload(root: Path, rel: object) -> Path | None:
    if not isinstance(rel, str) or not rel or Path(rel).is_absolute() or ".." in Path(rel).parts:
        return None
    resolved_root = root.resolve()
    candidate = (root / rel).resolve()
    if candidate != resolved_root and resolved_root not in candidate.parents:
        return None
    return candidate


def reason(result: dict, message: str) -> None:
    result["reasons"].append(message)


def validate_production_alpha(path: Path, commit: str, result: dict) -> bool:
    if not path.is_file():
        reason(result, "Production Alpha result is missing")
        return False
    try:
        payload = load_json(path)
    except Exception as exc:
        reason(result, f"Production Alpha result is invalid JSON: {exc}")
        return False
    ok = True
    if payload.get("status") != "CERTIFIED" or payload.get("certified") is not True:
        ok = False
        reason(result, "Production Alpha must be CERTIFIED")
    if payload.get("repositoryCommit") != commit:
        ok = False
        reason(result, "Production Alpha result must match the External Alpha build commit")
    result["productionAlphaCertifiedSameCommit"] = ok
    return ok


def validate_cohort(path: Path, commit: str, contract: dict, result: dict) -> bool:
    if not path.is_file():
        reason(result, "External Alpha cohort approval is missing")
        return False
    try:
        cohort = load_json(path)
    except Exception as exc:
        reason(result, f"Cohort file is invalid JSON: {exc}")
        return False
    problems: list[str] = []
    if cohort.get("schema") != "worldmakers.external-alpha-cohort.v1":
        problems.append("cohort schema mismatch")
    if cohort.get("status") != "approved":
        problems.append("cohort status must be approved")
    if cohort.get("buildCommit") != commit:
        problems.append("cohort buildCommit mismatch")
    if not isinstance(cohort.get("cohortId"), str) or not cohort.get("cohortId").strip():
        problems.append("cohortId is required")
    floor = int(contract["cohort"]["minimumUniqueTesters"])
    planned = cohort.get("plannedUniqueTesters")
    if not isinstance(planned, int) or planned < floor:
        problems.append(f"plannedUniqueTesters must be at least {floor}")
    minimums = cohort.get("platformMinimums") or {}
    required_minimums = contract["cohort"]["minimumSessionsByPlatform"]
    for platform, minimum in required_minimums.items():
        value = minimums.get(platform)
        if not isinstance(value, int) or value < int(minimum):
            problems.append(f"platformMinimums.{platform} must be at least {minimum}")
    if cohort.get("privacyReviewApproved") is not True:
        problems.append("privacy review must be approved")
    if cohort.get("childSafetyReviewApproved") is not True:
        problems.append("child-safety review must be approved")
    if cohort.get("rawIdentityDataStored") is not False:
        problems.append("raw identity data is not allowed")
    if cohort.get("openChatEnabled") is not False:
        problems.append("open chat must remain disabled")
    if cohort.get("realMoneyOrTokenEarningEnabled") is not False:
        problems.append("real-money/token earning must remain disabled")
    if cohort.get("feedbackRequestsChildPii") is not False:
        problems.append("feedback must not request child PII")
    if cohort.get("minorTestingEnabled") is True and cohort.get("guardianOrAuthorizedAdultFlowVerified") is not True:
        problems.append("minor testing requires verified guardian/authorized-adult flow")
    for item in problems:
        reason(result, "Cohort: " + item)
    result["cohortApproved"] = not problems
    return not problems


def validate_session(path: Path, root: Path, commit: str, contract: dict):
    problems: list[str] = []
    try:
        data = load_json(path)
    except Exception as exc:
        return False, [f"{path.name}: invalid JSON: {exc}"], None

    if data.get("schema") != "worldmakers.external-alpha-session.v1":
        problems.append(f"{path.name}: schema mismatch")
    if data.get("status") != "recorded":
        problems.append(f"{path.name}: status must be recorded")
    if data.get("buildCommit") != commit:
        problems.append(f"{path.name}: buildCommit mismatch")
    session_id = data.get("sessionId")
    if not isinstance(session_id, str) or not session_id.strip():
        problems.append(f"{path.name}: sessionId is required")
    tester_hash = data.get("testerIdHash")
    if not is_sha256(tester_hash):
        problems.append(f"{path.name}: testerIdHash must be a lowercase SHA-256")
    participant = data.get("participantClass")
    if participant not in {"adult", "minor-authorized"}:
        problems.append(f"{path.name}: participantClass invalid")
    guardian_hash = data.get("guardianAuthorizationRefHash", "")
    if participant == "minor-authorized" and not is_sha256(guardian_hash):
        problems.append(f"{path.name}: minor session requires hashed guardian authorization reference")
    if participant == "adult" and guardian_hash not in {"", None} and not is_sha256(guardian_hash):
        problems.append(f"{path.name}: guardianAuthorizationRefHash must be blank or hashed")
    platform = data.get("platform")
    if platform not in set(contract["requiredPlatforms"]):
        problems.append(f"{path.name}: unsupported platform")
    if not isinstance(data.get("buildId"), str) or not data.get("buildId").strip():
        problems.append(f"{path.name}: buildId is required")
    duration = data.get("durationMinutes")
    if not isinstance(duration, (int, float)) or float(duration) < 0:
        problems.append(f"{path.name}: durationMinutes invalid")
    outcome = data.get("outcome")
    if outcome not in {"completed", "aborted-crash", "aborted-blocking", "aborted-other"}:
        problems.append(f"{path.name}: outcome invalid")
    if outcome == "completed" and isinstance(duration, (int, float)) and float(duration) < float(contract["cohort"]["minimumSessionDurationMinutesForCompletion"]):
        problems.append(f"{path.name}: completed session is shorter than minimum duration")

    telemetry = safe_payload(root, data.get("telemetryFile"))
    if telemetry is None or not telemetry.is_file():
        problems.append(f"{path.name}: telemetry payload missing or unsafe")
    else:
        expected_hash = data.get("telemetrySha256")
        if not is_sha256(expected_hash) or sha256_file(telemetry) != expected_hash:
            problems.append(f"{path.name}: telemetry SHA-256 mismatch")

    checks = data.get("checks") or {}
    if outcome == "completed":
        for key in ("installLaunchPassed", "certificationMapLoaded", "firstPersonControlPassed", "observeScanPassed", "scienceInteractionPassed", "buildPlacePassed"):
            if checks.get(key) is not True:
                problems.append(f"{path.name}: completed session requires checks.{key}=true")
    if platform in {"Android", "iPadOS"} and checks.get("backgroundResumeApplicable") is not True:
        problems.append(f"{path.name}: mobile session must mark background/resume applicable")

    reliability = data.get("reliability") or {}
    for key in ("crashCount", "fatalErrorCount"):
        value = reliability.get(key)
        if not isinstance(value, int) or value < 0:
            problems.append(f"{path.name}: reliability.{key} invalid")
    privacy = data.get("privacy") or {}
    for key in ("containsRawIdentity", "containsChildPii", "containsFreeTextChildData"):
        if privacy.get(key) is not False:
            problems.append(f"{path.name}: privacy.{key} must be false")

    summary = {
        "sessionId": session_id,
        "testerIdHash": tester_hash,
        "platform": platform,
        "outcome": outcome,
        "coreJourneyCompleted": checks.get("coreJourneyCompleted") is True,
        "saveReloadPassed": checks.get("saveReloadPassed") is True,
        "backgroundResumeApplicable": checks.get("backgroundResumeApplicable") is True,
        "backgroundResumePassed": checks.get("backgroundResumePassed") is True,
        "crashCount": reliability.get("crashCount", 0),
        "fatalErrorCount": reliability.get("fatalErrorCount", 0),
        "blockingIssueEncountered": reliability.get("blockingIssueEncountered") is True,
        "saveCorruptionObserved": reliability.get("saveCorruptionObserved") is True,
    }
    return not problems, problems, summary


def validate_issue(path: Path, commit: str, contract: dict):
    problems: list[str] = []
    try:
        data = load_json(path)
    except Exception as exc:
        return False, [f"{path.name}: invalid JSON: {exc}"], None
    if data.get("schema") != "worldmakers.external-alpha-issue.v1":
        problems.append(f"{path.name}: schema mismatch")
    if data.get("status") != "recorded":
        problems.append(f"{path.name}: status must be recorded")
    if data.get("buildCommit") != commit:
        problems.append(f"{path.name}: buildCommit mismatch")
    if not isinstance(data.get("issueId"), str) or not data.get("issueId").strip():
        problems.append(f"{path.name}: issueId is required")
    severity = data.get("severity")
    if severity not in set(contract["issuePolicy"]["severities"]):
        problems.append(f"{path.name}: invalid severity")
    category = data.get("category")
    if category not in {"crash", "gameplay", "save", "ux", "performance", "safety", "content", "network"}:
        problems.append(f"{path.name}: invalid category")
    state = data.get("state")
    if state not in {"open", "resolved", "verified"}:
        problems.append(f"{path.name}: invalid state")
    source_hash = data.get("sourceSessionIdHash", "")
    if source_hash not in {"", None} and not is_sha256(source_hash):
        problems.append(f"{path.name}: sourceSessionIdHash must be blank or SHA-256")
    for key in ("containsChildPii", "containsRawIdentity", "containsFreeTextChildData"):
        if data.get(key) is not False:
            problems.append(f"{path.name}: {key} must be false")
    if state == "verified" and data.get("resolutionVerified") is not True:
        problems.append(f"{path.name}: verified issue must have resolutionVerified=true")
    summary = {
        "issueId": data.get("issueId"),
        "severity": severity,
        "category": category,
        "state": state,
        "saveCorruption": data.get("saveCorruption") is True,
        "confirmedChildSafetyIncident": data.get("confirmedChildSafetyIncident") is True,
    }
    return not problems, problems, summary


def validate_decision(path: Path, commit: str, result: dict) -> bool:
    if not path.is_file():
        reason(result, "Release decision is missing")
        return False
    try:
        data = load_json(path)
    except Exception as exc:
        reason(result, f"Release decision is invalid JSON: {exc}")
        return False
    problems: list[str] = []
    if data.get("schema") != "worldmakers.external-alpha-release-decision.v1":
        problems.append("release decision schema mismatch")
    if data.get("status") != "approved":
        problems.append("release decision status must be approved")
    if data.get("buildCommit") != commit:
        problems.append("release decision buildCommit mismatch")
    if data.get("decision") != "promote-to-release-candidate":
        problems.append("decision must be promote-to-release-candidate")
    for key in ("qaApproved", "childSafetyApproved", "productApproved", "telemetryReviewed", "rollbackPlanVerified"):
        if data.get(key) is not True:
            problems.append(f"{key} must be true")
    for key in ("knownBlockingIssues", "confirmedChildSafetyIncidents", "saveCorruptionIncidents"):
        if data.get(key) != 0:
            problems.append(f"{key} must be zero")
    for item in problems:
        reason(result, "Release decision: " + item)
    result["humanPromotionApproved"] = not problems
    return not problems


def assess(evidence_root: Path, production_alpha_result: Path, expected_commit: str) -> dict:
    contract = load_json(CONTRACT)
    result = {
        "schema": "worldmakers.external-alpha-readiness-result.v1",
        "status": "BLOCKED",
        "certified": False,
        "repositoryCommit": expected_commit,
        "productionAlphaCertifiedSameCommit": False,
        "cohortApproved": False,
        "humanPromotionApproved": False,
        "metrics": {},
        "issueSummary": {},
        "rollbackRequired": False,
        "reasons": [],
    }
    if not is_commit(expected_commit):
        reason(result, "Expected commit must be a full lowercase SHA")
        return result

    production_ok = validate_production_alpha(production_alpha_result, expected_commit, result)
    cohort_ok = validate_cohort(evidence_root / "cohort.json", expected_commit, contract, result)

    sessions_dir = evidence_root / "sessions"
    if not sessions_dir.is_dir():
        reason(result, "Session evidence directory does not exist")
        return result
    session_files = sorted(sessions_dir.glob("*.json"))
    sessions: list[dict] = []
    session_ids: list[str] = []
    all_sessions_valid = True
    for path in session_files:
        valid, problems, summary = validate_session(path, sessions_dir, expected_commit, contract)
        if not valid:
            all_sessions_valid = False
            result["reasons"].extend(problems)
        if summary:
            sessions.append(summary)
            if summary.get("sessionId"):
                session_ids.append(summary["sessionId"])
    if len(session_ids) != len(set(session_ids)):
        all_sessions_valid = False
        reason(result, "sessionId values must be unique")

    total = len(sessions)
    completed = [s for s in sessions if s["outcome"] == "completed"]
    unique_testers = {s["testerIdHash"] for s in sessions if is_sha256(s.get("testerIdHash"))}
    per_platform = Counter(s["platform"] for s in sessions if s.get("platform"))
    crash_free = sum(1 for s in sessions if int(s.get("crashCount", 0)) == 0)
    fatal_free = sum(1 for s in sessions if int(s.get("fatalErrorCount", 0)) == 0)
    core_completed = sum(1 for s in completed if s["coreJourneyCompleted"])
    save_passed = sum(1 for s in completed if s["saveReloadPassed"])
    mobile = [s for s in sessions if s["platform"] in {"Android", "iPadOS"}]
    mobile_resume_passed = sum(1 for s in mobile if s["backgroundResumeApplicable"] and s["backgroundResumePassed"])

    def rate(numerator: int, denominator: int) -> float:
        return 0.0 if denominator == 0 else numerator / denominator

    metrics = {
        "totalSessions": total,
        "completedSessions": len(completed),
        "uniqueTesters": len(unique_testers),
        "sessionsByPlatform": dict(sorted(per_platform.items())),
        "sessionCompletionRate": rate(len(completed), total),
        "crashFreeSessionRate": rate(crash_free, total),
        "fatalErrorFreeSessionRate": rate(fatal_free, total),
        "coreJourneyCompletionRate": rate(core_completed, len(completed)),
        "saveReloadSuccessRate": rate(save_passed, len(completed)),
        "mobileBackgroundResumeSuccessRate": rate(mobile_resume_passed, len(mobile)),
    }
    result["metrics"] = metrics

    cohort_contract = contract["cohort"]
    slo = contract["experienceSlo"]
    threshold_checks = [
        (total >= int(cohort_contract["minimumTotalSessions"]), "Not enough External Alpha sessions"),
        (len(completed) >= int(cohort_contract["minimumCompletedSessions"]), "Not enough completed External Alpha sessions"),
        (len(unique_testers) >= int(cohort_contract["minimumUniqueTesters"]), "Not enough unique External Alpha testers"),
        (metrics["sessionCompletionRate"] >= float(slo["minimumSessionCompletionRate"]), "Session completion rate below SLO"),
        (metrics["crashFreeSessionRate"] >= float(slo["minimumCrashFreeSessionRate"]), "Crash-free session rate below SLO"),
        (metrics["fatalErrorFreeSessionRate"] >= float(slo["minimumFatalErrorFreeSessionRate"]), "Fatal-error-free session rate below SLO"),
        (metrics["coreJourneyCompletionRate"] >= float(slo["minimumCoreJourneyCompletionRate"]), "Core journey completion rate below SLO"),
        (metrics["saveReloadSuccessRate"] >= float(slo["minimumSaveReloadSuccessRate"]), "Save/reload success rate below SLO"),
        (metrics["mobileBackgroundResumeSuccessRate"] >= float(slo["minimumMobileBackgroundResumeSuccessRate"]), "Mobile background/resume success rate below SLO"),
    ]
    for platform, minimum in cohort_contract["minimumSessionsByPlatform"].items():
        threshold_checks.append((per_platform.get(platform, 0) >= int(minimum), f"Not enough sessions for {platform}"))
    for passed, message in threshold_checks:
        if not passed:
            reason(result, message)

    if any(s["saveCorruptionObserved"] for s in sessions):
        result["rollbackRequired"] = True
        reason(result, "Save corruption observed during External Alpha")
    if any(s["blockingIssueEncountered"] for s in sessions):
        reason(result, "A blocking issue was encountered on the candidate build")

    issues_dir = evidence_root / "issues"
    if not issues_dir.is_dir():
        reason(result, "Issue evidence directory does not exist")
        return result
    issues: list[dict] = []
    issue_ids: list[str] = []
    all_issues_valid = True
    for path in sorted(issues_dir.glob("*.json")):
        valid, problems, summary = validate_issue(path, expected_commit, contract)
        if not valid:
            all_issues_valid = False
            result["reasons"].extend(problems)
        if summary:
            issues.append(summary)
            if summary.get("issueId"):
                issue_ids.append(summary["issueId"])
    if len(issue_ids) != len(set(issue_ids)):
        all_issues_valid = False
        reason(result, "issueId values must be unique")

    open_issues = [i for i in issues if i["state"] == "open"]
    open_by_severity = Counter(i["severity"] for i in open_issues)
    open_safety = sum(1 for i in open_issues if i["category"] == "safety")
    result["issueSummary"] = {
        "total": len(issues),
        "openBySeverity": dict(sorted(open_by_severity.items())),
        "openSafetyIssues": open_safety,
    }
    policy = contract["issuePolicy"]
    if open_by_severity.get("P0", 0) > int(policy["openP0Allowed"]):
        reason(result, "Open P0 issue blocks Release Candidate promotion")
    if open_by_severity.get("P1", 0) > int(policy["openP1Allowed"]):
        reason(result, "Open P1 issue blocks Release Candidate promotion")
    if open_by_severity.get("P2", 0) > int(policy["maxOpenP2"]):
        reason(result, "Too many open P2 issues")
    if open_safety > int(policy["openSafetyIssuesAllowed"]):
        reason(result, "Open safety issue blocks Release Candidate promotion")
    if any(i["saveCorruption"] for i in issues):
        result["rollbackRequired"] = True
        reason(result, "Save-corruption issue requires a new build and rollback")
    if any(i["confirmedChildSafetyIncident"] for i in issues):
        result["rollbackRequired"] = True
        reason(result, "Confirmed child-safety incident requires rollback")

    decision_ok = validate_decision(evidence_root / "release-decision.json", expected_commit, result)

    evidence_ok = all_sessions_valid and all_issues_valid
    no_threshold_failures = not any(msg for msg in result["reasons"] if msg.startswith((
        "Not enough", "Session completion", "Crash-free", "Fatal-error-free", "Core journey", "Save/reload", "Mobile background/resume"
    )))
    issue_gate_ok = (
        open_by_severity.get("P0", 0) <= int(policy["openP0Allowed"])
        and open_by_severity.get("P1", 0) <= int(policy["openP1Allowed"])
        and open_by_severity.get("P2", 0) <= int(policy["maxOpenP2"])
        and open_safety <= int(policy["openSafetyIssuesAllowed"])
        and not result["rollbackRequired"]
        and not any(s["blockingIssueEncountered"] for s in sessions)
    )

    certified = production_ok and cohort_ok and evidence_ok and no_threshold_failures and issue_gate_ok and decision_ok
    result["certified"] = certified
    result["status"] = "CERTIFIED" if certified else "BLOCKED"
    if certified:
        result["nextPhase"] = contract["exit"]["nextPhase"]
    return result


def write_json(path: Path, payload: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2), encoding="utf-8")


def synthetic_session(root: Path, index: int, platform: str, commit: str) -> None:
    telemetry_rel = f"telemetry/session-{index}.json"
    telemetry = root / telemetry_rel
    telemetry.parent.mkdir(parents=True, exist_ok=True)
    telemetry.write_text(json.dumps({"session": index, "platform": platform}), encoding="utf-8")
    payload = {
        "schema": "worldmakers.external-alpha-session.v1",
        "status": "recorded",
        "sessionId": f"session-{index}",
        "testerIdHash": hashlib.sha256(f"tester-{index % 10}".encode()).hexdigest(),
        "participantClass": "adult",
        "guardianAuthorizationRefHash": "",
        "platform": platform,
        "buildCommit": commit,
        "buildId": f"wm-alpha-{commit[:12]}-{platform.lower()}",
        "startedAtUtc": "2026-09-15T00:00:00Z",
        "endedAtUtc": "2026-09-15T00:20:00Z",
        "durationMinutes": 20,
        "outcome": "completed",
        "telemetryFile": telemetry_rel,
        "telemetrySha256": sha256_file(telemetry),
        "checks": {
            "installLaunchPassed": True,
            "certificationMapLoaded": True,
            "firstPersonControlPassed": True,
            "observeScanPassed": True,
            "scienceInteractionPassed": True,
            "buildPlacePassed": True,
            "coreJourneyCompleted": True,
            "saveReloadPassed": True,
            "backgroundResumeApplicable": platform in {"Android", "iPadOS"},
            "backgroundResumePassed": platform in {"Android", "iPadOS"},
        },
        "reliability": {
            "crashCount": 0,
            "fatalErrorCount": 0,
            "blockingIssueEncountered": False,
            "saveCorruptionObserved": False,
        },
        "privacy": {
            "containsRawIdentity": False,
            "containsChildPii": False,
            "containsFreeTextChildData": False,
        },
    }
    write_json(root / f"session-{index}.json", payload)


def self_test() -> int:
    commit = "1" * 40
    with tempfile.TemporaryDirectory() as temp:
        base = Path(temp)
        evidence = base / "external-alpha"
        sessions = evidence / "sessions"
        issues = evidence / "issues"
        issues.mkdir(parents=True)
        prod = base / "production-alpha.json"
        write_json(prod, {"status": "CERTIFIED", "certified": True, "repositoryCommit": commit})
        write_json(evidence / "cohort.json", {
            "schema": "worldmakers.external-alpha-cohort.v1",
            "status": "approved",
            "buildCommit": commit,
            "cohortId": "cohort-a",
            "plannedUniqueTesters": 10,
            "platformMinimums": {"Win64": 4, "Android": 6, "iPadOS": 6},
            "minorTestingEnabled": False,
            "guardianOrAuthorizedAdultFlowVerified": False,
            "privacyReviewApproved": True,
            "childSafetyReviewApproved": True,
            "rawIdentityDataStored": False,
            "openChatEnabled": False,
            "realMoneyOrTokenEarningEnabled": False,
            "feedbackRequestsChildPii": False,
        })
        platforms = ["Win64"] * 4 + ["Android"] * 8 + ["iPadOS"] * 8
        for idx, platform in enumerate(platforms):
            synthetic_session(sessions, idx, platform, commit)
        write_json(evidence / "release-decision.json", {
            "schema": "worldmakers.external-alpha-release-decision.v1",
            "status": "approved",
            "buildCommit": commit,
            "decision": "promote-to-release-candidate",
            "qaApproved": True,
            "childSafetyApproved": True,
            "productApproved": True,
            "telemetryReviewed": True,
            "rollbackPlanVerified": True,
            "knownBlockingIssues": 0,
            "confirmedChildSafetyIncidents": 0,
            "saveCorruptionIncidents": 0,
        })
        good = assess(evidence, prod, commit)
        if good["status"] != "CERTIFIED":
            raise SystemExit("Complete synthetic External Alpha evidence should certify: " + "; ".join(good["reasons"]))

        write_json(issues / "p1.json", {
            "schema": "worldmakers.external-alpha-issue.v1",
            "status": "recorded",
            "issueId": "P1-1",
            "buildCommit": commit,
            "severity": "P1",
            "category": "gameplay",
            "state": "open",
            "sourceSessionIdHash": "",
            "reproducible": True,
            "saveCorruption": False,
            "confirmedChildSafetyIncident": False,
            "containsChildPii": False,
            "containsRawIdentity": False,
            "containsFreeTextChildData": False,
            "resolutionVerified": False,
        })
        if assess(evidence, prod, commit)["status"] != "BLOCKED":
            raise SystemExit("Open P1 must block External Alpha promotion")
        (issues / "p1.json").unlink()

        cohort = load_json(evidence / "cohort.json")
        cohort["openChatEnabled"] = True
        write_json(evidence / "cohort.json", cohort)
        if assess(evidence, prod, commit)["status"] != "BLOCKED":
            raise SystemExit("Unsafe open chat must block External Alpha promotion")
        cohort["openChatEnabled"] = False
        write_json(evidence / "cohort.json", cohort)

        first_session = sessions / "session-0.json"
        session_payload = load_json(first_session)
        telemetry = sessions / session_payload["telemetryFile"]
        telemetry.write_text("tampered", encoding="utf-8")
        if assess(evidence, prod, commit)["status"] != "BLOCKED":
            raise SystemExit("Telemetry tampering must block External Alpha promotion")

    print("External Alpha assessor self-test passed: complete evidence certifies; P1, unsafe chat and telemetry tampering fail closed.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-root", type=Path)
    parser.add_argument("--production-alpha-result", type=Path)
    parser.add_argument("--expected-commit")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if not all((args.evidence_root, args.production_alpha_result, args.expected_commit, args.output)):
        parser.error("--evidence-root, --production-alpha-result, --expected-commit and --output are required")
    result = assess(args.evidence_root, args.production_alpha_result, args.expected_commit)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps({"status": result["status"], "certified": result["certified"], "reasons": result["reasons"]}, indent=2))
    return 0 if result["status"] == "CERTIFIED" else 1


if __name__ == "__main__":
    raise SystemExit(main())
