#!/usr/bin/env python3
"""Validate the controlled entry from Pre-Unreal closure into native G1/G2 materialization."""
from __future__ import annotations

import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "scripts" / "enter-native-unreal-materialization.ps1"
REPORTER = ROOT / "scripts" / "report-native-unreal-bringup.ps1"
LAUNCHER = ROOT / "WorldMakers-NativeMaterialize.cmd"
DOC = ROOT / "docs" / "native-unreal-materialization-entry.md"
BRINGUP_DOC = ROOT / "docs" / "native-unreal-bringup-runbook.md"
WORKFLOW = ROOT / ".github" / "workflows" / "native-unreal-materialization-entry.yml"
FINAL_CONTRACT = ROOT / "content" / "production" / "final-pre-unreal-handoff-v1.json"
G1_RUNNER = ROOT / "scripts" / "run-g1-native-certification.ps1"
G2_RUNNER = ROOT / "scripts" / "run-g2-authored-certification.ps1"
G2_AUTHOR = ROOT / "scripts" / "unreal" / "author-g2-certification-map.py"
GITATTRIBUTES = ROOT / ".gitattributes"


def require(condition: bool, message: str) -> None:
    if not condition:
        print(f"ERROR: {message}")
        raise SystemExit(1)


def text(path: Path) -> str:
    require(path.is_file(), f"missing required file: {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8-sig")


def require_tokens(path: Path, tokens: tuple[str, ...]) -> None:
    content = text(path)
    for token in tokens:
        require(token in content, f"{path.relative_to(ROOT)} missing required token: {token}")


def main() -> int:
    contract = json.loads(text(FINAL_CONTRACT))
    certification = contract.get("certification", {})
    require(certification.get("officialState") == "PRE_UNREAL_READY", "final handoff official state drift")
    require(contract.get("exitCriteria", {}).get("nextPhase") == "native-unreal-materialization", "final handoff next phase drift")

    require_tokens(
        RUNNER,
        (
            "PRE_UNREAL_READY",
            "final-pre-unreal-handoff.json",
            "run-g1-native-certification.ps1",
            "run-g2-authored-certification.ps1",
            "-AuthorMap",
            "AllowPreUnrealBlockedForDevelopment",
            "AUTHORING_COMPLETE_COMMIT_REQUIRED",
            "WM_PrototypeCertification.umap",
            "check-attr filter",
            "filter:\\s*lfs",
            "powershell.exe",
        ),
    )
    require_tokens(
        REPORTER,
        (
            "worldmakers.native-bringup-summary.v1",
            "native-failure-summary.json",
            "WM_PrototypeCertification.umap",
            "GITHUB_STEP_SUMMARY",
            "First actionable blocker",
        ),
    )
    require_tokens(
        WORKFLOW,
        (
            "workflow_dispatch",
            "run_native_bringup",
            "UNREAL_SELF_HOSTED_ENABLED",
            "self-hosted",
            "Windows",
            "X64",
            "unreal",
            "AllowPreUnrealBlockedForDevelopment",
            "AllowNonMain",
            "report-native-unreal-bringup.ps1",
            "native-unreal-bringup-${{ github.sha }}",
            "WM_PrototypeCertification.umap",
            "continue-on-error: true",
            "Preserve fail-closed bring-up result",
        ),
    )
    require_tokens(LAUNCHER, ("enter-native-unreal-materialization.ps1", "native-materialization-entry.json"))
    require_tokens(G1_RUNNER, ("CERTIFIED", "WorldMakers."))
    require_tokens(G2_RUNNER, ("[switch]$AuthorMap", "run-g1-native-certification.ps1", "WM_PrototypeCertification.umap"))
    require_tokens(G2_AUTHOR, ("/Game/WorldMakers/Maps/WM_PrototypeCertification", "new_level", "save_current_level"))
    require_tokens(GITATTRIBUTES, ("*.umap filter=lfs", "*.uasset filter=lfs"))
    require_tokens(
        DOC,
        (
            "WorldMakers-NativeMaterialize.cmd",
            "PRE_UNREAL_READY",
            "G1",
            "G2",
            "AllowPreUnrealBlockedForDevelopment",
            "AUTHORING_COMPLETE_COMMIT_REQUIRED",
            "Git LFS",
        ),
    )
    require_tokens(
        BRINGUP_DOC,
        (
            "Native Unreal Bring-Up",
            "Run workflow",
            "UNREAL_SELF_HOSTED_ENABLED",
            "native-bringup-summary.json",
            "WM_PrototypeCertification.umap",
            "Git LFS",
        ),
    )

    # Source validation must never fabricate native Unreal binaries or treat hosted CI as native evidence.
    require("create_file" not in text(RUNNER), "runner may not fabricate Unreal binaries through source tooling")
    require("runs-on: ubuntu" in text(WORKFLOW), "workflow must preserve a hosted source-validation lane")
    require("runs-on: [self-hosted, Windows, X64, unreal]" in text(WORKFLOW), "native bring-up must require the locked self-hosted runner labels")

    print("World Makers Native Unreal Materialization Entry source contract: PASS")
    print("  Official path: PRE_UNREAL_READY -> G1 CERTIFIED -> real G2 map authoring")
    print("  Manual self-hosted bring-up: G1 native pass -> real map authoring or classified blocker")
    print("  Development override remains explicitly non-certifying")
    return 0


if __name__ == "__main__":
    sys.exit(main())
