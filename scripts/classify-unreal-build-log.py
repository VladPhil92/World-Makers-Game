#!/usr/bin/env python3
"""Classify World Makers Unreal native build failures into actionable categories.

The classifier is deliberately local and privacy-minimized: it only emits a bounded
set of diagnostic excerpts, normalizes repository/home paths, and never uploads
anything. It is intended to turn an opaque UnrealBuildTool failure into a stable
machine-readable category that can be fixed in source or workstation configuration.
"""

from __future__ import annotations

import argparse
import json
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_LOG = ROOT / "artifacts" / "unreal-readiness" / "build.log"
DEFAULT_OUTPUT = ROOT / "artifacts" / "unreal-readiness" / "native-failure-summary.json"


@dataclass(frozen=True)
class Rule:
    category: str
    priority: int
    confidence: str
    repository_actionable: bool
    remediation: str
    patterns: tuple[re.Pattern[str], ...]


def rx(*patterns: str) -> tuple[re.Pattern[str], ...]:
    return tuple(re.compile(pattern, re.IGNORECASE) for pattern in patterns)


RULES: tuple[Rule, ...] = (
    Rule(
        "live-coding-active",
        100,
        "high",
        False,
        "Save editor work, close Unreal Editor / Live Coding, then rerun the build. This is a runtime-state blocker, not an installation failure.",
        rx(r"live coding is active", r"unable to build while live coding", r"LiveCodingConsole"),
    ),
    Rule(
        "visual-studio-toolchain-missing",
        98,
        "high",
        False,
        "Use Visual Studio Installer > Modify and verify Game development with C++ / MSVC x64 tools. Do not reinstall Unreal Engine for this category.",
        rx(
            r"no visual c\+\+ installation was found",
            r"unable to find valid installation of visual studio",
            r"visual studio .* must be installed",
            r"Microsoft\.VisualStudio\.Component\.VC\.Tools\.x86\.x64",
            r"toolchain.*not found",
        ),
    ),
    Rule(
        "windows-sdk-missing",
        97,
        "high",
        False,
        "Use Visual Studio Installer > Modify and add a current Windows 10/11 SDK. Do not reinstall Unreal Engine for this category.",
        rx(r"windows sdk.*must be installed", r"unable to find windows sdk", r"windows sdk is not installed", r"Windows Kits\\10.*not found"),
    ),
    Rule(
        "out-of-memory",
        96,
        "high",
        False,
        "Close memory-heavy applications and retry a clean build. If repeatable, reduce parallel compile pressure before changing source dependencies.",
        rx(r"fatal error C1060", r"out of heap space", r"out of memory", r"insufficient memory", r"LNK1102"),
    ),
    Rule(
        "include-file-missing",
        95,
        "high",
        True,
        "Fix the include path/module dependency or restore the missing generated/source file in the repository before changing workstation dependencies.",
        rx(r"fatal error C1083:.*cannot open include file", r"cannot open source file", r"fatal error: .* file not found"),
    ),
    Rule(
        "file-lock-or-access-denied",
        94,
        "medium",
        False,
        "Close processes holding generated binaries/intermediate files, verify antivirus/file permissions, then retry. Do not reinstall Unreal first.",
        rx(r"access is denied", r"being used by another process", r"permission denied", r"failed to delete .*Intermediate"),
    ),
    Rule(
        "unreal-header-tool-error",
        93,
        "high",
        True,
        "Inspect the reported UCLASS/USTRUCT/UFUNCTION/UPROPERTY header diagnostic and fix Unreal reflection syntax or generated-header ordering in source.",
        rx(
            r"unrealheadertool.*failed",
            r"error:.*GENERATED_BODY",
            r"unrecognized type .* - type must be a UCLASS, USTRUCT or UENUM",
            r"Expected an include at the top of the header:.*generated\.h",
            r"OtherCompilationError \(5\)",
        ),
    ),
    Rule(
        "compiler-error",
        90,
        "high",
        True,
        "Fix the reported C/C++ compiler diagnostics in the repository. Reinstalling Unreal is not the default corrective action once the workstation doctor is green.",
        rx(r"\berror C\d{4}\b", r"\bfatal error C\d{4}\b", r"\berror: .*\.(?:cpp|c|h|hpp)"),
    ),
    Rule(
        "linker-error",
        89,
        "high",
        True,
        "Fix the missing symbol/module/library definition or Build.cs dependency reported by the linker.",
        rx(r"\berror LNK\d+\b", r"unresolved external symbol", r"fatal error LNK\d+"),
    ),
    Rule(
        "plugin-or-module-error",
        86,
        "medium",
        True,
        "Verify the referenced plugin/module exists, is enabled for the project, and is declared correctly in .uproject/Build.cs before reinstalling the engine.",
        rx(
            r"plugin ['\"].+['\"].*failed to load",
            r"missing or incompatible modules",
            r"could not be compiled\. try rebuilding from source manually",
            r"unable to instantiate module",
            r"module .* could not be found",
        ),
    ),
    Rule(
        "git-lfs-or-binary-pointer-error",
        80,
        "medium",
        False,
        "Run the repository Git LFS checks and restore the real binary asset from LFS. Do not regenerate or reinstall Unreal to repair an LFS pointer mismatch.",
        rx(r"version https://git-lfs.github.com/spec/v1", r"git lfs", r"pointer file", r"not a valid .* package"),
    ),
    Rule(
        "generated-files-stale",
        78,
        "medium",
        True,
        "Remove stale project Intermediate/Build output through the repository readiness flow and rebuild; preserve source files and authored Content.",
        rx(r"outdated or invalid module", r"generated code is out of date", r"needs to be rebuilt", r"manifest.*out of date"),
    ),
)

