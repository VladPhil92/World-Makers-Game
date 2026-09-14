#!/usr/bin/env python3
"""M5.6B game-first Eclipse Engine vertical epic source gate."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EPIC_CANONICAL = ROOT / "content/epics/cross-disciplinary-epics-v1.json"
EPIC_PACKAGED = ROOT / "game/Content/WorldMakers/Epics/cross-disciplinary-epics-v1.json"
EXPERIENCE_CANONICAL = ROOT / "content/epics/eclipse-engine-player-experience-v1.json"
EXPERIENCE_PACKAGED = ROOT / "game/Content/WorldMakers/Epics/eclipse-engine-player-experience-v1.json"

EXPECTED_DISCIPLINES = {
    "mathematics", "geometry", "physics", "english-language", "literature", "philosophy-for-children"
}
FORBIDDEN_SCHOOL_TERMS = {
    "lesson", "quiz", "learning-objective", "learning objective", "grade", "score", "streak", "homework",
    "correct-answer", "wrong-answer",
}


def fail(message: str) -> None:
    raise SystemExit(message)


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def route_key(route: dict) -> tuple[str, ...]:
    return (
        route.get("objectiveId", ""), route.get("disciplineId", ""), route.get("producerKind", ""),
        route.get("producerRefId", ""), route.get("primitiveId", ""), route.get("evidenceEventId", ""),
    )


def state_key(route: dict) -> tuple[str, ...]:
    return (route.get("producerKind", ""), route.get("producerRefId", ""), route.get("worldStateId", ""))


def player_facing_strings(experience: dict) -> list[str]:
    values: list[str] = []
    for chapter in experience.get("chapters", []):
        values.extend(str(chapter.get(key, "")) for key in ("fantasyGoalKey", "tensionKey", "completionReactionKey"))
        for action in chapter.get("actions", []):
            values.extend(str(action.get(key, "")) for key in ("actionId", "interactionVerb", "promptKey", "feedbackKey"))
            values.extend(str(item) for item in action.get("hints", []))
    return values


def main() -> None:
    required_files = [
        EPIC_CANONICAL, EPIC_PACKAGED, EXPERIENCE_CANONICAL, EXPERIENCE_PACKAGED,
        ROOT / "docs/m5-6b-eclipse-engine-vertical-epic.md",
        ROOT / "game/Source/WorldMakers/Adventure/WMEclipseEngineExperience.h",
        ROOT / "game/Source/WorldMakers/Adventure/WMEclipseEngineExperience.cpp",
        ROOT / "game/Source/WorldMakers/Adventure/WMEclipseEngineExperienceSubsystem.h",
        ROOT / "game/Source/WorldMakers/Adventure/WMEclipseEngineExperienceSubsystem.cpp",
        ROOT / "game/Source/WorldMakers/Adventure/WMEclipseEngineInteractableActor.h",
        ROOT / "game/Source/WorldMakers/Adventure/WMEclipseEngineInteractableActor.cpp",
        ROOT / "game/Source/WorldMakers/Adventure/WMEclipseOpticsRuntime.h",
        ROOT / "game/Source/WorldMakers/Adventure/WMEclipseOpticsRuntime.cpp",
        ROOT / "game/Source/WorldMakers/Private/Tests/WMEclipseEngineExperienceTests.cpp",
    ]
    missing = [str(path.relative_to(ROOT)) for path in required_files if not path.exists()]
    if missing:
        fail(f"Missing M5.6B files: {missing}")

    epic_catalog = load(EPIC_CANONICAL)
    if epic_catalog != load(EPIC_PACKAGED):
        fail("Packaged epic catalog diverges from canonical source")
    experience = load(EXPERIENCE_CANONICAL)
    if experience != load(EXPERIENCE_PACKAGED):
        fail("Packaged Eclipse experience diverges from canonical source")

    if experience.get("schemaVersion") != 1 or experience.get("prototypeOnly") is not True:
        fail("M5.6B experience must remain schemaVersion=1 and prototypeOnly=true")
    if experience.get("experienceId") != "experience.eclipse-engine-v1" or experience.get("epicId") != "epic.eclipse-engine":
        fail("Unexpected Eclipse experience identity")
    if experience.get("presentationRule") != "world-first-no-school-ui":
        fail("M5.6B must remain world-first with no school-style player UI")
    if experience.get("rewardModel") != "world-transformation-and-new-capability":
        fail("M5.6B reward must be world transformation/new capability, not points")
    if experience.get("failureModel") != "reversible-experimentation-with-visible-consequence":
        fail("M5.6B failure must remain reversible experimentation")
    if experience.get("hintModel") != "player-requested-progressive-environmental-hints":
        fail("M5.6B hints must be player-requested and progressive")
    if experience.get("privacyModel") != "stable-ids-no-child-free-text":
        fail("M5.6B must preserve minimized learning evidence")

    eclipse = next((e for e in epic_catalog.get("epics", []) if e.get("epicId") == "epic.eclipse-engine"), None)
    if not eclipse:
        fail("Eclipse epic missing")
    if set(eclipse.get("disciplines", [])) != EXPECTED_DISCIPLINES:
        fail(f"Eclipse must use exactly the six causally represented streams: {sorted(EXPECTED_DISCIPLINES)}")

    epic_chapters = eclipse.get("chapters", [])
    experience_chapters = experience.get("chapters", [])
    if len(epic_chapters) != 6 or len(experience_chapters) != 6:
        fail("The Eclipse Engine vertical epic must contain six coherent acts")
    if [c.get("chapterId") for c in epic_chapters] != [c.get("chapterId") for c in experience_chapters]:
        fail("Player-facing acts must preserve authoritative epic chapter order")

    evidence_disciplines: set[str] = set()
    for epic_chapter, player_chapter in zip(epic_chapters, experience_chapters, strict=True):
        actions = player_chapter.get("actions", [])
        if len(actions) < 2:
            fail(f"Chapter {player_chapter.get('chapterId')} is too thin to feel like a game act")
        if "worldState" not in actions[-1]:
            fail(f"Chapter {player_chapter.get('chapterId')} must culminate in visible world transformation")

        epic_evidence = {route_key(item) for item in epic_chapter.get("evidenceRequirements", [])}
        player_evidence = {route_key(action["evidence"]) for action in actions if "evidence" in action}
        if epic_evidence != player_evidence:
            fail(f"Diegetic actions do not map exactly to hidden evidence for {player_chapter.get('chapterId')}")
        evidence_disciplines.update(route[1] for route in player_evidence)

        epic_states = {state_key(item) for item in epic_chapter.get("worldStateRequirements", [])}
        player_states = {state_key(action["worldState"]) for action in actions if "worldState" in action}
        if epic_states != player_states or len(player_states) != 1:
            fail(f"Chapter {player_chapter.get('chapterId')} must have exactly one authoritative causal state")

        for action in actions:
            if bool(action.get("evidence")) == bool(action.get("worldState")):
                fail(f"Action {action.get('actionId')} must be either a puzzle result or a world consequence, never both/neither")
            hints = action.get("hints", [])
            if not 1 <= len(hints) <= 3:
                fail(f"Action {action.get('actionId')} must expose bounded progressive hints")

    if evidence_disciplines != EXPECTED_DISCIPLINES:
        fail(f"Every declared Eclipse discipline must be mechanically necessary, got {sorted(evidence_disciplines)}")

    player_surface = "\n".join(player_facing_strings(experience)).lower()
    for term in FORBIDDEN_SCHOOL_TERMS:
        if term in player_surface:
            fail(f"School-style language leaked into player-facing Eclipse surface: {term}")
    for subject in EXPECTED_DISCIPLINES:
        if subject in player_surface:
            fail(f"Subject label leaked into player-facing Eclipse surface: {subject}")

    serialized = json.dumps(experience, sort_keys=True).lower()
    for manipulative in ("lootbox", "daily-streak", "fomo", "battle-pass", "chance-reward", "artificial-scarcity"):
        if manipulative in serialized:
            fail(f"Manipulative progression is forbidden in M5.6B: {manipulative}")

    subsystem_h = read("game/Source/WorldMakers/Adventure/WMEclipseEngineExperienceSubsystem.h")
    subsystem_cpp = read("game/Source/WorldMakers/Adventure/WMEclipseEngineExperienceSubsystem.cpp")
    interactable_cpp = read("game/Source/WorldMakers/Adventure/WMEclipseEngineInteractableActor.cpp")
    optics_cpp = read("game/Source/WorldMakers/Adventure/WMEclipseOpticsRuntime.cpp")
    interaction_cpp = read("game/Source/WorldMakers/Environment/WMInteractionComponent.cpp")
    player_h = read("game/Source/WorldMakers/Player/WMPlayerCharacter.h")
    tests = read("game/Source/WorldMakers/Private/Tests/WMEclipseEngineExperienceTests.cpp")

    for token in ("BeginAction", "ResolveTrustedAction", "RequestHint", "GetCurrentFantasyGoalKey", "GetCurrentTensionKey"):
        if token not in subsystem_h or token not in subsystem_cpp:
            fail(f"M5.6B experience subsystem missing {token}")
    if "Mission->GetMissionState() != EWMMissionRuntimeState::Completed" not in subsystem_cpp:
        fail("Causal chapter completion must require the underlying mission result")
    if "Evidence-bearing puzzle affordances merely enter their interaction mode" not in interactable_cpp:
        fail("Interactable must document that click alone cannot award evidence")
    for token in ("IsSymmetrySolved", "IsReflectionSolved", "IsPathStable", "ResolveTrustedAction"):
        if token not in optics_cpp:
            fail(f"Executable mirror-lattice gameplay missing {token}")
    if "TActorIterator<AWMEclipseEngineInteractableActor>" not in interaction_cpp:
        fail("Eclipse targets must participate in the normal world-focus loop")
    if "FirstPersonInteractionComponent" not in player_h:
        fail("Player character must expose the presentation-only first-person bridge")

    for test_name in (
        "WorldMakers.Eclipse.PlayerExperience.WorldFirstContract",
        "WorldMakers.Eclipse.PlayerExperience.HiddenEvidenceParity",
        "WorldMakers.Eclipse.PlayerExperience.ProgressivePlayerRequestedHints",
        "WorldMakers.Eclipse.Gameplay.OpticsRequiresSpatialReasoning",
    ):
        if test_name not in tests:
            fail(f"Missing M5.6B automation test: {test_name}")

    docs = read("docs/m5-6b-eclipse-engine-vertical-epic.md").lower()
    for idea in ("discover", "manipulate", "visible consequence", "intrinsic", "no school", "reversible"):
        if idea not in docs:
            fail(f"M5.6B game-design documentation missing principle: {idea}")

    workflow = read(".github/workflows/repo-quality.yml")
    if "python scripts/validate-m5-6b-eclipse-player-experience.py" not in workflow:
        fail("Repository Quality must execute the M5.6B game-first source gate")

    print("M5.6B passed: Eclipse is a six-act world-first game loop with intrinsic rewards, reversible experimentation, hidden evidence parity, progressive hints and executable optics reasoning.")


if __name__ == "__main__":
    main()
