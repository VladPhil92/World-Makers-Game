#!/usr/bin/env python3
"""M5.6F native bootstrap + durable epic progress sync source contract."""

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
        "docs/m5-6f-native-bootstrap-progress-sync.md",
        "docs/player-dashboard-d3-native-launch-handoff.md",
        "apps/player-dashboard/src/domain/native-launch-session.mjs",
        "apps/player-dashboard/src/domain/native-handoff.mjs",
        "apps/player-dashboard/src/domain/epic-progress.mjs",
        "apps/player-dashboard/src/server.mjs",
        "apps/player-dashboard/tests/native-launch-session.test.mjs",
        "apps/player-dashboard/tests/native-handoff.test.mjs",
        "game/Source/WorldMakers/Launch/WMLaunchBootstrapSubsystem.h",
        "game/Source/WorldMakers/Launch/WMLaunchBootstrapSubsystem.cpp",
        "game/Source/WorldMakers/Adventure/WMEpicRuntimeSubsystem.h",
        "game/Source/WorldMakers/Adventure/WMEpicRuntimeSubsystem.cpp",
        "game/Source/WorldMakers/Adventure/WMEclipseEngineExperienceSubsystem.cpp",
        "game/Source/WorldMakers/Adventure/WMGardenEndWinterExperienceSubsystem.cpp",
        "game/Source/WorldMakers/Private/Tests/WMLaunchBootstrapTests.cpp",
        "game/Source/WorldMakers/WorldMakers.Build.cs",
        "game/Config/DefaultGame.ini",
        ".github/workflows/repo-quality.yml",
        ".github/workflows/unreal-ci.yml",
    )
    missing = [path for path in required if not (ROOT / path).is_file()]
    if missing:
        fail(f"Missing M5.6F files: {missing}")

    session_store = read("apps/player-dashboard/src/domain/native-launch-session.mjs")
    require_tokens(
        "Native launch session store",
        session_store,
        (
            "randomBytes(TOKEN_BYTES).toString('base64url')",
            "createHash('sha256')",
            "this.#launchTickets.delete(key)",
            "epic-checkpoint:write",
            "expiresAtMs <= now",
            "session.epicId !== null && session.epicId !== normalizedEpicId",
            "session.epicId === null",
        ),
    )
    if "TOKEN_BYTES = 32" not in session_store:
        fail("Native launch tickets must use at least 32 random bytes")

    handoff = read("apps/player-dashboard/src/domain/native-handoff.mjs")
    require_tokens(
        "Native v2 handoff",
        handoff,
        (
            "worldmakers-launch-v2",
            "createNativeLaunchTicketUri",
            "decodeNativeLaunchTicketUri",
            "parsed.searchParams.has('payload')",
            "Ticket launch URI must not embed player context.",
        ),
    )

    server = read("apps/player-dashboard/src/server.mjs")
    require_tokens(
        "Player dashboard native bootstrap API",
        server,
        (
            "/api/native/launch/redeem",
            "nativeLaunchStore.redeem(body.ticket)",
            "verifyLaunchContext(redeemed.context, launchSigningSecret)",
            "/api/native/epic-checkpoint",
            "nativeLaunchStore.authorizeProgressSync",
            "objectiveSummary: existing?.objectiveSummary ?? []",
            "updatedAt: new Date().toISOString()",
            "protocol: 'worldmakers-launch-v2'",
            "createNativeLaunchTicketUri(issued.ticket)",
        ),
    )
    if not any(
        token in server
        for token in (
            "updateProfileEpicCheckpoint(profile, candidate)",
            "reconcileProfileEpicProgress(profile, candidate)",
        )
    ):
        fail("Native checkpoint API must persist through the monotonic M5.6F/M5.6G epic progress boundary")
    if "new Set(['epicId', 'chapterId', 'chapterIndex', 'state'])" not in server:
        fail("Native checkpoint API must expose only the four approved checkpoint fields")
    if "Epic checkpoint contains unsupported fields." not in server:
        fail("Native checkpoint API must reject expanded payloads")

    epic_progress = read("apps/player-dashboard/src/domain/epic-progress.mjs")
    require_tokens(
        "Epic progress strict state validation",
        epic_progress,
        (
            "input.state !== STATE_IN_PROGRESS && input.state !== STATE_COMPLETE",
            "throw new TypeError('state is invalid.')",
            "const state = input.state",
        ),
    )

    launch_header = read("game/Source/WorldMakers/Launch/WMLaunchBootstrapSubsystem.h")
    launch_cpp = read("game/Source/WorldMakers/Launch/WMLaunchBootstrapSubsystem.cpp")
    native_launch = launch_header + "\n" + launch_cpp
    require_tokens(
        "Unreal native launch bootstrap",
        native_launch,
        (
            "worldmakers-launch-v2",
            "WORLD_MAKERS_API_BASE_URL",
            "https://",
            "#if !UE_BUILD_SHIPPING",
            "http://localhost",
            "http://127.0.0.1",
            "/api/native/launch/redeem",
            "epic-checkpoint:write",
            "/api/native/epic-checkpoint",
            "TryExtractTicketFromCommandLine",
            "TryApplyEpicResume",
            "SyncEpicCheckpoint",
        ),
    )
    if "WORLD_MAKERS_LAUNCH_SIGNING_SECRET" in native_launch or "createHmac" in native_launch or "HMAC" in launch_cpp:
        fail("Server HMAC secret/implementation must not be embedded in the Unreal native bootstrap")
    if "UPROPERTY" in launch_header.split("FString ProgressSyncToken", 1)[0].split("FString LaunchTicket", 1)[-1]:
        # The credentials intentionally remain plain transient C++ members, not serialized UPROPERTY state.
        fail("Launch/sync credentials must remain process-memory-only and outside reflected persistence")

    epic_h = read("game/Source/WorldMakers/Adventure/WMEpicRuntimeSubsystem.h")
    epic_cpp = read("game/Source/WorldMakers/Adventure/WMEpicRuntimeSubsystem.cpp")
    require_tokens(
        "Epic runtime external checkpoint merge",
        epic_h + "\n" + epic_cpp,
        (
            "ApplyExternalResumeCheckpoint",
            "Existing.bCompleted || Existing.ChapterIndex >= Incoming.ChapterIndex",
            "const TArray<FWMEpicCheckpoint> PreviousCheckpoints = Checkpoints",
            "Checkpoints = PreviousCheckpoints",
            "Launch->SyncEpicCheckpoint(Checkpoint)",
        ),
    )
    save_section = epic_cpp.split("bool UWMEpicRuntimeSubsystem::SaveCurrentCheckpoint", 1)[1].split("bool UWMEpicRuntimeSubsystem::ActivateEpic", 1)[0]
    if save_section.find("SaveEpicCheckpoints()") > save_section.find("SyncEpicCheckpoint(Checkpoint)"):
        fail("Remote sync must happen only after local SaveGame succeeds")

    for path in (
        "game/Source/WorldMakers/Adventure/WMEclipseEngineExperienceSubsystem.cpp",
        "game/Source/WorldMakers/Adventure/WMGardenEndWinterExperienceSubsystem.cpp",
    ):
        experience = read(path)
        require_tokens(
            f"Native launch auto-start gate in {path}",
            experience,
            (
                "Launch->IsNativeLaunchRequested()",
                "Launch->IsNativeLaunchReady()",
                "Launch->TryApplyEpicResume()",
                "return;",
            ),
        )

    tests = read("game/Source/WorldMakers/Private/Tests/WMLaunchBootstrapTests.cpp")
    require_tokens(
        "Native launch bootstrap tests",
        tests,
        (
            "WorldMakers.Launch.Bootstrap.TicketUriParses",
            "WorldMakers.Launch.Bootstrap.RejectsPayloadAndLegacy",
            "worldmakers-launch-v2",
            "payload=forbidden",
        ),
    )

    build = read("game/Source/WorldMakers/WorldMakers.Build.cs")
    if '"HTTP"' not in build:
        fail("WorldMakers module must depend on Unreal HTTP for native bootstrap")

    config = read("game/Config/DefaultGame.ini")
    require_tokens(
        "Native launch configuration",
        config,
        (
            "[/Script/WorldMakers.WMLaunchBootstrapSubsystem]",
            "ApiBaseUrl=",
            "WORLD_MAKERS_API_BASE_URL",
        ),
    )
    section = config.split("[/Script/WorldMakers.WMLaunchBootstrapSubsystem]", 1)[1].split("[/Script/", 1)[0]
    for line in section.splitlines():
        if line.startswith("ApiBaseUrl=") and line.strip() != "ApiBaseUrl=":
            fail("Source configuration must not hardcode a production native API endpoint")

    node_tests = "\n".join(
        read(path)
        for path in (
            "apps/player-dashboard/tests/native-launch-session.test.mjs",
            "apps/player-dashboard/tests/native-handoff.test.mjs",
            "apps/player-dashboard/tests/epic-progress.test.mjs",
        )
    )
    require_tokens(
        "M5.6F Node tests",
        node_tests,
        (
            "single-use",
            "another epic",
            "payload-bearing v2",
            "unknown epic progress states fail closed",
        ),
    )

    docs = read("docs/m5-6f-native-bootstrap-progress-sync.md").lower()
    for boundary in (
        "one-time ticket",
        "server hmac secret",
        "memory",
        "local progress never regresses",
        "local-first persistence",
        "process-local memory",
        "shared ephemeral store",
        "issue #9",
        "source contract ready / native end-to-end evidence blocked",
    ):
        if boundary not in docs:
            fail(f"M5.6F documentation missing boundary: {boundary}")

    command = "python scripts/validate-m5-6f-native-bootstrap-progress-sync.py"
    repo_quality = read(".github/workflows/repo-quality.yml")
    unreal_ci = read(".github/workflows/unreal-ci.yml")
    if command not in repo_quality:
        fail("Repository Quality must run M5.6F validation")
    if command not in unreal_ci:
        fail("Unreal project-validation must run M5.6F validation")

    print(
        "M5.6F passed: one-time ticket native bootstrap, HTTPS trust boundary, local-first monotonic epic merge, "
        "scoped checkpoint-only sync and source-level fail-closed governance are wired without embedding server signing secrets."
    )


if __name__ == "__main__":
    main()