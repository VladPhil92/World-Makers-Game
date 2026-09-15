#!/usr/bin/env python3
"""Validate Production Release source infrastructure without claiming launch evidence."""
from __future__ import annotations
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
CONTRACT=ROOT/"content/production/production-release-readiness-v1.json"
RC=ROOT/"content/production/release-candidate-readiness-v1.json"
TEMPLATES=[ROOT/"content/production/production-release-manifest-template.json",ROOT/"content/production/production-distribution-template.json",ROOT/"content/production/production-rollout-plan-template.json",ROOT/"content/production/production-operations-template.json",ROOT/"content/production/production-release-decision-template.json"]
ASSESSOR=ROOT/"scripts/assess-production-release-readiness.py"; RUNNER=ROOT/"scripts/run-production-release-readiness.ps1"; WORKFLOW=ROOT/".github/workflows/production-release-readiness.yml"; DOC=ROOT/"docs/production-release-readiness.md"; LAUNCHER=ROOT/"WorldMakers-ProductionRelease-Certify.cmd"
def load(p): return json.loads(p.read_text(encoding="utf-8-sig"))
def req(c,m):
    if not c: raise SystemExit("Production Release source validation FAILED: "+m)
def contains(p,tokens):
    text=p.read_text(encoding="utf-8")
    for t in tokens: req(t in text,f"{p.relative_to(ROOT)} missing token: {t}")
def main():
    for p in [CONTRACT,RC,ASSESSOR,RUNNER,WORKFLOW,DOC,LAUNCHER,*TEMPLATES]: req(p.is_file(),f"missing {p.relative_to(ROOT)}")
    c=load(CONTRACT); rc=load(RC)
    req(c.get("schema")=="worldmakers.production-release-readiness.v1","contract schema mismatch")
    req(c.get("status")=="source-ready-launch-evidence-required","contract must remain non-certifying")
    req(rc.get("schema")=="worldmakers.release-candidate-readiness.v1","RC dependency schema mismatch")
    req(c["dependencies"]["releaseCandidateResult"]=="artifacts/release-candidate/release-candidate-readiness.json","RC result path mismatch")
    req(set(c["requiredTargets"])=={"windows-reference","android-alpha","ipados-alpha"},"target matrix mismatch")
    for k in ("sameRcCommitRequired","sameRcPayloadSha256Required","publicVersionMustMatchRcBaseVersion"): req(c["immutablePromotion"].get(k) is True,f"immutablePromotion.{k} must be true")
    req(c["immutablePromotion"].get("rebuildAfterRcAllowed") is False,"rebuild after RC must remain prohibited")
    req(c["rollout"]["stagesPercent"]==[5,25,50,100],"rollout percentages mismatch"); req(c["rollout"]["minimumObservationHours"]==[2,6,12,24],"rollout observation windows mismatch")
    req(c["healthSlo"]["minimumSaveIntegrityRate"]==1.0,"save integrity SLO must be 100%")
    for k in ("healthDashboardRequired","alertRoutingRequired","crashSymbolsRequired","rollbackDrillRequired","storeWithdrawalProcedureRequired","incidentCommanderRequired","supportEscalationRequired","privacyPolicyReviewRequired","termsReviewRequired","childSafetyReviewRequired","storeDataSafetyReviewRequired"): req(c["operations"].get(k) is True,f"operations.{k} must be true")
    req(c["operations"]["childPiiAllowed"] is False and c["operations"]["openChatAllowed"] is False and c["operations"]["realMoneyOrTokenEarningAllowed"] is False,"safety boundaries incomplete")
    for k in ("humanGoNoGoRequired","qaApprovalRequired","engineeringApprovalRequired","childSafetyApprovalRequired","productApprovalRequired","privacyLegalApprovalRequired","releaseOperationsApprovalRequired","decisionMustMatchCommitAndVersion"): req(c["promotion"].get(k) is True,f"promotion.{k} must be true")
    req(c["exit"]["nextPhase"]=="production-release-execution","exit phase mismatch")
    for p in TEMPLATES: req(load(p).get("status")=="pending",f"{p.name} must start pending")
    contains(ASSESSOR,["Immutable promotion failed","payload SHA-256 differs from RC","unsafe open chat did not block","missing rollback drill did not block","LAUNCH_APPROVED"])
    contains(RUNNER,["production-release-readiness.json","release-candidate-readiness.json","NON_CERTIFYING_PASS","origin/main"])
    contains(WORKFLOW,["Production Release Readiness","--self-test","Parser]::ParseFile"])
    contains(DOC,["Production Release Readiness","immutable","5%","rollback","Production Release Execution"])
    contains(LAUNCHER,["run-production-release-readiness.ps1"])
    print("Production Release source validation PASS: immutable promotion/distribution/rollout/operations/approval contracts are coherent and fail closed pending real launch evidence.")
    return 0
if __name__=="__main__": raise SystemExit(main())