MSVC_DIAGNOSTIC = re.compile(
    r"(?P<path>(?:[A-Za-z]:)?[^\r\n]*?\.(?:cpp|c|cc|cxx|h|hpp|inl|cs))"
    r"\((?P<line>\d+)(?:,(?P<column>\d+))?\)\s*:\s*"
    r"(?P<kind>fatal error|error|warning)\s*(?P<code>[A-Z]+\d+)?\s*:\s*(?P<message>.*)",
    re.IGNORECASE,
)
GENERIC_DIAGNOSTIC = re.compile(r"(?P<kind>fatal error|error)\s*:\s*(?P<message>.+)", re.IGNORECASE)


def sanitize_text(text: str, repo_root: Path) -> str:
    normalized = text.replace("/", "\\")
    replacements: list[tuple[str, str]] = []
    try:
        replacements.append((str(repo_root.resolve()).replace("/", "\\"), "<repo>"))
    except OSError:
        pass
    try:
        replacements.append((str(Path.home().resolve()).replace("/", "\\"), "<home>"))
    except OSError:
        pass

    for source, target in replacements:
        if source:
            normalized = re.sub(re.escape(source), target, normalized, flags=re.IGNORECASE)

    normalized = re.sub(r"([A-Za-z]:\\Users\\)[^\\\s]+", r"\1<user>", normalized, flags=re.IGNORECASE)
    normalized = re.sub(r"\\+", r"\\", normalized)
    return normalized.strip()


def iter_lines(text: str) -> Iterable[tuple[int, str]]:
    for index, line in enumerate(text.splitlines(), start=1):
        yield index, line.rstrip()


def extract_diagnostics(text: str, repo_root: Path, limit: int = 24) -> list[dict[str, object]]:
    diagnostics: list[dict[str, object]] = []
    seen: set[str] = set()

    for line_number, raw_line in iter_lines(text):
        if len(diagnostics) >= limit:
            break
        line = sanitize_text(raw_line, repo_root)
        if not line:
            continue

        match = MSVC_DIAGNOSTIC.search(line)
        if match:
            key = line.lower()
            if key in seen:
                continue
            seen.add(key)
            diagnostics.append(
                {
                    "logLine": line_number,
                    "kind": match.group("kind").lower(),
                    "code": match.group("code") or None,
                    "source": match.group("path").strip(),
                    "sourceLine": int(match.group("line")),
                    "sourceColumn": int(match.group("column")) if match.group("column") else None,
                    "message": match.group("message").strip(),
                }
            )
            continue

        generic = GENERIC_DIAGNOSTIC.search(line)
        if generic and any(token in line.lower() for token in ("unreal", "build", "module", "plugin", "toolchain", "sdk", "link", "compile")):
            key = line.lower()
            if key in seen:
                continue
            seen.add(key)
            diagnostics.append(
                {
                    "logLine": line_number,
                    "kind": generic.group("kind").lower(),
                    "code": None,
                    "source": None,
                    "sourceLine": None,
                    "sourceColumn": None,
                    "message": generic.group("message").strip(),
                }
            )

    return diagnostics


