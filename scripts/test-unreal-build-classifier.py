#!/usr/bin/env python3
"""Deterministic hosted tests for classify-unreal-build-log.py."""

from __future__ import annotations

import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CLASSIFIER_PATH = ROOT / "scripts" / "classify-unreal-build-log.py"

spec = importlib.util.spec_from_file_location("worldmakers_unreal_classifier", CLASSIFIER_PATH)
if spec is None or spec.loader is None:
    raise SystemExit("Unable to load classify-unreal-build-log.py")
classifier = importlib.util.module_from_spec(spec)
spec.loader.exec_module(classifier)


def expect(log: str, category: str, *, actionable: bool | None = None) -> None:
    result = classifier.classify_text(log, ROOT)
    assert result["primaryCategory"] == category, result
    if actionable is not None:
        assert result["repositoryActionable"] is actionable, result


def main() -> None:
    expect(
        "Unable to build while Live Coding is active. Exit the editor or press Ctrl+Alt+F11.\nResult: Failed",
        "live-coding-active",
        actionable=False,
    )
    expect(
        "ERROR: No Visual C++ installation was found.\nResult: Failed",
        "visual-studio-toolchain-missing",
        actionable=False,
    )
    expect(
        "Windows SDK must be installed in order to build this target.\nResult: Failed",
        "windows-sdk-missing",
        actionable=False,
    )
    expect(
        r"C:\Users\Builder\World-Makers-Game\game\Source\WorldMakers\Foo.cpp(17): fatal error C1083: Cannot open include file: 'Missing.h': No such file or directory\nResult: Failed",
        "include-file-missing",
        actionable=True,
    )
    expect(
        "UnrealHeaderTool failed for target WorldMakersEditor.\nFoo.h(22): error: Unrecognized type 'FThing' - type must be a UCLASS, USTRUCT or UENUM\nResult: Failed",
        "unreal-header-tool-error",
        actionable=True,
    )
    expect(
        r"D:\repo\game\Source\WorldMakers\Foo.cpp(42): error C2664: cannot convert argument 2\nResult: Failed",
        "compiler-error",
        actionable=True,
    )
    expect(
        "WorldMakers.obj : error LNK2019: unresolved external symbol Example referenced in function Main\nResult: Failed",
        "linker-error",
        actionable=True,
    )
    expect(
        "Plugin 'ProceduralThing' failed to load because module ProceduralThing could not be found.\nResult: Failed",
        "plugin-or-module-error",
        actionable=True,
    )
    expect(
        "LINK : fatal error LNK1104: cannot open file 'WorldMakersEditor.dll' because it is being used by another process\nResult: Failed",
        "file-lock-or-access-denied",
        actionable=False,
    )
    expect(
        "fatal error C1060: compiler is out of heap space\nResult: Failed",
        "out-of-memory",
        actionable=False,
    )
    expect(
        "Asset payload begins with version https://git-lfs.github.com/spec/v1 and is not a valid Unreal package.\nResult: Failed",
        "git-lfs-or-binary-pointer-error",
        actionable=False,
    )
    expect("Total time in Parallel executor: 1.24 seconds\nResult: Succeeded", "none", actionable=False)
    expect("AutomationTool exiting with ExitCode=6 (6)\nResult: Failed", "unknown-native-build-failure", actionable=True)

    privacy_log = r"C:\Users\JuanPablo\Desktop\WorldMakers\Foo.cpp(7): error C2065: identifier not found"
    privacy_result = classifier.classify_text(privacy_log, ROOT)
    diagnostics = privacy_result["diagnostics"]
    assert diagnostics, privacy_result
    assert "JuanPablo" not in diagnostics[0]["source"], diagnostics[0]
    assert "<user>" in diagnostics[0]["source"], diagnostics[0]

    compiler_result = classifier.classify_text(
        r"C:\Users\Builder\repo\Foo.cpp(10,3): error C2143: syntax error\nC:\Users\Builder\repo\Foo.cpp(11): warning C4996: old API\nResult: Failed",
        ROOT,
    )
    assert compiler_result["diagnosticCount"] == 2, compiler_result
    assert compiler_result["diagnostics"][0]["code"] == "C2143", compiler_result
    assert compiler_result["diagnostics"][0]["sourceLine"] == 10, compiler_result
    assert compiler_result["diagnostics"][0]["sourceColumn"] == 3, compiler_result

    print("World Makers Unreal build classifier tests passed.")


if __name__ == "__main__":
    main()
