#!/usr/bin/env python3
"""Hosted regression checks for the Windows Unreal workstation bootstrap contract.

GitHub-hosted Linux runners cannot prove native UE readiness, but they can prevent
regressions that reintroduce automatic installers, opaque native failures,
unsafe editor launch behavior, or PowerShell-7-only syntax.
"""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "doctor": ROOT / "scripts" / "diagnose-unreal-workstation.ps1",
    "resolver": ROOT / "scripts" / "resolve-unreal-engine.ps1",
    "readiness": ROOT / "scripts" / "run-unreal-readiness-gate.ps1",
    "build": ROOT / "scripts" / "build-unreal.ps1",
    "test": ROOT / "scripts" / "test-unreal.ps1",
    "classifier": ROOT / "scripts" / "classify-unreal-build-log.py",
    "classifier_test": ROOT / "scripts" / "test-unreal-build-classifier.py",
    "editor_launcher": ROOT / "scripts" / "open-unreal-project.ps1",
    "doctor_launcher": ROOT / "WorldMakers-Doctor.cmd",
    "readiness_launcher": ROOT / "WorldMakers-Readiness.cmd",
    "open_editor_launcher": ROOT / "WorldMakers-OpenEditor.cmd",
    "failure_launcher": ROOT / "WorldMakers-DiagnoseLastFailure.cmd",
    "doc": ROOT / "docs" / "native-unreal-readiness-gate.md",
}


def fail(message: str) -> None:
    raise SystemExit(message)


def read(name: str) -> str:
    path = FILES[name]
    if not path.is_file():
        fail(f"Missing Windows workstation contract file: {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8")


def require(text: str, tokens: tuple[str, ...], subject: str) -> None:
    missing = [token for token in tokens if token not in text]
    if missing:
        fail(f"{subject} is missing workstation-contract tokens: {missing}")


