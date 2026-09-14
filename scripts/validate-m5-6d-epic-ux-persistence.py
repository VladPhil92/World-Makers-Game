#!/usr/bin/env python3
"""M5.6D epic UX, checkpoint persistence and minimized progress projection source gate."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    raise SystemExit(message)


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def main() -> None:
    required = [
        "docs/m5-6d-epic-ux-persistence.md",
        "game/Source/WorldMakers/Adventure/WMEpicJourneySaveGame.h",
        "game/Source/WorldMakers/Adventure/WMEpicRuntime.h",
        "game/Source/WorldMakers/Adventure/WMEpicRuntime.cpp",
        "game/Source/WorldMakers/Adventure/WMEpicRuntimeSubsystem.h",
        "game/Source/WorldMakers/Adventure/WMEpicRuntimeSubsystem.cpp",
        "game/Source/WorldMakers/Adventure/WMEclipseEngineExperienceSubsystem.cpp",
        "game/Source/WorldMakers/Adventure/WMGardenEndWinterExperienceSubsystem.cpp",
        "game/Source/WorldMakers/Private/Tests/WMEpicPersistenceTests.cpp",
        "apps/player-dashboard/src/domain/epic-progress.mjs",
        "apps/player-dashboard/src/domain/profile.mjs",
        "apps/player-dashboard/src/domain/launch.mjs",
        "apps/player-dashboard/public/app.js",
        "apps/player-dashboard/tests/epic-progress.test.mjs",
        "apps/player-dashboard/tests/epic-launch.test.mjs",
        "apps/player-dashboard/tests/epic-profile-store.test.mjs",
        "apps/parent-portal/src/domain/dashboard.mjs",
        "apps/parent-portal/tests/epic-progress.test.mjs",
        "services/backend/contracts/parent-family-dashboard.schema.json",
    ]
    missing = [path for path in required if not (ROOT / path).is_file()]
    if missing:
        fail(f"Missing M5.6D files: {missing}")

    savegame = read("game/Source/WorldMakers/Adventure/WMEpicJourneySaveGame.h")
    for token in ("FWMEpicCheckpoint", "EpicId", "ChapterId", "ChapterIndex", "ChapterCount", "bCompleted", "UWMEpicJourneySaveGame"):
        if token not in savegame:
            fail(f"Epic checkpoint SaveGame missing: {token}")
    for forbidden in ("EvidenceEventId", "Answer", "FreeText", "MoralChoice", "Personality", "Ideology", "HintHistory"):
        if forbidden in savegame:
            fail(f"Epic SaveGame must remain chapter-level and privacy-minimized: {forbidden}")

    runtime_h = read("game/Source/WorldMakers/Adventure/WMEpicRuntime.h")
    runtime_cpp = read("game/Source/WorldMakers/Adventure/WMEpicRuntime.cpp")
    subsystem_h = read("game/Source/WorldMakers/Adventure/WMEpicRuntimeSubsystem.h")
    subsystem_cpp = read("game/Source/WorldMakers/Adventure/WMEpicRuntimeSubsystem.cpp")
    resume_body = runtime_h.split("bool ResumeAtChapter", 1)[1].split("bool CanAcceptEvidence", 1)[0] if "bool ResumeAtChapter" in runtime_h else ""
    if "Reset();" not in resume_body or "InDefinition.Chapters.IsValidIndex" not in resume_body:
        fail("ResumeAtChapter must validate the chapter and reset all partial progress before restore")
    reset_sources = runtime_h + "\n" + runtime_cpp
    if "CurrentEvidenceCounts.Reset()" not in reset_sources or "CurrentWorldStates.Reset()" not in reset_sources:
        fail("The progress model reset implementation must clear partial evidence and world state")
    for token in ("ResumeEpic", "ActivateOrResumeEpic", "HasResumableEpicCheckpoint", "ClearEpicCheckpoint", "SaveCurrentCheckpoint"):
        if token not in subsystem_h and token not in subsystem_cpp:
            fail(f"Epic persistence subsystem missing: {token}")
    for token in ("UGameplayStatics::LoadGameFromSlot", "UGameplayStatics::SaveGameToSlot", "IsCheckpointValidForCatalog", "ResumeAtChapter"):
        if token not in subsystem_cpp:
            fail(f"Epic persistence implementation missing: {token}")
    evidence_section = subsystem_cpp.split("bool UWMEpicRuntimeSubsystem::RecordEpicEvidence", 1)[1].split("bool UWMEpicRuntimeSubsystem::RecordEpicWorldState", 1)[0]
    if "SaveCurrentCheckpoint" in evidence_section:
        fail("Partial evidence must never trigger persistent checkpoints")

    for label, path in (
        ("Eclipse Engine", "game/Source/WorldMakers/Adventure/WMEclipseEngineExperienceSubsystem.cpp"),
        ("Garden at the End of Winter", "game/Source/WorldMakers/Adventure/WMGardenEndWinterExperienceSubsystem.cpp"),
    ):
        experience = read(path)
        if "ActivateOrResumeEpic(Catalog.EpicId)" not in experience:
            fail(f"{label} start path must resume a valid persisted epic checkpoint")
        if "ActivateEpic(Catalog.EpicId)" in experience:
            fail(f"{label} start path must not overwrite a resumable checkpoint with a fresh chapter-zero activation")

    unreal_tests = read("game/Source/WorldMakers/Private/Tests/WMEpicPersistenceTests.cpp")
    for name in (
        "WorldMakers.Epic.Persistence.ChapterCheckpointDropsPartialEvidence",
        "WorldMakers.Epic.Persistence.ResumeIndexFailClosed",
    ):
        if name not in unreal_tests:
            fail(f"Missing Unreal epic persistence test: {name}")

    epic_progress = read("apps/player-dashboard/src/domain/epic-progress.mjs")
    for token in (
        "epic.eclipse-engine", "epic.garden-end-winter", "buildPlayerEpicReadModel", "buildLaunchEpicResume",
        "buildParentEpicProjection", "updateProfileEpicProgress", "Puedes continuar desde este capítulo cuando quieras.",
    ):
        if token not in epic_progress:
            fail(f"Player epic progress domain missing: {token}")
    garden_definition = epic_progress.split("'epic.garden-end-winter': Object.freeze({", 1)[1].split("});\n\nconst STATE_IN_PROGRESS", 1)[0]
    persistent_groups = garden_definition.split("persistentObjectiveGroups:", 1)[1]
    for forbidden in ("'mathematics'", "'philosophy-for-children'"):
        if forbidden in persistent_groups:
            fail("Garden session-only mathematics/philosophy mastery must not enter persistent objective groups")

    profile = read("apps/player-dashboard/src/domain/profile.mjs")
    for token in ("normalizePlayerProgress", "epicJourney", "updateProfileEpicCheckpoint"):
        if token not in profile:
            fail(f"Player profile does not persist/project epic state: {token}")

    launch = read("apps/player-dashboard/src/domain/launch.mjs")
    canonical = launch.split("function canonicalPayload", 1)[1].split("function sign", 1)[0]
    if "epicResume: payload.epicResume" not in canonical or "epicResume: buildLaunchEpicResume" not in launch:
        fail("epicResume must be part of the signed canonical launch payload")

    browser = read("apps/player-dashboard/public/app.js")
    for token in ("epicJourney?.current", "Capítulo ${journey.chapterNumber} de ${journey.chapterCount}", "Continuar viaje"):
        if token not in browser:
            fail(f"Visible calm epic resume UX missing: {token}")

    player_tests = "\n".join(read(path) for path in (
        "apps/player-dashboard/tests/epic-progress.test.mjs",
        "apps/player-dashboard/tests/epic-launch.test.mjs",
        "apps/player-dashboard/tests/epic-profile-store.test.mjs",
    ))
    for token in ("tampering with epic resume", "cannot regress", "cuando quieras", "optimistic revision protection"):
        if token not in player_tests:
            fail(f"Player persistence/tamper test coverage missing: {token}")

    parent = read("apps/parent-portal/src/domain/dashboard.mjs")
    for token in ("safeEpics", "objectiveSummary", "chaptersCompleted"):
        if token not in parent:
            fail(f"Parent epic projection missing: {token}")
    for token in ("rawevidence", "evidenceeventid", "personalityscore", "ideologylabel"):
        if token not in parent.lower():
            fail(f"Parent minimization guard missing: {token}")
    if "epics: safeEpics(source.adventures)" not in parent:
        fail("Parent portal must reuse existing adventures aggregate rather than require a new child telemetry table")

    schema = json.loads(read("services/backend/contracts/parent-family-dashboard.schema.json"))
    if "epics" not in schema.get("required", []) or "epics" not in schema.get("properties", {}):
        fail("Parent dashboard schema must include closed epic summary")
    epic_item = schema["properties"]["epics"]["items"]
    if epic_item.get("additionalProperties") is not False:
        fail("Parent epic summary schema must reject unknown raw telemetry fields")

    parent_tests = read("apps/parent-portal/tests/epic-progress.test.mjs")
    for token in ("aggregate, bounded and label-authoritative", "strips raw evidence"):
        if token not in parent_tests:
            fail(f"Parent epic projection test coverage missing: {token}")

    docs = read("docs/m5-6d-epic-ux-persistence.md").lower()
    for idea in ("chapter checkpoint", "partial evidence", "cuando quieras", "session-only mastery", "m5.6e"):
        if idea not in docs:
            fail(f"M5.6D documentation missing boundary: {idea}")

    repo_quality = read(".github/workflows/repo-quality.yml")
    unreal_ci = read(".github/workflows/unreal-ci.yml")
    command = "python scripts/validate-m5-6d-epic-ux-persistence.py"
    if command not in repo_quality or command not in unreal_ci:
        fail("M5.6D validator must run in Repository Quality and Unreal project-validation")

    print("M5.6D passed: chapter-only resume, signed epic handoff, persistent calm player read model and minimized parent summaries are source-wired without persisting partial evidence or Garden session-only mastery gates.")


if __name__ == "__main__":
    main()
