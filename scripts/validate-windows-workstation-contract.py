#!/usr/bin/env python3
"""Hosted regression checks for the Windows Unreal workstation bootstrap contract.

This validator is intentionally static: GitHub-hosted Linux runners cannot prove
native UE readiness, but they can prevent regressions that reintroduce automatic
installers, opaque Live Coding failures, or PowerShell-7-only syntax.
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
    "doctor_launcher": ROOT / "WorldMakers-Doctor.cmd",
    "readiness_launcher": ROOT / "WorldMakers-Readiness.cmd",
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
    doctor_launcher = read("doctor_launcher")
    readiness_launcher = read("readiness_launcher")
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
        (
            "ExpectedVersion",
            "Build.version",
            "UNREAL_ENGINE_ROOT",
            "EpicGamesLauncher",
        ),
        "scripts/resolve-unreal-engine.ps1",
    )

    for name, text in (("build", build), ("test", test)):
        require(
            text,
            (
                "resolve-unreal-engine.ps1",
                "LiveCodingConsole",
                "blocking-unreal-process",
                "StopBlockingProcesses",
            ),
            f"scripts/{'build-unreal.ps1' if name == 'build' else 'test-unreal.ps1'}",
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
        doctor_launcher,
        (
            "diagnose-unreal-workstation.ps1",
            "ExecutionPolicy Bypass",
            "workstation-doctor.json",
        ),
        "WorldMakers-Doctor.cmd",
    )
    require(
        readiness_launcher,
        (
            "run-unreal-readiness-gate.ps1",
            "-CleanIntermediate",
            "Do not reinstall Unreal Engine",
        ),
        "WorldMakers-Readiness.cmd",
    )

    require(
        doc,
        (
            "WorldMakers-Doctor.cmd",
            "WorldMakers-Readiness.cmd",
            "workstation-doctor.json",
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
    }
    forbidden_ps7_tokens = ("??", "?.", "&&", "||", "ForEach-Object -Parallel")
    for name, text in powershell_files.items():
        found = [token for token in forbidden_ps7_tokens if token in text]
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
    mutable_scripts = doctor + readiness + build + test
    found_installers = [token for token in automatic_install_tokens if token.lower() in mutable_scripts.lower()]
    if found_installers:
        fail(f"Windows workstation flow must remain diagnostic/manual-only; automatic installer token found: {found_installers}")

    print(
        "World Makers Windows workstation contract passed: "
        "diagnostic-first, no-auto-install, engine auto-resolution, Live Coding blocker classification, "
        "Visual Studio/SDK verification, Windows PowerShell 5.1 compatibility, CMD launchers present."
    )


if __name__ == "__main__":
    main()
