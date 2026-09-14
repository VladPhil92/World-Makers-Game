#!/usr/bin/env python3
"""M5.6E native epic continuity certification source contract."""

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
        "docs/m5-6e-native-epic-continuity-certification.md",
        "game/Source/WorldMakers/Private/Tests/WMEpicPersistenceTests.cpp",
        "scripts/collect-m5-6e-epic-evidence.ps1",
        "scripts/assess-m5-6e-epic-certification.py",
        "scripts/validate-m5-6e-native-epic-certification.py",
        ".github/workflows/m5-6e-epic-native-certification.yml",
        ".github/workflows/repo-quality.yml",
        ".github/workflows/unreal-ci.yml",
    )
    missing = [path for path in required if not (ROOT / path).is_file()]
    if missing:
        fail(f"Missing M5.6E files: {missing}")

    tests = read("game/Source/WorldMakers/Private/Tests/WMEpicPersistenceTests.cpp")
    require_tokens(
        "Epic persistence native tests",
        tests,
        (
            "WorldMakers.Epic.Persistence.ChapterCheckpointDropsPartialEvidence",
            "WorldMakers.Epic.Persistence.ResumeIndexFailClosed",
            "WorldMakers.Epic.Persistence.NativeSaveGameRoundTrip",
            "FGuid::NewGuid()",
            "UGameplayStatics::SaveGameToSlot",
            "UGameplayStatics::LoadGameFromSlot",
            "UGameplayStatics::DeleteGameInSlot",
            "UGameplayStatics::DoesSaveGameExist",
        ),
    )
    native_section = tests.split("FWMEpicNativeSaveGameRoundTripTest", 1)[1]
    if "WM_EpicJourney_v1" in native_section:
        fail("Native automation must use a unique test slot and never touch the production epic journey slot")

    collector = read("scripts/collect-m5-6e-epic-evidence.ps1")
    require_tokens(
        "M5.6E evidence collector",
        collector,
        (
            "WorldMakers.Epic.Persistence.",
            "WorldMakers.Epic.Persistence.NativeSaveGameRoundTrip",
            "game\\UNREAL_ENGINE_VERSION",
            "WM_PrototypeCertification.umap",
            "Get-FileHash -Algorithm SHA256",
            "repositoryCommit",
            "expectedUnrealVersion",
            "actualUnrealVersion",
            "missingTests",
            "privacyBoundary",
            "RequireNativePass",
            "nativeEpicContinuity",
        ),
    )

    assessor = read("scripts/assess-m5-6e-epic-certification.py")
    require_tokens(
        "M5.6E independent assessor",
        assessor,
        (
            "EXPECTED_FILTER = \"WorldMakers.Epic.Persistence.\"",
            "WorldMakers.Epic.Persistence.NativeSaveGameRoundTrip",
            "EXPECTED_MAP = \"game/Content/WorldMakers/Maps/WM_PrototypeCertification.umap\"",
            "sha256_file",
            "safe_path",
            "evidence commit does not match assessed commit",
            "native SaveGame round-trip test was not observed",
            "--require-pass",
        ),
    )

    workflow = read(".github/workflows/m5-6e-epic-native-certification.yml")
    require_tokens(
        "M5.6E workflow",
        workflow,
        (
            "source-contract:",
            "native-epic-continuity:",
            "vars.UNREAL_SELF_HOSTED_ENABLED == 'true'",
            "- self-hosted",
            "- Windows",
            "- X64",
            "- unreal",
            "lfs: true",
            "-RequireAuthoredMap",
            "WorldMakers.Epic.Persistence.",
            "collect-m5-6e-epic-evidence.ps1",
            "assess-m5-6e-epic-certification.py",
            "--require-pass",
            "actions/upload-artifact@v4",
            "retention-days: 30",
        ),
    )
    if "TestFilter 'WorldMakers.Epic.Persistence.'" not in workflow:
        fail("M5.6E native workflow must run the exact epic persistence automation filter")

    docs = read("docs/m5-6e-native-epic-continuity-certification.md").lower()
    for boundary in (
        "issue #9",
        "source harness ready / native execution blocked",
        "savegametoslot",
        "loadgamefromslot",
        "deletegameinslot",
        "partial evidence",
        "packaged-device certification",
    ):
        if boundary not in docs:
            fail(f"M5.6E documentation missing certification boundary: {boundary}")

    command = "python scripts/validate-m5-6e-native-epic-certification.py"
    repo_quality = read(".github/workflows/repo-quality.yml")
    unreal_ci = read(".github/workflows/unreal-ci.yml")
    if command not in repo_quality:
        fail("Repository Quality must run the M5.6E source validator")
    if command not in unreal_ci:
        fail("Unreal project-validation must run the M5.6E source validator")

    print(
        "M5.6E passed: native epic SaveGame round-trip, locked UE runner contract, "
        "hashed evidence collection and independent fail-closed assessment are source-wired; "
        "native execution remains gated by Issue #9 infrastructure."
    )


if __name__ == "__main__":
    main()
