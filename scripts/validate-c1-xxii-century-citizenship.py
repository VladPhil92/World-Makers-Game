#!/usr/bin/env python3
"""Validate C1 XXII Century Citizenship & Interspecies Ethics framework."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FRAMEWORK = ROOT / "content/citizenship/xxii-century-citizenship-v1.json"
OBJECTIVES = ROOT / "content/learning-objectives/xxii-century-citizenship-v1.json"
THOUGHT = ROOT / "content/thought/xxii-interspecies-citizenship-v1.json"
ADVENTURES = ROOT / "content/adventures/xxii-citizenship-adventure-pack-v1.json"
RUNTIME_H = ROOT / "game/Source/WorldMakers/Thought/WMCitizenshipRuntime.h"
RUNTIME_CPP = ROOT / "game/Source/WorldMakers/Thought/WMCitizenshipRuntime.cpp"
TESTS = ROOT / "game/Source/WorldMakers/Private/Tests/WMCitizenshipRuntimeTests.cpp"
DOC = ROOT / "docs/c1-xxii-century-citizenship.md"
QUALITY = ROOT / ".github/workflows/repo-quality.yml"

PRIMITIVES = {
    "construct-to-constraint",
    "observe-and-classify",
    "predict-test-revise",
    "sequence-and-infer",
    "communicate-in-language",
    "model-system",
    "solve-spatial-system",
    "interpret-text-world",
    "reason-through-dilemma",
    "argue-and-revise",
}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit("C1 citizenship validation failed: " + message)


def load(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def main() -> None:
    required = (FRAMEWORK, OBJECTIVES, THOUGHT, ADVENTURES, RUNTIME_H, RUNTIME_CPP, TESTS, DOC, QUALITY)
    missing = [str(p.relative_to(ROOT)) for p in required if not p.is_file()]
    require(not missing, f"missing files: {missing}")

    framework = load(FRAMEWORK)
    require(framework.get("frameworkId") == "citizenship.xxii-century.interspecies-v1", "framework identity drifted")
    require(framework.get("normative") is True, "framework must be normative")
    require(framework.get("prototypeOnly") is True, "C1 native world systems are not yet certified")
    require(framework.get("privacyModel") == "stable-ids-no-child-free-text", "privacy boundary drifted")

    source_ids = {s["sourceId"] for s in framework.get("sourceGrounding", [])}
    require("source.valderrama.en-torno-al-animal" in source_ids, "primary En torno al animal grounding missing")
    require("source.valderrama.doctoral-extension-v2" in source_ids, "AI/philosophy extension grounding missing")

    domains = {d["domainId"] for d in framework.get("domains", [])}
    expected_domains = {
        "domain.living-world",
        "domain.planetary-commons",
        "domain.technology-energy",
        "domain.responsible-intelligence",
        "domain.ethics-sovereignty-coexistence",
    }
    require(domains == expected_domains, f"five-domain model drifted: {sorted(domains)}")

    hooks = set(framework.get("simulationHooks", []))
    for hook in (
        "water-quality", "air-quality", "soil-health", "biodiversity", "habitat-connectivity",
        "animal-stress", "energy-reliability", "material-demand", "compute-energy-demand",
        "human-wellbeing", "civic-legitimacy",
    ):
        require(hook in hooks, f"simulation hook missing: {hook}")

    ai = framework.get("responsibleAIGovernance", {})
    for key in (
        "humanResponsibilityRequired", "boundedAuthorityRequired", "auditabilityRequired",
        "privacyAndFairnessRequired", "failSafeRequired", "ecologicalCostReviewRequired",
    ):
        require(ai.get(key) is True, f"responsible AI safeguard missing: {key}")
    require(ai.get("machineMoralStatusAssumed") is False, "C1 may not assume AI moral status")
    require(ai.get("machineMoralStatusDenied") is False, "C1 may not deny AI moral status by fiat")

    pedagogy = framework.get("pedagogyPolicy", {})
    require(pedagogy.get("doctrinalComplianceScoring") is False, "doctrinal compliance scoring is forbidden")
    require(pedagogy.get("evaluateReasoningNotConclusion") is True, "reasoning-not-conclusion rule missing")
    require(pedagogy.get("requireMultipleDefensibleOptions") is True, "multiple defensible options required")
    require(pedagogy.get("noMoralRewardForAgreement") is True, "agreement cannot be rewarded as moral correctness")
    for key in (
        "minimumReasonIds", "minimumHumanPerspectives", "minimumNonHumanPerspectives", "minimumCommons",
        "minimumTradeoffs", "minimumUncertainties", "minimumRevisions",
    ):
        require(int(pedagogy.get(key, 0)) >= 1, f"structured reasoning minimum missing: {key}")

    objectives = load(OBJECTIVES)
    items = objectives.get("objectives", [])
    require(len(items) >= 10, "at least ten citizenship learning objectives are required")
    require(objectives.get("assessmentPolicy", {}).get("ideologicalAgreementRequired") is False, "objectives cannot require ideological agreement")
    require(objectives.get("assessmentPolicy", {}).get("revisionOpportunityRequired") is True, "revision opportunity is required")
    for objective in items:
        require(objective.get("domainId") in expected_domains, f"objective references unknown domain: {objective.get('objectiveId')}")
        primitive_ids = set(objective.get("evidencePrimitiveIds", []))
        require(primitive_ids and primitive_ids.issubset(PRIMITIVES), f"invalid objective primitive set: {objective.get('objectiveId')}")

    thought = load(THOUGHT)
    require(thought.get("evaluationRule") == "score-structured-reasoning-never-doctrinal-agreement", "thought evaluation rule drifted")
    cases = thought.get("deliberationCases", [])
    require(len(cases) >= 5, "five deliberation cases are required")
    serialized_thought = THOUGHT.read_text(encoding="utf-8")
    for forbidden in ("correctOption", "preferredOption", "approvedMoralAnswer", "ideologyScore"):
        require(forbidden not in serialized_thought, f"hidden moral-answer key detected: {forbidden}")
    for case in cases:
        entities = case.get("affectedEntityIds", [])
        require(any(e.startswith("human.") for e in entities), f"case lacks human perspective: {case.get('caseId')}")
        require(any(e.startswith("animal.") or e.startswith("ecosystem.") for e in entities), f"case lacks non-human perspective: {case.get('caseId')}")
        require(any(e.startswith("commons.") for e in entities) or "domain.planetary-commons" not in case.get("domainIds", []), f"planetary-commons case lacks commons: {case.get('caseId')}")
        require(len(case.get("optionIds", [])) >= 3, f"case needs multiple defensible options: {case.get('caseId')}")
        require(len(case.get("reasonIds", [])) >= 2, f"case lacks reasons: {case.get('caseId')}")
        require(case.get("tradeoffIds"), f"case lacks tradeoffs: {case.get('caseId')}")
        require(case.get("uncertaintyIds"), f"case lacks uncertainty: {case.get('caseId')}")

    adventures = load(ADVENTURES)
    require(adventures.get("citizenshipFrameworkId") == framework["frameworkId"], "adventure pack must bind C1 framework")
    adventure_items = adventures.get("adventures", [])
    require(len(adventure_items) == 5, "C1 must ship exactly five first-wave adventure contracts")
    required_adventures = {
        "adventure.xxii.river-two-futures",
        "adventure.xxii.city-trusted-algorithm",
        "adventure.xxii.forest-learned-to-listen",
        "adventure.xxii.last-power-grid",
        "adventure.xxii.community-new-standards",
    }
    require({a["adventureId"] for a in adventure_items} == required_adventures, "first-wave adventure set drifted")
    for adventure in adventure_items:
        beats = adventure.get("beats", [])
        require(len(beats) >= 3, f"adventure requires experience/concept/formalization depth: {adventure.get('adventureId')}")
        primitive_ids = {b.get("primitiveId") for b in beats}
        require(primitive_ids.issubset(PRIMITIVES), f"unsupported mission primitive in {adventure.get('adventureId')}")
        require("reason-through-dilemma" in primitive_ids or "argue-and-revise" in primitive_ids, f"citizenship adventure lacks deliberation: {adventure.get('adventureId')}")

    header = RUNTIME_H.read_text(encoding="utf-8")
    cpp = RUNTIME_CPP.read_text(encoding="utf-8")
    for token in (
        "FWMXXIICitizenshipReasoningPolicy", "FWMXXIICitizenshipReasoningInput", "FWMAIGovernanceReviewInput",
        "FWMSustainableTechnologyReviewInput", "EvaluateDeliberation", "EvaluateResponsibleAI", "EvaluateSustainableTechnology",
    ):
        require(token in header, f"runtime header missing: {token}")
    require("OptionId" not in header and "Correct" not in header, "citizenship runtime must not accept a moral answer key")
    require("FString" not in header and "FText" not in header, "runtime evidence must remain stable-ID only")
    for token in (
        'TEXT("reason-through-dilemma")', 'TEXT("construct-to-constraint")', 'TEXT("model-system")',
        "bRecognizedPowerAsymmetry", "bCheckedReversibility", "bEcologicalCostReviewed", "bLifecycleMaterialsConsidered",
    ):
        require(token in cpp or token in header, f"runtime implementation missing: {token}")

    tests = TESTS.read_text(encoding="utf-8")
    for test_name in (
        "WorldMakers.Citizenship.Deliberation.ReasonsAcrossHumanNonHumanAndCommons",
        "WorldMakers.Citizenship.Deliberation.NoCorrectMoralOptionKey",
        "WorldMakers.Citizenship.AI.BoundedAuditableAccountableFailSafe",
        "WorldMakers.Citizenship.Technology.WholeSystemNotGreenLabel",
    ):
        require(test_name in tests, f"automation test missing: {test_name}")
    require("Human-only reasoning cannot satisfy an interspecies case" in tests, "negative interspecies coverage test missing")
    require("renewable label alone" in tests, "anti-green-label shortcut test missing")

    doc = DOC.read_text(encoding="utf-8").lower()
    for phrase in (
        "sovereignty of hospitality", "radical singularity", "more-than-human community", "no ideology key",
        "every action enters a web of relations", "the city that trusted the algorithm", "the last power grid",
    ):
        require(phrase in doc, f"C1 documentation missing: {phrase}")

    quality = QUALITY.read_text(encoding="utf-8")
    require("python scripts/validate-c1-xxii-century-citizenship.py" in quality, "Repository Quality must run C1 gate")

    print("C1 XXII Century Citizenship passed: five domains, interspecies/commons reasoning, sustainable technology, responsible AI governance, no-ideology scoring, five adventure contracts and deterministic stable-ID runtime are wired.")


if __name__ == "__main__":
    main()