def classify_text(text: str, repo_root: Path = ROOT) -> dict[str, object]:
    matches: list[tuple[Rule, list[str]]] = []
    sanitized_lines = [(line_no, sanitize_text(line, repo_root)) for line_no, line in iter_lines(text)]

    for rule in RULES:
        excerpts: list[str] = []
        for _, line in sanitized_lines:
            if any(pattern.search(line) for pattern in rule.patterns):
                if line not in excerpts:
                    excerpts.append(line)
                if len(excerpts) >= 4:
                    break
        if excerpts:
            matches.append((rule, excerpts))

    matches.sort(key=lambda item: item[0].priority, reverse=True)

    success_marker = bool(re.search(r"\bResult:\s*Succeeded\b", text, re.IGNORECASE))
    failed_marker = bool(re.search(r"\bResult:\s*Failed\b", text, re.IGNORECASE))
    diagnostics = extract_diagnostics(text, repo_root)

    if matches:
        primary_rule, primary_excerpts = matches[0]
        status = "failed"
        primary_category = primary_rule.category
        confidence = primary_rule.confidence
        repository_actionable = primary_rule.repository_actionable
        remediation = primary_rule.remediation
    elif success_marker and not failed_marker:
        status = "passed"
        primary_category = "none"
        confidence = "high"
        repository_actionable = False
        remediation = "No native build failure detected."
        primary_excerpts = []
    else:
        status = "failed" if failed_marker or diagnostics or text.strip() else "unknown"
        primary_category = "unknown-native-build-failure" if status == "failed" else "no-diagnostic-data"
        confidence = "low"
        repository_actionable = status == "failed"
        remediation = (
            "Inspect the bounded diagnostics and full local build.log. If the workstation doctor is green, treat this as a repository/native build defect first."
            if status == "failed"
            else "No build log content was available to classify. Run the native build again."
        )
        primary_excerpts = []

    categories = [
        {
            "category": rule.category,
            "confidence": rule.confidence,
            "repositoryActionable": rule.repository_actionable,
            "remediation": rule.remediation,
            "excerpts": excerpts,
        }
        for rule, excerpts in matches[:8]
    ]

    return {
        "schemaVersion": 1,
        "status": status,
        "primaryCategory": primary_category,
        "confidence": confidence,
        "repositoryActionable": repository_actionable,
        "remediation": remediation,
        "matchedCategories": categories,
        "primaryExcerpts": primary_excerpts,
        "diagnostics": diagnostics,
        "diagnosticCount": len(diagnostics),
        "privacy": {
            "boundedDiagnostics": True,
            "absoluteRepositoryPathRedacted": True,
            "homePathRedacted": True,
            "uploadsData": False,
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--log", type=Path, default=DEFAULT_LOG)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    parser.add_argument("--print", dest="print_json", action="store_true")
    args = parser.parse_args()

    if not args.log.is_file():
        result = {
            "schemaVersion": 1,
            "status": "missing-log",
            "primaryCategory": "build-log-missing",
            "confidence": "high",
            "repositoryActionable": False,
            "remediation": f"Run the native build to create {args.log.name} before classifying it.",
            "matchedCategories": [],
            "primaryExcerpts": [],
            "diagnostics": [],
            "diagnosticCount": 0,
            "privacy": {
                "boundedDiagnostics": True,
                "absoluteRepositoryPathRedacted": True,
                "homePathRedacted": True,
                "uploadsData": False,
            },
        }
    else:
        text = args.log.read_text(encoding="utf-8", errors="replace")
        result = classify_text(text, args.repo_root)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    if args.print_json:
        print(json.dumps(result, indent=2, ensure_ascii=False))
    else:
        print(f"World Makers native build classification: {result['primaryCategory']} ({result['confidence']})")
        print(f"Evidence: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
