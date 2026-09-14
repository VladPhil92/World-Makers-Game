#!/usr/bin/env python3
"""M5.6G durable epic sync outbox and conflict reconciliation source contract."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    raise SystemExit(message)


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require_tokens(label: str, text: str, tokens: tuple[str, ...]) -> None:
    for token in tokens:
        if token not in text:
            fail(f"{label} missing required token: {token}")


def main() -> None:
    required = (
        "docs/m5-6g-durable-epic-sync-outbox.md",
        "game/Source/WorldMakers/Adventure/WMEpicJourneySaveGame.h",
        "game/Source/WorldMakers/Adventure/WMEpicRuntimeSubsystem.h",
        "game/Source/WorldMakers/Adventure/WMEpicRuntimeSubsystem.cpp",
        "game/Source/WorldMakers/Launch/WMLaunchBootstrapSubsystem.h",
        "game/Source/WorldMakers/Launch/WMLaunchBootstrapSubsystem.cpp",
        "game/Source/WorldMakers/Private/Tests/WMEpicPersistenceTests.cpp",
        "apps/player-dashboard/src/domain/epic-progress.mjs",
        "apps/player-dashboard/src/server.mjs",
        "apps/player-dashboard/tests/epic-progress.test.mjs",
        ".github/workflows/repo-quality.yml",
        ".github/workflows/unreal-ci.yml",
    )
    missing = [path for path in required if not (ROOT / path).is_file()]
    if missing:
        fail(f"Missing M5.6G files: {missing}")

    savegame = read("game/Source/WorldMakers/Adventure/WMEpicJourneySaveGame.h")
    require_tokens(
        "Epic SaveGame v2",
        savegame,
        (
            "MinimumSupportedFormatVersion = 1",
            "CurrentFormatVersion = 2",
            "TArray<FWMEpicCheckpoint> PendingSyncCheckpoints",
            "UPROPERTY(SaveGame)",
        ),
    )
    for forbidden in ("FString LaunchTicket", "FString ProgressSyncToken", "PlayerProfileId", "EvidenceEventId", "FreeText"):
        if forbidden in savegame:
            fail(f"Durable epic outbox must not persist credential/identity/evidence field: {forbidden}")

    runtime_h = read("game/Source/WorldMakers/Adventure/WMEpicRuntimeSubsystem.h")
    runtime_cpp = read("game/Source/WorldMakers/Adventure/WMEpicRuntimeSubsystem.cpp")
    runtime = runtime_h + "\n" + runtime_cpp
    require_tokens(
        "Durable epic runtime outbox",
        runtime,
        (
            "PendingSyncCheckpoints",
            "QueuePendingSyncCheckpoint",
            "FindPendingSyncCheckpointIndex",
            "AcknowledgeEpicCheckpointSync",
            "FlushPendingEpicCheckpointSyncs(FName EpicId)",
            "MinimumSupportedFormatVersion",
            "Save->FormatVersion >= 2",
            "Save->PendingSyncCheckpoints = PendingSyncCheckpoints",
            "const TArray<FWMEpicCheckpoint> PreviousPendingSyncCheckpoints = PendingSyncCheckpoints",
            "PendingSyncCheckpoints = PreviousPendingSyncCheckpoints",
            "IsAtLeastAsAdvanced",
        ),
    )

    save_current = runtime_cpp.split("bool UWMEpicRuntimeSubsystem::SaveCurrentCheckpoint", 1)[1].split(
        "bool UWMEpicRuntimeSubsystem::ActivateEpic", 1
    )[0]
    queue_pos = save_current.find("QueuePendingSyncCheckpoint(Checkpoint)")
    save_pos = save_current.find("SaveEpicCheckpoints()")
    sync_pos = save_current.find("SyncEpicCheckpoint(Checkpoint)")
    if min(queue_pos, save_pos, sync_pos) < 0 or not queue_pos < save_pos < sync_pos:
        fail("M5.6G must persist the coalesced outbox before dispatching remote sync")

    flush_section = runtime_cpp.split("bool UWMEpicRuntimeSubsystem::FlushPendingEpicCheckpointSyncs", 1)[1].split(
        "bool UWMEpicRuntimeSubsystem::RecordEpicEvidence", 1
    )[0]
    if "FindPendingSyncCheckpointIndex(EpicId)" not in flush_section or "SyncEpicCheckpoint(PendingSyncCheckpoints[PendingIndex])" not in flush_section:
        fail("Outbox retry must target only the currently authorized epic checkpoint")

    launch_h = read("game/Source/WorldMakers/Launch/WMLaunchBootstrapSubsystem.h")
    launch_cpp = read("game/Source/WorldMakers/Launch/WMLaunchBootstrapSubsystem.cpp")
    require_tokens(
        "Native outbox acknowledgement",
        launch_h + "\n" + launch_cpp,
        (
            "FName ProgressSyncEpicId",
            "ProgressSyncEpicId = ParsedResume.IsSet() ? ParsedResume.EpicId : NAME_None",
            "Checkpoint.EpicId != ProgressSyncEpicId",
            "StatusCode < 200 || StatusCode >= 300",
            "AcknowledgeEpicCheckpointSync(Checkpoint)",
            "FlushPendingEpicCheckpointSyncs(EpicResume.EpicId)",
            "if (bDispatched && ProgressSyncEpicId.IsNone()) ProgressSyncEpicId = Checkpoint.EpicId",
        ),
    )
    if "UPROPERTY" in launch_h.split("FString ProgressSyncToken", 1)[-1].split("FName ProgressSyncEpicId", 1)[0]:
        fail("Progress sync credential must remain process-memory-only and non-reflected")

    epic_progress = read("apps/player-dashboard/src/domain/epic-progress.mjs")
    require_tokens(
        "Server epic reconciliation",
        epic_progress,
        (
            "reconcileProfileEpicProgress",
            "existing ? 'advanced' : 'created'",
            "disposition: 'idempotent'",
            "disposition: 'server-ahead'",
            "changed: false",
            "if (next.progress.currentEpicId === incoming.epicId) next.progress.currentEpicId = null",
        ),
    )

    server = read("apps/player-dashboard/src/server.mjs")
    require_tokens(
        "Native checkpoint reconciliation endpoint",
        server,
        (
            "reconcileProfileEpicProgress",
            "const reconciliation = reconcileProfileEpicProgress(profile, candidate)",
            "reconciliation.changed",
            "disposition: reconciliation.disposition",
            "authoritativeCheckpoint: nativeAuthoritativeCheckpoint(reconciliation.authoritative)",
        ),
    )
    authoritative = server.split("function nativeAuthoritativeCheckpoint", 1)[1].split("async function handleApi", 1)[0]
    for required_field in ("epicId", "chapterId", "chapterIndex", "state"):
        if required_field not in authoritative:
            fail(f"Native authoritative acknowledgement missing field: {required_field}")
    for forbidden in ("objectiveSummary", "updatedAt", "playerId", "evidence"):
        if forbidden in authoritative:
            fail(f"Native authoritative acknowledgement is too broad: {forbidden}")

    unreal_tests = read("game/Source/WorldMakers/Private/Tests/WMEpicPersistenceTests.cpp")
    require_tokens(
        "Native durable SaveGame test",
        unreal_tests,
        (
            "WorldMakers.Epic.Persistence.NativeSaveGameRoundTrip",
            "Save->PendingSyncCheckpoints.Add(Checkpoint)",
            "Exactly one pending sync checkpoint round-trips",
            "Outbox epic id round-trips",
        ),
    )

    node_tests = read("apps/player-dashboard/tests/epic-progress.test.mjs")
    require_tokens(
        "M5.6G reconciliation tests",
        node_tests,
        (
            "native checkpoint retry is idempotent without a profile mutation",
            "stale native retry accepts server-ahead progress without regression",
            "newer native retry advances the server exactly once",
            "completing a non-current epic preserves the current epic pointer",
        ),
    )

    docs = read("docs/m5-6g-durable-epic-sync-outbox.md").lower()
    for boundary in (
        "format version 1",
        "format version 2",
        "pendingsynccheckpoints",
        "local-first atomicity",
        "idempotent",
        "server-ahead",
        "epic-bound",
        "issue #9",
        "does **not** claim packaged crash/restart",
    ):
        if boundary not in docs:
            fail(f"M5.6G documentation missing boundary: {boundary}")

    command = "python scripts/validate-m5-6g-durable-epic-sync-outbox.py"
    repo_quality = read(".github/workflows/repo-quality.yml")
    unreal_ci = read(".github/workflows/unreal-ci.yml")
    if command not in repo_quality:
        fail("Repository Quality must run M5.6G validation")
    if command not in unreal_ci:
        fail("Unreal project-validation must run M5.6G validation")

    print(
        "M5.6G passed: SaveGame-v2 durable coalesced checkpoint outbox, epic-bound retry, "
        "2xx acknowledgement and idempotent/server-ahead reconciliation are source-wired without persisting native credentials."
    )


if __name__ == "__main__":
    main()