#!/usr/bin/env python3
"""Fail-closed Production Release readiness assessor."""
from __future__ import annotations
import argparse, json, re, tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTRACT_PATH = ROOT / "content/production/production-release-readiness-v1.json"
TARGETS = {"windows-reference", "android-alpha", "ipados-alpha"}
MOBILE = {"android-alpha", "ipados-alpha"}
RC_RE = re.compile(r"^(\d+\.\d+\.\d+)-rc\.\d+$")
SEMVER_RE = re.compile(r"^\d+\.\d+\.\d+$")
HEX64_RE = re.compile(r"^[0-9a-f]{64}$")


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def write(path: Path, payload: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def full_sha(v: object) -> bool:
    return isinstance(v, str) and len(v) == 40 and all(c in "0123456789abcdef" for c in v)


def hex64(v: object) -> bool:
    return isinstance(v, str) and HEX64_RE.fullmatch(v) is not None


def add(r: dict, msg: str) -> None:
    r["reasons"].append(msg)


def validate_rc(result_path: Path, freeze_path: Path, commit: str, result: dict):
    try:
        rc, freeze = load(result_path), load(freeze_path)
    except Exception as exc:
        add(result, f"Release Candidate dependency evidence is missing or invalid: {exc}")
        return False, None, None
    ok = True
    if rc.get("status") != "CERTIFIED" or rc.get("certified") is not True:
        add(result, "Release Candidate must be CERTIFIED before Production Release readiness can certify"); ok = False
    if rc.get("repositoryCommit") != commit:
        add(result, "Release Candidate result must reference the exact production release commit"); ok = False
    version = rc.get("releaseVersion")
    match = RC_RE.fullmatch(version or "")
    if not match:
        add(result, "Release Candidate result must contain x.y.z-rc.n releaseVersion"); ok = False
        public = None
    else:
        public = match.group(1)
    if freeze.get("schema") != "worldmakers.rc-freeze-manifest.v1" or freeze.get("status") != "frozen" or freeze.get("repositoryCommit") != commit or freeze.get("releaseVersion") != version:
        add(result, "RC freeze manifest does not match the certified candidate"); ok = False
    rows = freeze.get("targets") or []
    target_map = {row.get("targetId"): row for row in rows if isinstance(row, dict)}
    if set(target_map) != TARGETS:
        add(result, "RC freeze manifest must contain exactly three canonical targets"); ok = False
    for tid, row in target_map.items():
        if not hex64(row.get("payloadSha256")):
            add(result, f"RC target {tid} lacks a valid payload SHA-256"); ok = False
    result["releaseCandidateCertifiedSameCommit"] = ok
    return ok, public, target_map


def validate_manifest(path: Path, commit: str, public: str | None, rc_version: str | None, rc_targets: dict | None, result: dict):
    try: data = load(path)
    except Exception as exc:
        add(result, f"Production release manifest is missing or invalid: {exc}"); return False, None
    ok = True
    if data.get("schema") != "worldmakers.production-release-manifest.v1" or data.get("status") != "prepared": add(result, "Production release manifest must be status=prepared"); ok = False
    if data.get("repositoryCommit") != commit or data.get("rcVersion") != rc_version or data.get("publicVersion") != public: add(result, "Production release manifest provenance/version mismatch"); ok = False
    if not data.get("preparedAtUtc") or not data.get("preparedBy"): add(result, "Production release manifest provenance is incomplete"); ok = False
    rows = data.get("targets") or []
    m = {row.get("targetId"): row for row in rows if isinstance(row, dict)}
    if set(m) != TARGETS: add(result, "Production release manifest requires exactly three canonical targets"); ok = False
    if rc_targets:
        for tid in TARGETS:
            actual = (m.get(tid) or {}).get("payloadSha256")
            expected = rc_targets[tid].get("payloadSha256")
            if actual != expected or not hex64(actual): add(result, f"Immutable promotion failed for {tid}: payload SHA-256 differs from RC"); ok = False
    return ok, m


def validate_distribution(path: Path, commit: str, version: str | None, manifest: dict | None, result: dict):
    try: data = load(path)
    except Exception as exc: add(result, f"Distribution evidence is missing or invalid: {exc}"); return False
    ok = True
    if data.get("schema") != "worldmakers.production-distribution.v1" or data.get("status") != "ready": add(result, "Distribution evidence must be status=ready"); ok = False
    if data.get("repositoryCommit") != commit or data.get("publicVersion") != version or not data.get("reviewedAtUtc") or not data.get("reviewer"): add(result, "Distribution evidence provenance is incomplete"); ok = False
    if data.get("credentialsStoredInRepository") is not False: add(result, "Distribution credentials must not be stored in the repository"); ok = False
    expected_provider = {"windows-reference":"managed-windows-production","android-alpha":"google-play-production","ipados-alpha":"apple-app-store-production"}
    rows = data.get("targets") or []; seen = set()
    for row in rows:
        tid = row.get("targetId"); seen.add(tid)
        if tid not in TARGETS: add(result, f"Unexpected distribution target {tid}"); ok = False; continue
        if row.get("provider") != expected_provider[tid] or not row.get("channelReference"): add(result, f"Distribution channel incomplete for {tid}"); ok = False
        if manifest and row.get("payloadSha256") != manifest[tid].get("payloadSha256"): add(result, f"Distribution payload hash mismatch for {tid}"); ok = False
        for key in ("listingApproved","privacyDisclosureReviewed","ageRatingReviewed","supportContactConfigured"):
            if row.get(key) is not True: add(result, f"Distribution readiness {key} failed for {tid}"); ok = False
        if tid in MOBILE and row.get("signingVerified") is not True: add(result, f"Mobile signing must remain verified for {tid}"); ok = False
    if seen != TARGETS or len(rows) != 3: add(result, "Distribution evidence requires exactly three canonical targets"); ok = False
    return ok


def validate_rollout(path: Path, commit: str, version: str | None, contract: dict, result: dict):
    try: data = load(path)
    except Exception as exc: add(result, f"Rollout plan is missing or invalid: {exc}"); return False
    ok = True
    if data.get("schema") != "worldmakers.production-rollout-plan.v1" or data.get("status") != "approved": add(result, "Rollout plan must be status=approved"); ok = False
    if data.get("repositoryCommit") != commit or data.get("publicVersion") != version or not data.get("approvedAtUtc") or not data.get("approvedBy") or not data.get("rollbackAuthority"): add(result, "Rollout plan provenance/authority is incomplete"); ok = False
    expected = list(zip(contract["rollout"]["stagesPercent"], contract["rollout"]["minimumObservationHours"]))
    actual = [(x.get("percent"),x.get("minimumObservationHours")) for x in (data.get("stages") or []) if isinstance(x,dict)]
    if actual != expected: add(result, "Rollout stages or observation windows differ from contract"); ok = False
    if data.get("manualPromotionBetweenStages") is not True: add(result, "Manual promotion between rollout stages is required"); ok = False
    if set(data.get("haltTriggers") or []) != set(contract["rollout"]["haltTriggers"]): add(result, "Rollout halt triggers must match contract"); ok = False
    return ok


def validate_operations(path: Path, commit: str, version: str | None, result: dict):
    try: data = load(path)
    except Exception as exc: add(result, f"Production operations evidence is missing or invalid: {exc}"); return False
    ok = True
    if data.get("schema") != "worldmakers.production-operations.v1" or data.get("status") != "passed": add(result, "Production operations evidence must be status=passed"); ok = False
    if data.get("repositoryCommit") != commit or data.get("publicVersion") != version or not data.get("reviewedAtUtc") or not data.get("reviewer"): add(result, "Production operations provenance is incomplete"); ok = False
    for key in ("dashboardReady","alertRoutingReady","crashSymbolsReady","telemetryCoverageVerified"):
        if (data.get("health") or {}).get(key) is not True: add(result, f"Production health requirement not met: {key}"); ok = False
    rb = data.get("rollback") or {}
    for key in ("drillPassed","withdrawalProcedureVerified"):
        if rb.get(key) is not True: add(result, f"Rollback requirement not met: {key}"); ok = False
    if not rb.get("rollbackTargetVersion") or not hex64(rb.get("rollbackTargetSha256")) or not rb.get("authority"): add(result, "Rollback target, SHA-256 and authority are required"); ok = False
    ir = data.get("incidentResponse") or {}
    if not ir.get("incidentCommander") or ir.get("supportEscalationReady") is not True: add(result, "Incident response/support escalation is incomplete"); ok = False
    for key in ("privacyPolicyReviewed","termsReviewed","childSafetyReviewed","storeDataSafetyReviewed"):
        if (data.get("compliance") or {}).get(key) is not True: add(result, f"Compliance requirement not met: {key}"); ok = False
    safety = data.get("safety") or {}
    if safety.get("childPiiCollected") is not False or safety.get("openChatEnabled") is not False or safety.get("realMoneyOrTokenEarningEnabled") is not False: add(result, "Production child-safety boundary failed"); ok = False
    return ok


def validate_decision(path: Path, commit: str, version: str | None, result: dict):
    try: data = load(path)
    except Exception as exc: add(result, f"Production release decision is missing or invalid: {exc}"); return False
    ok = True
    if data.get("schema") != "worldmakers.production-release-decision.v1" or data.get("status") != "approved" or data.get("decision") != "LAUNCH_APPROVED": add(result, "Production decision must be approved LAUNCH_APPROVED"); ok = False
    if data.get("repositoryCommit") != commit or data.get("publicVersion") != version or not data.get("decidedAtUtc"): add(result, "Production release decision provenance is incomplete"); ok = False
    approvals, reviewers = data.get("approvals") or {}, data.get("reviewers") or {}
    for role in ("qa","engineering","childSafety","product","privacyLegal","releaseOperations"):
        if approvals.get(role) is not True or not reviewers.get(role): add(result, f"Production launch requires explicit {role} approval and reviewer"); ok = False
    if data.get("knownBlockingIssues") != []: add(result, "Production launch cannot contain known blocking issues"); ok = False
    return ok


def assess(root: Path, rc_result: Path, rc_freeze: Path, commit: str) -> dict:
    contract = load(CONTRACT_PATH)
    result = {"schema":"worldmakers.production-release-readiness-result.v1","status":"BLOCKED","certified":False,"repositoryCommit":commit,"publicVersion":None,"releaseCandidateCertifiedSameCommit":False,"checks":{"manifest":False,"distribution":False,"rollout":False,"operations":False,"decision":False},"reasons":[],"nextPhase":None}
    if not full_sha(commit): add(result, "Expected commit must be a full lowercase SHA"); return result
    rc_ok, public, rc_targets = validate_rc(rc_result, rc_freeze, commit, result)
    result["publicVersion"] = public
    rc_version = None
    try: rc_version = load(rc_result).get("releaseVersion")
    except Exception: pass
    manifest_ok, manifest = validate_manifest(root/"release-manifest.json", commit, public, rc_version, rc_targets, result)
    distribution_ok = validate_distribution(root/"distribution.json", commit, public, manifest, result)
    rollout_ok = validate_rollout(root/"rollout-plan.json", commit, public, contract, result)
    operations_ok = validate_operations(root/"operations.json", commit, public, result)
    decision_ok = validate_decision(root/"release-decision.json", commit, public, result)
    result["checks"].update({"manifest":manifest_ok,"distribution":distribution_ok,"rollout":rollout_ok,"operations":operations_ok,"decision":decision_ok})
    certified = rc_ok and manifest_ok and distribution_ok and rollout_ok and operations_ok and decision_ok and not result["reasons"]
    result["certified"] = certified; result["status"] = "CERTIFIED" if certified else "BLOCKED"; result["nextPhase"] = contract["exit"]["nextPhase"] if certified else None
    return result


def self_test() -> int:
    commit = "1"*40; digest = "a"*64; rollback = "b"*64; rc_version="0.1.0-rc.1"; public="0.1.0"
    with tempfile.TemporaryDirectory() as td:
        r=Path(td); rc=r/"rc-result.json"; freeze=r/"rc-freeze.json"
        write(rc,{"status":"CERTIFIED","certified":True,"repositoryCommit":commit,"releaseVersion":rc_version})
        targets=[{"targetId":t,"payloadSha256":digest} for t in sorted(TARGETS)]
        write(freeze,{"schema":"worldmakers.rc-freeze-manifest.v1","status":"frozen","repositoryCommit":commit,"releaseVersion":rc_version,"targets":targets})
        write(r/"release-manifest.json",{"schema":"worldmakers.production-release-manifest.v1","status":"prepared","repositoryCommit":commit,"rcVersion":rc_version,"publicVersion":public,"preparedAtUtc":"2026-01-01T00:00:00Z","preparedBy":"self-test","targets":targets})
        providers={"windows-reference":"managed-windows-production","android-alpha":"google-play-production","ipados-alpha":"apple-app-store-production"}
        dist=[]
        for t in sorted(TARGETS): dist.append({"targetId":t,"provider":providers[t],"channelReference":"self-test","payloadSha256":digest,"listingApproved":True,"privacyDisclosureReviewed":True,"ageRatingReviewed":True,"supportContactConfigured":True,"signingVerified":True})
        write(r/"distribution.json",{"schema":"worldmakers.production-distribution.v1","status":"ready","repositoryCommit":commit,"publicVersion":public,"reviewedAtUtc":"2026-01-01T00:00:00Z","reviewer":"self-test","targets":dist,"credentialsStoredInRepository":False})
        write(r/"rollout-plan.json",{"schema":"worldmakers.production-rollout-plan.v1","status":"approved","repositoryCommit":commit,"publicVersion":public,"approvedAtUtc":"2026-01-01T00:00:00Z","approvedBy":"self-test","rollbackAuthority":"self-test","stages":[{"percent":5,"minimumObservationHours":2},{"percent":25,"minimumObservationHours":6},{"percent":50,"minimumObservationHours":12},{"percent":100,"minimumObservationHours":24}],"manualPromotionBetweenStages":True,"haltTriggers":["open-P0-or-P1","crash-free-rate-below-slo","fatal-error-free-rate-below-slo","save-integrity-failure","confirmed-child-safety-incident","telemetry-blindness"]})
        write(r/"operations.json",{"schema":"worldmakers.production-operations.v1","status":"passed","repositoryCommit":commit,"publicVersion":public,"reviewedAtUtc":"2026-01-01T00:00:00Z","reviewer":"self-test","health":{"dashboardReady":True,"alertRoutingReady":True,"crashSymbolsReady":True,"telemetryCoverageVerified":True},"rollback":{"drillPassed":True,"withdrawalProcedureVerified":True,"rollbackTargetVersion":"0.0.9","rollbackTargetSha256":rollback,"authority":"self-test"},"incidentResponse":{"incidentCommander":"self-test","supportEscalationReady":True},"compliance":{"privacyPolicyReviewed":True,"termsReviewed":True,"childSafetyReviewed":True,"storeDataSafetyReviewed":True},"safety":{"childPiiCollected":False,"openChatEnabled":False,"realMoneyOrTokenEarningEnabled":False}})
        approvals={x:True for x in ("qa","engineering","childSafety","product","privacyLegal","releaseOperations")}; reviewers={x:"self-test" for x in approvals}
        write(r/"release-decision.json",{"schema":"worldmakers.production-release-decision.v1","status":"approved","decision":"LAUNCH_APPROVED","repositoryCommit":commit,"publicVersion":public,"decidedAtUtc":"2026-01-01T00:00:00Z","approvals":approvals,"reviewers":reviewers,"knownBlockingIssues":[]})
        good=assess(r,rc,freeze,commit)
        if not good["certified"]: raise AssertionError(good)
        m=load(r/"release-manifest.json"); m["targets"][0]["payloadSha256"]="c"*64; write(r/"release-manifest.json",m)
        if assess(r,rc,freeze,commit)["certified"]: raise AssertionError("RC payload drift did not block production release")
        m["targets"][0]["payloadSha256"]=digest; write(r/"release-manifest.json",m)
        o=load(r/"operations.json"); o["safety"]["openChatEnabled"]=True; write(r/"operations.json",o)
        if assess(r,rc,freeze,commit)["certified"]: raise AssertionError("unsafe open chat did not block production release")
        o["safety"]["openChatEnabled"]=False; o["rollback"]["drillPassed"]=False; write(r/"operations.json",o)
        if assess(r,rc,freeze,commit)["certified"]: raise AssertionError("missing rollback drill did not block production release")
    print("Production Release assessor self-test passed: valid evidence certifies; RC payload drift, unsafe chat and missing rollback drill fail closed.")
    return 0


def main() -> int:
    p=argparse.ArgumentParser(); p.add_argument("--root",default="artifacts/production-release"); p.add_argument("--rc-result",default="artifacts/release-candidate/release-candidate-readiness.json"); p.add_argument("--rc-freeze",default="artifacts/release-candidate/freeze-manifest.json"); p.add_argument("--commit"); p.add_argument("--output"); p.add_argument("--self-test",action="store_true"); a=p.parse_args()
    if a.self_test: return self_test()
    if not a.commit: p.error("--commit is required unless --self-test is used")
    result=assess(Path(a.root),Path(a.rc_result),Path(a.rc_freeze),a.commit)
    if a.output: write(Path(a.output),result)
    else: print(json.dumps(result,indent=2))
    return 0 if result["certified"] else 1

if __name__ == "__main__": raise SystemExit(main())
