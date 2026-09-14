#!/usr/bin/env python3
"""Static preflight for World Makers before native UE 5.8.2 execution.

This gate catches repository defects that hosted source CI can detect without an
installed Unreal Engine. It deliberately does not claim native certification.
Use --require-authored-map only on a runner/workstation where the binary map is
expected to have been authored and committed through Git LFS.
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "game"
SOURCE = GAME / "Source" / "WorldMakers"
CERTIFICATION_MAP = GAME / "Content" / "WorldMakers" / "Maps" / "WM_PrototypeCertification.umap"


def fail(message: str) -> None:
    raise SystemExit(message)


def require_file(path: Path) -> str:
    if not path.is_file():
        fail(f"Missing required file: {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8")


def scan_sources(pattern: re.Pattern[str]) -> list[str]:
    matches: list[str] = []
    for path in SOURCE.rglob("*"):
        if path.suffix.lower() not in {".h", ".hpp", ".cpp"} or not path.is_file():
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        if pattern.search(text):
            matches.append(path.relative_to(ROOT).as_posix())
    return matches


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--require-authored-map", action="store_true")
    args = parser.parse_args()

    project = json.loads(require_file(GAME / "WorldMakers.uproject"))
    if project.get("EngineAssociation") != "5.8":
        fail("WorldMakers.uproject must remain associated with Unreal Engine 5.8")

    engine_version = require_file(GAME / "UNREAL_ENGINE_VERSION").strip()
    if engine_version != "5.8.2":
        fail(f"Native baseline must remain UE 5.8.2, found {engine_version!r}")

    build_rules = require_file(SOURCE / "WorldMakers.Build.cs")
    if "PublicIncludePaths.Add(ModuleDirectory);" not in build_rules:
        fail("WorldMakers.Build.cs must expose ModuleDirectory for module-root-qualified includes")
    if '"ProceduralMeshComponent"' not in build_rules:
        fail("WorldMakers must declare the ProceduralMeshComponent module dependency")

    bad_procedural_include = scan_sources(re.compile(r'#include\s+"Components/ProceduralMeshComponent\.h"'))
    if bad_procedural_include:
        fail(
            "UE 5.8 ProceduralMeshComponent uses the plugin-root include path; fix: "
            + ", ".join(bad_procedural_include)
        )

    slot_shadowing = scan_sources(
        re.compile(r"\b(?:UVerticalBoxSlot|UHorizontalBoxSlot)\s*\*\s*Slot\b")
    )
    if slot_shadowing:
        fail(
            "Local UMG variable named Slot shadows UWidget::Slot under the locked compiler: "
            + ", ".join(slot_shadowing)
        )

    game_ini = require_file(GAME / "Config" / "DefaultGame.ini")
    project_id_match = re.search(r"^ProjectID=([0-9A-Fa-f]{32})$", game_ini, re.MULTILINE)
    if not project_id_match or set(project_id_match.group(1)) == {"0"}:
        fail("DefaultGame.ini must contain a stable, non-zero 32-hex ProjectID")

    attributes = require_file(ROOT / ".gitattributes")
    logo_exception = "apps/player-dashboard/public/logo-primary.png -filter -diff -merge -text"
    if logo_exception not in attributes:
        fail("Dashboard logo must be explicitly exempted from the global PNG LFS rule")

    map_status = "present" if CERTIFICATION_MAP.is_file() else "missing"
    if args.require_authored_map and map_status != "present":
        fail(
            "Native readiness requires game/Content/WorldMakers/Maps/"
            "WM_PrototypeCertification.umap authored in Unreal and committed via Git LFS"
        )

    uasset_count = sum(1 for path in GAME.rglob("*.uasset") if path.is_file())
    umap_count = sum(1 for path in GAME.rglob("*.umap") if path.is_file())
    print(
        "World Makers Unreal source preflight passed: "
        f"engine={engine_version}, authored_map={map_status}, "
        f"tracked_uasset_count={uasset_count}, tracked_umap_count={umap_count}. "
        "Native UE build/test certification remains a separate gate."
    )


if __name__ == "__main__":
    main()
