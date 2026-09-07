#!/usr/bin/env python3
"""Source-level M1.8 visual proof gates when Unreal runtime execution is unavailable."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    raise SystemExit(message)


def main() -> None:
    required = [
        "game/Source/WorldMakers/Visual/WMVisualProfileSettings.h",
        "game/Source/WorldMakers/Visual/WMVisualProfileSettings.cpp",
        "game/Source/WorldMakers/Environment/WMCaribbeanRainforestPrototype.h",
        "game/Source/WorldMakers/Environment/WMCaribbeanRainforestPrototype.cpp",
        "game/Source/WorldMakers/Private/Tests/WMVisualProfileTests.cpp",
        "docs/m1-8-visual-proof.md",
    ]
    for relative in required:
        if not (ROOT / relative).is_file():
            fail(f"missing M1.8 source: {relative}")

    environment = (ROOT / "game/Source/WorldMakers/Environment/WMCaribbeanRainforestPrototype.cpp").read_text(encoding="utf-8")
    for token in (
        "UHierarchicalInstancedStaticMeshComponent",
        "GroundTiles->AddInstance",
        "TerrainMounds->AddInstance",
        "TreeTrunks->AddInstance",
        "TreeCanopies->AddInstance",
        "Rocks->AddInstance",
        "WaterEdgeMarkers->AddInstance",
        "FRandomStream Random(Seed)",
        "BuildClearingRadiusCm",
    ):
        if token not in environment:
            fail(f"M1.8 environment contract missing token: {token}")

    game_mode = (ROOT / "game/Source/WorldMakers/Game/WMGameMode.cpp").read_text(encoding="utf-8")
    if "EnsurePrototypeEnvironment" not in game_mode or "AWMCaribbeanRainforestPrototype" not in game_mode:
        fail("M1.8 GameMode must bootstrap the Caribbean Rainforest micro slice")

    player_header = (ROOT / "game/Source/WorldMakers/Player/WMPlayerCharacter.h").read_text(encoding="utf-8")
    for limb in ("PrototypeLeftArm", "PrototypeRightArm", "PrototypeLeftLeg", "PrototypeRightLeg"):
        if limb not in player_header:
            fail(f"M1.8 neutral child proxy missing component: {limb}")

    config = (ROOT / "game/Config/DefaultGame.ini").read_text(encoding="utf-8")
    for token in (
        "[/Script/WorldMakers.WMVisualProfileSettings]",
        "ProfileName=CaribbeanRainforestPrototype",
        "LowBudget=(TargetFPS=30",
        "MidBudget=(TargetFPS=30",
        "HighBudget=(TargetFPS=60",
        "bSpawnMicroVerticalSlice=True",
    ):
        if token not in config:
            fail(f"M1.8 visual config missing token: {token}")

    art_guide = (ROOT / "docs/art-style-guide.md").read_text(encoding="utf-8")
    if "[PLACEHOLDER" in art_guide:
        fail("M1.8 art guide must replace the initial visual placeholders")
    for token in ("#184F3A", "4.5–5 heads tall", "Cultural boundary", "HISM"):
        if token not in art_guide:
            fail(f"M1.8 art direction missing token: {token}")

    biome = (ROOT / "content/biomes/caribbean-rainforest/biome.yaml").read_text(encoding="utf-8")
    for token in ("status: prototype", "treeClusters: 16", "treeClusters: 28", "treeClusters: 42", "water-edge"):
        if token not in biome:
            fail(f"M1.8 biome profile missing token: {token}")

    tests = (ROOT / "game/Source/WorldMakers/Private/Tests/WMVisualProfileTests.cpp").read_text(encoding="utf-8")
    if "WorldMakers.Visual.Profile.Budgets" not in tests:
        fail("M1.8 visual profile automation test is missing")

    print("M1.8 visual proof source validation passed")


if __name__ == "__main__":
    main()
