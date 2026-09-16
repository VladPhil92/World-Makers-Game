#!/usr/bin/env python3
"""Validate the controlled entry from Pre-Unreal closure into native G1/G2 materialization."""
from __future__ import annotations

import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "scripts" / "enter-native-unreal-materialization.ps1"
LAUNCHER = ROOT / "WorldMakers-NativeMaterialize.cmd"
DOC = ROOT / "docs" / "native-unreal-materialization-entry.md"
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

    # This gate must never fabricate the binary map in hosted/source validation.
    require("create_file" not in text(RUNNER), "runner may not fabricate Unreal binaries through source tooling")
    print("World Makers Native Unreal Materialization Entry source contract: PASS")
    print("  Official path: PRE_UNREAL_READY -> G1 CERTIFIED -> real G2 map authoring")
    print("  Development override remains explicitly non-certifying")
    return 0


if __name__ == "__main__":
    sys.exit(main())
