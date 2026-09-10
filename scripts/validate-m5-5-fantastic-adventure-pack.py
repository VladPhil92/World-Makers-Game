#!/usr/bin/env python3
"""M5.5 First Fantastic Adventure Pack source gate."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CANONICAL_PACK = ROOT / "content/adventures/first-fantastic-adventure-pack-v1.json"
PACKAGED_PACK = ROOT / "game/Content/WorldMakers/Adventures/first-fantastic-adventure-pack-v1.json"
FIRST_CLASS_STREAMS = {
    "mathematics", "geometry", "english-language", "spanish-language", "literature", "biology",
    "chemistry", "physics", "ecology", "ethics", "philosophy-for-children",
}
ALLOWED_SECONDARY = FIRST_CLASS_STREAMS | {"history-culture"}
ALLOWED_PRODUCERS = {"building", "science", "thought", "world"}
FORBIDDEN_REWARD_TOKENS = {"loot", "streak", "daily", "limited", "chance", "gacha", "jackpot"}


def fail(message: str) -> None:
    raise SystemExit(message)


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def mission_filename(mission_id: str) -> str:
    prefix = "mission.fantastic."
    if not mission_id.startswith(prefix):
        fail(f"M5.5 mission must use {prefix}: {mission_id}")
    return f"{mission_id[len(prefix):]}.json"


def main() -> None:
    required_files = (
        CANONICAL_PACK,
        PACKAGED_PACK,
        ROOT / "docs/m5-5-first-fantastic-adventure-pack.md",
        ROOT / "game/Source/WorldMakers/Adventure/WMAdventureRuntime.h",
        ROOT / "game/Source/WorldMakers/Adventure/WMAdventureRuntime.cpp",
        ROOT / "game/Source/WorldMakers/Adventure/WMAdventureRuntimeSubsystem.h",
        ROOT / "game/Source/WorldMakers/Adventure/WMAdventureRuntimeSubsystem.cpp",
        ROOT / "game/Source/WorldMakers/Private/Tests/WMFantasticAdventurePackTests.cpp",
    )
    missing = [str(path.relative_to(ROOT)) for path in required_files if not path.exists()]
    if missing:
        fail(f"Missing M5.5 files: {missing}")

    canonical = load(CANONICAL_PACK)
    packaged = load(PACKAGED_PACK)
    if canonical != packaged:
        fail("Packaged First Fantastic Adventure Pack must match canonical source semantically")
    if canonical.get("schemaVersion") != 1 or canonical.get("prototypeOnly") is not True:
        fail("M5.5 pack must remain schemaVersion=1 and prototypeOnly=true")
    if canonical.get("designPrinciple") != "learning-is-structurally-necessary-to-play":
        fail("M5.5 must lock learning-is-structurally-necessary-to-play")
    if canonical.get("progressionModel") != "ordered-beats":
        fail("M5.5 must use ordered-beats progression")
    if canonical.get("privacyModel") != "stable-ids-no-child-free-text":
        fail("M5.5 must preserve stable-ids-no-child-free-text")

    adventures = canonical.get("adventures", [])
    if len(adventures) != 11:
        fail(f"M5.5 requires exactly 11 first-pack adventures, got {len(adventures)}")
    primary_streams = [item.get("primaryDiscipline") for item in adventures]
    if set(primary_streams) != FIRST_CLASS_STREAMS or len(set(primary_streams)) != 11:
        fail(f"M5.5 must cover each first-class stream exactly once: {primary_streams}")

    thought = load(ROOT / "content/thought/language-literature-thought-v1.json")
    thought_refs = {
        *(item["challengeId"] for item in thought.get("communicationChallenges", [])),
        *(item["storyId"] for item in thought.get("narrativeStories", [])),
        *(item["dilemmaId"] for item in thought.get("ethicalDilemmas", [])),
        *(item["problemId"] for item in thought.get("philosophyProblems", [])),
    }

    seen_adventures: set[str] = set()
    seen_missions: set[str] = set()
    beat_count = 0
    for adventure in adventures:
        aid = adventure.get("adventureId")
        mid = adventure.get("missionId")
        if not isinstance(aid, str) or not aid.startswith("adventure.fantastic.") or aid in seen_adventures:
            fail(f"Invalid/duplicate adventure ID: {aid!r}")
        if not isinstance(mid, str) or mid in seen_missions:
            fail(f"Invalid/duplicate mission link for {aid}: {mid!r}")
        seen_adventures.add(aid)
        seen_missions.add(mid)
        if adventure.get("ageBand") not in {"4-6", "7-8", "9-10"}:
            fail(f"Adventure {aid} has unsupported age band")
        secondaries = adventure.get("secondaryDisciplines", [])
        if len(secondaries) != len(set(secondaries)) or not set(secondaries).issubset(ALLOWED_SECONDARY):
            fail(f"Adventure {aid} has invalid secondary disciplines")
        if adventure["primaryDiscipline"] in secondaries:
            fail(f"Adventure {aid} repeats primary discipline as secondary")

        filename = mission_filename(mid)
        mission_path = ROOT / "content/missions/fantastic" / filename
        runtime_path = ROOT / "game/Content/WorldMakers/Missions" / filename
        if not mission_path.exists() or not runtime_path.exists():
            fail(f"Adventure {aid} is missing canonical or packaged mission {filename}")
        mission = load(mission_path)
        runtime_mission = load(runtime_path)
        if mission != runtime_mission:
            fail(f"Adventure {aid} mission source/runtime copy differs")
        if mission.get("id") != mid or mission.get("discipline") != adventure["primaryDiscipline"]:
            fail(f"Adventure {aid} mission identity/discipline mismatch")
        if mission.get("analyticsClassification") != "minimized-learning-evidence":
            fail(f"Adventure {aid} must use minimized learning evidence")
        mission_runtime = mission.get("runtime", {})
        if mission_runtime.get("evaluator") != "composable" or mission_runtime.get("prototypeOnly") is not True:
            fail(f"Adventure {aid} must map to a composable prototype mission")

        rewards = mission_runtime.get("rewardIds", [])
        for reward in rewards:
            lowered = reward.lower()
            if any(token in lowered for token in FORBIDDEN_REWARD_TOKENS):
                fail(f"Adventure {aid} reward violates calm-progression policy: {reward}")

        beats = adventure.get("beats", [])
        requirements = mission_runtime.get("evidencePrimitives", [])
        if not beats or len(beats) != len(requirements):
            fail(f"Adventure {aid} beats must map 1:1 and in order to mission evidence primitives")
        beat_ids: set[str] = set()
        event_ids: set[str] = set()
        for index, (beat, requirement) in enumerate(zip(beats, requirements)):
            beat_count += 1
            beat_id = beat.get("beatId")
            event_id = beat.get("evidenceEventId")
            if not isinstance(beat_id, str) or not beat_id.startswith("beat.") or beat_id in beat_ids:
                fail(f"Adventure {aid} has invalid/duplicate beat {beat_id!r}")
            if not isinstance(event_id, str) or not event_id or event_id in event_ids:
                fail(f"Adventure {aid} has invalid/duplicate evidence event {event_id!r}")
            beat_ids.add(beat_id)
            event_ids.add(event_id)
            if beat.get("producerKind") not in ALLOWED_PRODUCERS or not beat.get("producerRefId"):
                fail(f"Adventure {aid} beat {beat_id} has invalid producer identity")
            if not str(beat.get("promptKey", "")).startswith("adventure.") or not str(beat.get("formalizationKey", "")).startswith("adventure."):
                fail(f"Adventure {aid} beat {beat_id} lacks prompt/formalization keys")
            for field in ("primitiveId", "evidenceEventId"):
                if beat.get(field) != requirement.get(field):
                    fail(f"Adventure {aid} beat {index} does not match mission requirement field {field}")
            if beat.get("requiredCount") != requirement.get("requiredCount", 1):
                fail(f"Adventure {aid} beat {index} requiredCount differs from mission requirement")
            if beat["producerKind"] == "thought" and beat["producerRefId"] not in thought_refs:
                fail(f"Adventure {aid} references unknown M5.4 thought producer {beat['producerRefId']}")
            if beat["producerKind"] == "science" and not beat["producerRefId"].startswith(("science.", "reaction.", "plant.")):
                fail(f"Adventure {aid} science producer must use a stable science/reaction/plant ID")

    if beat_count < 20:
        fail(f"M5.5 pack is too shallow: expected at least 20 ordered beats, got {beat_count}")

    runtime_h = read("game/Source/WorldMakers/Adventure/WMAdventureRuntime.h")
    runtime_cpp = read("game/Source/WorldMakers/Adventure/WMAdventureRuntime.cpp")
    subsystem_h = read("game/Source/WorldMakers/Adventure/WMAdventureRuntimeSubsystem.h")
    subsystem_cpp = read("game/Source/WorldMakers/Adventure/WMAdventureRuntimeSubsystem.cpp")
    for token in ("ordered-beats", "CanAcceptEvidence", "CommitEvidence", "ProducerKind", "ProducerRefId", "stable-ids-no-child-free-text"):
        if token not in runtime_h and token not in runtime_cpp:
            fail(f"M5.5 ordered runtime missing contract token: {token}")
    for token in ("ActivateAdventure", "RecordAdventureEvidence", "RecordComposableEvidence", "first-fantastic-adventure-pack-v1.json"):
        if token not in subsystem_h and token not in subsystem_cpp:
            fail(f"M5.5 subsystem missing integration token: {token}")

    thought_subsystem = read("game/Source/WorldMakers/Thought/WMLanguageThoughtSubsystem.cpp")
    for token in ("UWMAdventureRuntimeSubsystem", "RecordAdventureEvidence", "RecordComposableEvidence"):
        if token not in thought_subsystem:
            fail(f"M5.5 must route M5.4 evidence through adventures with backward compatibility: {token}")

    tests = read("game/Source/WorldMakers/Private/Tests/WMFantasticAdventurePackTests.cpp")
    for test_name in (
        "WorldMakers.Adventure.Pack.CatalogHasElevenStreams",
        "WorldMakers.Adventure.Pack.OrderedEvidenceGate",
        "WorldMakers.Adventure.Pack.StableProducerEvidenceOnly",
    ):
        if test_name not in tests:
            fail(f"Missing M5.5 automation test: {test_name}")

    game_ini = read("game/Config/DefaultGame.ini")
    if 'DirectoriesToAlwaysStageAsNonUFS=(Path="WorldMakers/Adventures")' not in game_ini:
        fail("Packaged builds must stage WorldMakers/Adventures")

    workflow = read(".github/workflows/repo-quality.yml")
    if "python scripts/validate-m5-5-fantastic-adventure-pack.py" not in workflow:
        fail("Repository Quality must execute M5.5 source gate")

    docs = read("docs/m5-5-first-fantastic-adventure-pack.md")
    for token in ("11 adventures", "22 ordered beats", "M5.3", "M5.4", "ordered", "source-complete", "not final 3D production"):
        if token not in docs:
            fail(f"M5.5 documentation missing contract token: {token}")

    print(f"M5.5 First Fantastic Adventure Pack passed: 11 adventures, {beat_count} ordered beats, all 11 learning streams, mission parity, producer gates and calm/privacy boundaries are wired.")


if __name__ == "__main__":
    main()