def main() -> None:
    doctor = read("doctor")
    resolver = read("resolver")
    readiness = read("readiness")
    build = read("build")
    test = read("test")
    classifier = read("classifier")
    classifier_test = read("classifier_test")
    editor_launcher = read("editor_launcher")
    doctor_launcher = read("doctor_launcher")
    readiness_launcher = read("readiness_launcher")
    open_editor_launcher = read("open_editor_launcher")
    failure_launcher = read("failure_launcher")
    doc = read("doc")

    require(
        doctor,
        (
            "workstation-doctor.json",
            "manual-only-no-auto-install",
            "LiveCodingConsole",
            "blocking-unreal-process",
            "StopBlockingProcesses",
            "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
            "Windows Kits\\10",
            "Epic Games Launcher > Library > Verify",
            "Do not reinstall Unreal Engine",
        ),
        "scripts/diagnose-unreal-workstation.ps1",
    )

    require(
        resolver,
        ("ExpectedVersion", "Build.version", "UNREAL_ENGINE_ROOT", "EpicGamesLauncher"),
        "scripts/resolve-unreal-engine.ps1",
    )

    require(
        build,
        (
            "resolve-unreal-engine.ps1",
            "LiveCodingConsole",
            "blocking-unreal-process",
            "StopBlockingProcesses",
            "classify-unreal-build-log.py",
            "native-failure-summary.json",
            "primaryFailureCategory",
            "schemaVersion = 3",
            "$ClassifierOutput = @(& $Python.Source",
            "$ClassifierExitCode = $LASTEXITCODE",
        ),
        "scripts/build-unreal.ps1",
    )

    require(
        test,
        (
            "resolve-unreal-engine.ps1",
            "LiveCodingConsole",
            "blocking-unreal-process",
            "StopBlockingProcesses",
        ),
        "scripts/test-unreal.ps1",
    )

    require(
        classifier,
        (
            "live-coding-active",
            "visual-studio-toolchain-missing",
            "windows-sdk-missing",
            "include-file-missing",
            "unreal-header-tool-error",
            "compiler-error",
            "linker-error",
            "plugin-or-module-error",
            "file-lock-or-access-denied",
            "out-of-memory",
            "git-lfs-or-binary-pointer-error",
            "unknown-native-build-failure",
            "absoluteRepositoryPathRedacted",
            "homePathRedacted",
            "uploadsData",
        ),
        "scripts/classify-unreal-build-log.py",
    )
    executable_classifier = "\n".join(
        line for line in classifier.splitlines() if not line.lstrip().startswith("#")
    )
    if "OtherCompilationError" in executable_classifier:
        fail("Generic OtherCompilationError must not be used as an UnrealHeaderTool classifier signature.")

    require(
        classifier_test,
        (
            "classify_text",
            "live-coding-active",
            "include-file-missing",
            "unreal-header-tool-error",
            "compiler-error",
            "linker-error",
            "unknown-native-build-failure",
            "OtherCompilationError (5)",
            "JuanPablo",
            "<user>",
        ),
        "scripts/test-unreal-build-classifier.py",
    )

    require(
        readiness,
        (
            "diagnose-unreal-workstation.ps1",
            "workstationDoctor",
            "workstation-doctor.json",
            "manual-only-no-auto-install",
            "StopBlockingProcesses",
            "Do not reinstall Unreal",
        ),
        "scripts/run-unreal-readiness-gate.ps1",
    )

    require(
        editor_launcher,
        (
            "run-unreal-readiness-gate.ps1",
            "readiness-result.json",
            "workstation-doctor.json",
            "native-failure-summary.json",
            "UNREAL EDITOR NOT OPENED",
            "UnrealEditor.exe",
            "Start-Process",
            "editor-launch-result.json",
            "manual-only-no-auto-install",
            "Remove-Item $StaleEvidence -Force",
            "nativeBuildStatus",
        ),
        "scripts/open-unreal-project.ps1",
    )

    require(
        doctor_launcher,
        ("diagnose-unreal-workstation.ps1", "ExecutionPolicy Bypass", "workstation-doctor.json"),
        "WorldMakers-Doctor.cmd",
    )
    require(
        readiness_launcher,
        ("run-unreal-readiness-gate.ps1", "-CleanIntermediate", "Do not reinstall Unreal Engine"),
        "WorldMakers-Readiness.cmd",
    )
    require(
        open_editor_launcher,
        ("open-unreal-project.ps1", "ExecutionPolicy Bypass", "native-failure-summary.json"),
        "WorldMakers-OpenEditor.cmd",
    )
    require(
        failure_launcher,
        ("classify-unreal-build-log.py", "native-failure-summary.json", "--print"),
        "WorldMakers-DiagnoseLastFailure.cmd",
    )

    require(
        doc,
        (
            "WorldMakers-Doctor.cmd",
            "WorldMakers-Readiness.cmd",
            "WorldMakers-OpenEditor.cmd",
            "WorldMakers-DiagnoseLastFailure.cmd",
            "workstation-doctor.json",
            "native-failure-summary.json",
            "blocking-unreal-process",
            "Live Coding",
            "Do not reinstall",
            "Visual Studio Installer",
        ),
        "docs/native-unreal-readiness-gate.md",
    )

    powershell_files = {
        "doctor": doctor,
        "resolver": resolver,
        "readiness": readiness,
        "build": build,
        "test": test,
        "editor_launcher": editor_launcher,
    }
    forbidden_ps7_tokens = ("??", "?.", "&&", "||", "ForEach-Object -Parallel")
    for name, source in powershell_files.items():
        found = [token for token in forbidden_ps7_tokens if token in source]
        if found:
            fail(f"{name} uses PowerShell-7-only syntax incompatible with Windows PowerShell 5.1: {found}")

    automatic_install_tokens = (
        "winget install",
        "choco install",
        "Install-Package",
        "Start-Process msiexec",
        "Invoke-WebRequest http",
        "Invoke-RestMethod http",
    )
    mutable_scripts = doctor + readiness + build + test + editor_launcher
    found_installers = [token for token in automatic_install_tokens if token.lower() in mutable_scripts.lower()]
    if found_installers:
        fail(f"Windows workstation flow must remain diagnostic/manual-only; automatic installer token found: {found_installers}")

    if "Start-Process -FilePath $EditorExe" not in editor_launcher:
        fail("Editor launcher must open the exact resolved UnrealEditor.exe rather than an arbitrary file association.")
    if "if ($ReadinessExitCode -ne 0)" not in editor_launcher:
        fail("Editor launcher must fail closed when native readiness fails.")

    print(
        "World Makers Windows workstation contract passed: diagnostic-first, no-auto-install, "
        "native failure classification, scalar classifier output, current-run evidence only, guarded editor launch, "
        "engine auto-resolution, Live Coding blocker classification, Visual Studio/SDK verification, "
        "Windows PowerShell 5.1 compatibility, CMD launchers present."
    )


if __name__ == "__main__":
    main()
