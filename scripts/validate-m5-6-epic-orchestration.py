#!/usr/bin/env python3
"""M5.6A cross-disciplinary epic orchestration source gate."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CANONICAL = ROOT / "content/epics/cross-disciplinary-epics-v1.json"
PACKAGED = ROOT / "game/Content/WorldMakers/Epics/cross-disciplinary-epics-v1.json"
ADVENTURE_PACK = ROOT / "content/adventures/first-fantastic-adventure-pack-v1.json"
FIRST_CLASS = {
    "mathematics", "geometry", "english-language", "spanish-language", "literature", "biology",
    "chemistry", "physics", "ecology", "ethics", "philosophy-for-children",
}
ALLOWED_PRODUCERS = {"building", "science", "thought", "world"}


def fail(message: str) -> None:
    raise SystemExit(message)


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def mission_index() -> dict[str, dict]:
    result: dict[str, dict] = {}
    for path in (ROOT / "content/missions").rglob("*.json"):
        try:
            item = load(path)
        except (json.JSONDecodeError, UnicodeDecodeError):
            continue
        mission_id = item.get("id")
        if isinstance(mission_id, str):
            if mission_id in result:
                fail(f"Duplicate mission id while validating M5.6A: {mission_id}")
            result[mission_id] = item
    return result


def main() -> None:
    required_files = (
        CANONICAL,
        PACKAGED,
        ADVENTURE_PACK,
        ROOT / "docs/m5-6-cross-disciplinary-epic-adventures.md",
        ROOT / "game/Source/WorldMakers/Adventure/WMEpicRuntime.h",
        ROOT / "game/Source/WorldMakers/Adventure/WMEpicRuntime.cpp",
        ROOT / "game/Source/WorldMakers/Adventure/WMEpicRuntimeSubsystem.h",
        ROOT / "game/Source/WorldMakers/Adventure/WMEpicRuntimeSubsystem.cpp",
        ROOT / "game/Source/WorldMakers/Private/Tests/WMEpicRuntimeTests.cpp",
    )
    missing = [str(path.relative_to(ROOT)) for path in required_files if not path.exists()]
    if missing:
        fail(f"Missing M5.6A files: {missing}")

    canonical = load(CANONICAL)
    packaged = load(PACKAGED)
    if canonical != packaged:
        fail("Packaged epic catalog must match canonical source semantically")
    if canonical.get("schemaVersion") != 1 or canonical.get("prototypeOnly") is not True:
        fail("M5.6A catalog must remain schemaVersion=1 and prototypeOnly=true")
    if canonical.get("catalogId") != "epic-catalog.cross-disciplinary-v1":
        fail("Unexpected M5.6A catalogId")
    if canonical.get("designPrinciple") != "learning-is-structurally-necessary-to-play":
        fail("M5.6A must preserve the core gameplay-learning principle")
    if canonical.get("progressionModel") != "ordered-chapters-world-state-gated":
        fail("M5.6A must use ordered chapters gated by trusted world state")
    if canonical.get("privacyModel") != "stable-ids-no-child-free-text":
        fail("M5.6A must preserve stable-ids-no-child-free-text")

    epics = canonical.get("epics", [])
    if len(epics) < 2:
        fail("M5.6A requires at least two epic contracts")
    ids = {epic.get("epicId") for epic in epics}
    for required in {"epic.eclipse-engine", "epic.garden-end-winter"}:
        if required not in ids:
            fail(f"Missing required first epic: {required}")

    missions = mission_index()
    adventure_pack = load(ADVENTURE_PACK)
    producer_index: dict[tuple[str, str, str], tuple[str, str]] = {}
    for adventure in adventure_pack.get("adventures", []):
        mission_id = adventure.get("missionId")
        for beat in adventure.get("beats", []):
            key = (mission_id, beat.get("primitiveId"), beat.get("evidenceEventId"))
            producer_index[key] = (beat.get("producerKind"), beat.get("producerRefId"))

    seen_epics: set[str] = set()
    for epic in epics:
        epic_id = epic.get("epicId")
        if not isinstance(epic_id, str) or not epic_id.startswith("epic.") or epic_id in seen_epics:
            fail(f"Invalid/duplicate epic id: {epic_id!r}")
        seen_epics.add(epic_id)
        disciplines = epic.get("disciplines", [])
        if len(disciplines) != len(set(disciplines)) or len(set(disciplines) & FIRST_CLASS) < 4:
            fail(f"Epic {epic_id} must declare at least four distinct first-class streams")

        chapters = epic.get("chapters", [])
        if len(chapters) < 2:
            fail(f"Epic {epic_id} must contain ordered chapters")
        chapter_ids: set[str] = set()
        mission_ids: set[str] = set()
        attributed_disciplines: set[str] = set()
        for chapter in chapters:
            chapter_id = chapter.get("chapterId")
            mission_id = chapter.get("missionId")
            if not isinstance(chapter_id, str) or not chapter_id.startswith("chapter.") or chapter_id in chapter_ids:
                fail(f"Epic {epic_id} has invalid/duplicate chapter: {chapter_id!r}")
            if not isinstance(mission_id, str) or mission_id in mission_ids or mission_id not in missions:
                fail(f"Epic {epic_id} chapter {chapter_id} references invalid/duplicate mission {mission_id!r}")
            chapter_ids.add(chapter_id)
            mission_ids.add(mission_id)

            mission = missions[mission_id]
            runtime = mission.get("runtime", {})
            if runtime.get("evaluator") != "composable" or runtime.get("prototypeOnly") is not True:
                fail(f"Epic chapter {chapter_id} must reuse a composable prototype mission")
            mission_requirements = runtime.get("evidencePrimitives", [])
            epic_requirements = chapter.get("evidenceRequirements", [])
            if len(epic_requirements) != len(mission_requirements) or not epic_requirements:
                fail(f"Epic chapter {chapter_id} must map every mission evidence requirement exactly once")

            by_key = {
                (r.get("primitiveId"), r.get("evidenceEventId"), r.get("objectiveId")): r
                for r in mission_requirements
            }
            requirement_ids: set[str] = set()
            for requirement in epic_requirements:
                requirement_id = requirement.get("requirementId")
                key = (requirement.get("primitiveId"), requirement.get("evidenceEventId"), requirement.get("objectiveId"))
                if not isinstance(requirement_id, str) or not requirement_id.startswith("req.") or requirement_id in requirement_ids:
                    fail(f"Chapter {chapter_id} has invalid/duplicate requirement id")
                requirement_ids.add(requirement_id)
                mission_requirement = by_key.get(key)
                if mission_requirement is None:
                    fail(f"Chapter {chapter_id} has evidence that does not map to mission objective: {key}")
                if requirement.get("requiredCount") != mission_requirement.get("requiredCount", 1):
                    fail(f"Chapter {chapter_id} requiredCount diverges from mission contract")
                discipline = requirement.get("disciplineId")
                if discipline != mission.get("discipline") or discipline not in FIRST_CLASS:
                    fail(f"Chapter {chapter_id} evidence attribution must name the mission's first-class discipline")
                attributed_disciplines.add(discipline)
                producer = (requirement.get("producerKind"), requirement.get("producerRefId"))
                expected_producer = producer_index.get((mission_id, requirement.get("primitiveId"), requirement.get("evidenceEventId")))
                if producer != expected_producer or producer[0] not in ALLOWED_PRODUCERS or not producer[1]:
                    fail(f"Chapter {chapter_id} evidence producer does not match trusted M5.5 producer routing")

            states = chapter.get("worldStateRequirements", [])
            if not states:
                fail(f"Chapter {chapter_id} must require at least one trusted world-state predicate")
            state_ids: set[str] = set()
            for state in states:
                state_id = state.get("worldStateId")
                if not isinstance(state_id, str) or not state_id.startswith("world-state.") or state_id in state_ids:
                    fail(f"Chapter {chapter_id} has invalid/duplicate world-state id")
                state_ids.add(state_id)
                if state.get("producerKind") not in ALLOWED_PRODUCERS or not state.get("producerRefId"):
                    fail(f"Chapter {chapter_id} has invalid world-state producer")

        if len(attributed_disciplines) < 4:
            fail(f"Epic {epic_id} must demonstrate at least four evidence-attributed disciplines, got {sorted(attributed_disciplines)}")

    serialized = json.dumps(canonical, sort_keys=True).lower()
    for forbidden in ("voicetranscript", "childauthoredtext", "personalityscore", "ideologylabel", "clientcompletionflag"):
        if forbidden in serialized:
            fail(f"M5.6A privacy/authority boundary violated by field: {forbidden}")

    runtime_h = read("game/Source/WorldMakers/Adventure/WMEpicRuntime.h")
    runtime_cpp = read("game/Source/WorldMakers/Adventure/WMEpicRuntime.cpp")
    subsystem = read("game/Source/WorldMakers/Adventure/WMEpicRuntimeSubsystem.cpp")
    for token in ("ObjectiveId", "DisciplineId", "WorldStateId", "CanAcceptEvidence", "CanAcceptWorldState", "IsCurrentChapterReadyToAdvance", "AdvanceChapter"):
        if token not in runtime_h and token not in runtime_cpp:
            fail(f"M5.6A runtime missing fail-closed orchestration token: {token}")
    for token in ("ActivateEpic", "RecordEpicEvidence", "RecordComposableEvidence", "RecordEpicWorldState", "AdvanceEpicIfReady"):
        if token not in subsystem and token not in read("game/Source/WorldMakers/Adventure/WMEpicRuntimeSubsystem.h"):
            fail(f"M5.6A subsystem missing integration token: {token}")

    tests = read("game/Source/WorldMakers/Private/Tests/WMEpicRuntimeTests.cpp")
    for test_name in (
        "WorldMakers.Epic.Catalog.CrossDisciplinaryBreadth",
        "WorldMakers.Epic.Runtime.WorldStateGate",
        "WorldMakers.Epic.Runtime.AttributionFailClosed",
        "WorldMakers.Epic.Runtime.CatalogEndToEnd",
    ):
        if test_name not in tests:
            fail(f"Missing M5.6A automation test: {test_name}")

    game_ini = read("game/Config/DefaultGame.ini")
    for staged in (
        'DirectoriesToAlwaysStageAsNonUFS=(Path="WorldMakers/Adventures")',
        'DirectoriesToAlwaysStageAsNonUFS=(Path="WorldMakers/Epics")',
    ):
        if staged not in game_ini:
            fail(f"Packaged builds missing content staging: {staged}")

    workflow = read(".github/workflows/repo-quality.yml")
    if "python scripts/validate-m5-6-epic-orchestration.py" not in workflow:
        fail("Repository Quality must execute M5.6A source gate")

    docs = read("docs/m5-6-cross-disciplinary-epic-adventures.md")
    for token in ("M5.6A", "The Eclipse Engine", "The Garden at the End of Winter", "world-state", "source-complete"):
        if token not in docs:
            fail(f"M5.6A documentation missing contract token: {token}")

    print("M5.6A epic orchestration passed: two real epics traverse end-to-end with 4+ attributed streams, exact mission/producers, world-state gates, privacy and authority boundaries wired.")


if __name__ == "__main__":
    main()
