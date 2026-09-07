#!/usr/bin/env python3
"""Source-level M1.9 runtime-certification infrastructure gates."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    raise SystemExit(message)


def read(relative: str) -> str:
    path = ROOT / relative
    if not path.is_file():
        fail(f"missing M1.9 file: {relative}")
    return path.read_text(encoding="utf-8")


def main() -> None:
    preflight = read("scripts/preflight-unreal-runner.ps1")
    build = read("scripts/build-unreal.ps1")
    tests = read("scripts/test-unreal.ps1")
    collect = read("scripts/collect-unreal-certification-evidence.ps1")
    workflow = read(".github/workflows/unreal-ci.yml")
    docs = read("docs/m1-9-runtime-certification.md")
    certification = read("scripts/validate-m1-certification.py")

    for token in (
        "runner-preflight.json",
        "UNREAL_ENGINE_VERSION",
        "UnrealEditor-Cmd.exe",
        "git lfs version",
        "WM_PrototypeCertification.umap",
    ):
        if token not in preflight:
            fail(f"runner preflight contract missing: {token}")

    for token in ("build.log", "build-result.json", "Tee-Object"):
        if token not in build:
            fail(f"build evidence contract missing: {token}")

    for token in ("WorldMakers.", "automation.log", "automation-result.json", "Tee-Object"):
        if token not in tests:
            fail(f"automation evidence contract missing: {token}")

    for token in (
        "certification-manifest.json",
        "runtimeCertification",
        "native-pass-manual-smoke-pending",
        "Get-FileHash",
        "manual-smoke.json",
        "RequireNativePass",
    ):
        if token not in collect:
            fail(f"certification manifest contract missing: {token}")

    for token in (
        "Preflight locked Unreal runner",
        "Run complete World Makers automation suite",
        "Collect certification evidence",
        "actions/upload-artifact@v4",
        "if: always()",
        "retention-days: 30",
        "WorldMakers.",
        "Validate M1.9 Runtime Certification Infrastructure",
    ):
        if token not in workflow:
            fail(f"native workflow contract missing: {token}")

    if "WorldMakers.Building; Quit" in tests:
        fail("M1.9 must not restrict native certification to Building tests only")

    if "WM_PrototypeCertification.umap" not in certification:
        fail("certification validator must require the exact certification map")

    for phrase in (
        "SOURCE GREEN IS NOT RUNTIME CERTIFIED",
        "manual smoke",
        "UNREAL_SELF_HOSTED_ENABLED",
        "UNREAL_ENGINE_ROOT",
        "WorldMakers.*",
    ):
        if phrase not in docs:
            fail(f"M1.9 operator documentation missing: {phrase}")

    print("M1.9 runtime-certification infrastructure source validation passed")


if __name__ == "__main__":
    main()
