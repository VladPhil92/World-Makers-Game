#!/usr/bin/env python3
"""M5.4 Language, Literature and Thought Runtime source gate."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CANONICAL = "content/thought/language-literature-thought-v1.json"
PACKAGED = "game/Content/WorldMakers/Thought/language-literature-thought-v1.json"


def fail(message: str) -> None:
    raise SystemExit(message)


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def walk_forbidden_keys(value: object, path: str = "root") -> None:
    forbidden = {
        "freetext", "free_text", "transcript", "voiceTranscript", "politicalLabel",
        "religiousLabel", "personalityScore", "correctOptionId", "preferredMoralOption"
    }
    if isinstance(value, dict):
        for key, child in value.items():
            if key in forbidden:
                fail(f"M5.4 privacy/neutrality boundary forbids field {path}.{key}")
            walk_forbidden_keys(child, f"{path}.{key}")
    elif isinstance(value, list):
        for index, child in enumerate(value):
            walk_forbidden_keys(child, f"{path}[{index}]")


def main() -> None:
    required_files = (
        CANONICAL,
        PACKAGED,
        "docs/m5-4-language-literature-thought-runtime.md",
        "game/Source/WorldMakers/Thought/WMLanguageThoughtRuntime.h",
        "game/Source/WorldMakers/Thought/WMLanguageThoughtRuntime.cpp",
        "game/Source/WorldMakers/Thought/WMLanguageThoughtSubsystem.h",
        "game/Source/WorldMakers/Thought/WMLanguageThoughtSubsystem.cpp",
        "game/Source/WorldMakers/Private/Tests/WMLanguageThoughtRuntimeTests.cpp",
    )
    missing = [path for path in required_files if not (ROOT / path).exists()]
    if missing:
        fail(f"Missing M5.4 files: {missing}")

    canonical = json.loads(read(CANONICAL))
    packaged = json.loads(read(PACKAGED))
    if canonical != packaged:
        fail("Packaged thought catalog must exactly match canonical source semantically")
    if canonical.get("schemaVersion") != 1 or canonical.get("prototypeOnly") is not True:
        fail("M5.4 catalog must remain schemaVersion=1 and prototypeOnly=true")
    if canonical.get("privacyModel") != "stable-ids-no-child-free-text":
        fail("M5.4 must lock stable-ids-no-child-free-text privacy model")
    walk_forbidden_keys(canonical)

    challenges = canonical.get("communicationChallenges", [])
    languages = {item.get("targetLanguageId") for item in challenges}
    if not {"en", "es"}.issubset(languages):
        fail("M5.4 requires contextual communication coverage for both English and Spanish")
    challenge_ids: set[str] = set()
    for challenge in challenges:
        cid = challenge.get("challengeId")
        if not isinstance(cid, str) or not cid.startswith("language.") or cid in challenge_ids:
            fail(f"Invalid/duplicate communication challenge: {cid!r}")
        challenge_ids.add(cid)
        target = challenge.get("targetLanguageId")
        meaning = challenge.get("requiredMeaningId")
        registers = challenge.get("acceptedRegisterIds", [])
        options = challenge.get("options", [])
        if not target or not meaning or not registers or len(options) < 2:
            fail(f"Communication challenge {cid} lacks semantic choice structure")
        option_ids: set[str] = set()
        passing = 0
        for option in options:
            oid = option.get("choiceId")
            if not isinstance(oid, str) or oid in option_ids:
                fail(f"Communication challenge {cid} has invalid/duplicate option {oid!r}")
            option_ids.add(oid)
            for field in ("languageId", "meaningId", "registerId", "syntaxPatternId", "evidenceEventId"):
                if not isinstance(option.get(field), str) or not option[field]:
                    fail(f"Communication option {oid} missing {field}")
            if option["languageId"] == target and option["meaningId"] == meaning and option["registerId"] in registers:
                passing += 1
        if passing < 1:
            fail(f"Communication challenge {cid} has no semantically valid option")

    stories = canonical.get("narrativeStories", [])
    if not stories:
        fail("M5.4 requires at least one myth/legend narrative graph")
    allowed_source_classes = {"retelling", "adaptation", "historical-source"}
    allowed_reviews = {"draft", "approved", "changes-requested"}
    for story in stories:
        sid = story.get("storyId")
        if not isinstance(sid, str) or not sid.startswith("story."):
            fail(f"Invalid story id: {sid!r}")
        if story.get("sourceClassId") not in allowed_source_classes:
            fail(f"Story {sid} missing valid source classification")
        if not str(story.get("provenanceKey", "")).startswith("provenance."):
            fail(f"Story {sid} missing provenance key")
        if story.get("culturalReviewState") not in allowed_reviews:
            fail(f"Story {sid} missing cultural review state")
        nodes = story.get("nodes", [])
        choices = story.get("choices", [])
        node_ids = {node.get("nodeId") for node in nodes}
        if len(nodes) < 2 or len(node_ids) != len(nodes) or story.get("startNodeId") not in node_ids:
            fail(f"Story {sid} graph nodes/start are invalid")
        for node in nodes:
            if "passage" in node:
                fail(f"Story {sid} must use localization passageKey rather than embedded authored passage")
            if not str(node.get("passageKey", "")).startswith("story.") or not str(node.get("pointOfViewId", "")).startswith("pov."):
                fail(f"Story {sid} node lacks passage key or point of view")
        if not choices:
            fail(f"Story {sid} needs inference edges")
        for choice in choices:
            if choice.get("fromNodeId") not in node_ids or choice.get("toNodeId") not in node_ids:
                fail(f"Story {sid} has edge to unknown node")
            if not str(choice.get("inferenceTag", "")).startswith("inference."):
                fail(f"Story {sid} edge lacks inference tag")
            if not str(choice.get("evidenceEventId", "")).startswith("literature."):
                fail(f"Story {sid} edge lacks literature evidence")

    dilemmas = canonical.get("ethicalDilemmas", [])
    if not dilemmas:
        fail("M5.4 requires at least one ethical dilemma")
    for dilemma in dilemmas:
        did = dilemma.get("dilemmaId")
        perspectives = dilemma.get("perspectiveIds", [])
        required = dilemma.get("requiredPerspectiveCount", 0)
        options = dilemma.get("options", [])
        if not isinstance(did, str) or not did.startswith("ethics.") or len(perspectives) < 2 or not 2 <= required <= len(perspectives) or len(options) < 2:
            fail(f"Ethical dilemma {did!r} lacks multi-perspective structure")
        for option in options:
            oid = option.get("optionId")
            if not option.get("supportedReasonIds") or not option.get("affectedPerspectiveIds") or not option.get("tradeoffTags"):
                fail(f"Ethical option {oid!r} must expose reasons, perspectives and tradeoffs")
            if not set(option["affectedPerspectiveIds"]).issubset(set(perspectives)):
                fail(f"Ethical option {oid!r} references unknown perspective")
            profile = option.get("consequenceProfile", {})
            if not profile:
                fail(f"Ethical option {oid!r} lacks consequence profile")
            for dimension, value in profile.items():
                if not isinstance(value, (int, float)) or not -1 <= value <= 1:
                    fail(f"Ethical consequence {oid!r}/{dimension} must be normalized to [-1,1]")
            if not str(option.get("evidenceEventId", "")).startswith("ethics."):
                fail(f"Ethical option {oid!r} lacks ethics evidence event")

    problems = canonical.get("philosophyProblems", [])
    if not problems:
        fail("M5.4 requires at least one philosophy problem")
    relations = {"supports", "challenges", "clarifies"}
    for problem in problems:
        pid = problem.get("problemId")
        claims = problem.get("claimIds", [])
        assumptions = problem.get("assumptionIds", [])
        counterexamples = problem.get("counterexampleIds", [])
        links = problem.get("reasonLinks", [])
        if not isinstance(pid, str) or not pid.startswith("philosophy.") or len(claims) < 2:
            fail(f"Philosophy problem {pid!r} must permit alternative claims")
        if not assumptions or not counterexamples or not links:
            fail(f"Philosophy problem {pid} must include assumptions, counterexamples and reason links")
        if problem.get("minReasonLinks", 0) < 1 or problem.get("minAssumptions", 0) < 1 or problem.get("minCounterexamples", 0) < 1 or problem.get("minRevisions", 0) < 1:
            fail(f"Philosophy problem {pid} rubric must require reasoning and revision")
        targeted_claims: set[str] = set()
        for link in links:
            if link.get("relationId") not in relations or link.get("toPropositionId") not in claims:
                fail(f"Philosophy problem {pid} has invalid reason relation/claim target")
            targeted_claims.add(link["toPropositionId"])
        if len(targeted_claims) < 2:
            fail(f"Philosophy problem {pid} must provide argument paths for alternative positions")
        if not str(problem.get("evidenceEventId", "")).startswith("philosophy."):
            fail(f"Philosophy problem {pid} lacks stable philosophy evidence")

    runtime_h = read("game/Source/WorldMakers/Thought/WMLanguageThoughtRuntime.h")
    runtime_cpp = read("game/Source/WorldMakers/Thought/WMLanguageThoughtRuntime.cpp")
    for token in (
        "EvaluateCommunication", "TraverseNarrative", "EvaluateEthicalReasoning", "EvaluatePhilosophicalArgument",
        "communicate-in-language", "interpret-text-world", "reason-through-dilemma", "argue-and-revise",
        "stable-ids-no-child-free-text"
    ):
        if token not in runtime_h and token not in runtime_cpp:
            fail(f"M5.4 runtime missing contract token: {token}")

    subsystem_h = read("game/Source/WorldMakers/Thought/WMLanguageThoughtSubsystem.h")
    subsystem_cpp = read("game/Source/WorldMakers/Thought/WMLanguageThoughtSubsystem.cpp")
    for token in (
        "EvaluateCommunicationAndRecord", "TraverseNarrativeAndRecord", "EvaluateEthicalReasoningAndRecord",
        "EvaluatePhilosophicalArgumentAndRecord", "RecordComposableEvidence", "language-literature-thought-v1.json"
    ):
        if token not in subsystem_h and token not in subsystem_cpp:
            fail(f"M5.4 subsystem missing integration token: {token}")

    tests = read("game/Source/WorldMakers/Private/Tests/WMLanguageThoughtRuntimeTests.cpp")
    for test_name in (
        "WorldMakers.Thought.Language.ContextualBilingualCommunication",
        "WorldMakers.Thought.Literature.ProvenancePointOfViewAndInference",
        "WorldMakers.Thought.Ethics.ReasonsPerspectivesNotIdeology",
        "WorldMakers.Thought.Philosophy.ArgumentCounterexampleAndRevision",
        "WorldMakers.Thought.Integration.ComposableEvidenceAcrossDomains",
    ):
        if test_name not in tests:
            fail(f"Missing M5.4 automation test: {test_name}")

    game_ini = read("game/Config/DefaultGame.ini")
    if 'DirectoriesToAlwaysStageAsNonUFS=(Path="WorldMakers/Thought")' not in game_ini:
        fail("Packaged builds must stage WorldMakers/Thought")

    docs = read("docs/m5-4-language-literature-thought-runtime.md")
    for token in (
        "English and Spanish", "provenance", "point-of-view", "not moral answer keys",
        "counterexamples", "RecordComposableEvidence", "stable-ids-no-child-free-text"
    ):
        if token not in docs:
            fail(f"M5.4 documentation missing contract token: {token}")

    workflow = read(".github/workflows/repo-quality.yml")
    if "python scripts/validate-m5-4-language-thought-runtime.py" not in workflow:
        fail("Repository Quality must execute M5.4 source gate")

    print(
        f"M5.4 Language/Literature/Thought passed: {len(challenges)} bilingual challenges, {len(stories)} narrative graph(s), "
        f"{len(dilemmas)} ethical dilemma(s), {len(problems)} philosophy problem(s), neutral structured reasoning and M5.2 evidence integration are wired."
    )


if __name__ == "__main__":
    main()
