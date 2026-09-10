#!/usr/bin/env python3
"""V3 environment-art / biome-production source gate."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CANONICAL = ROOT / "content/visual/environment/caribbean-rainforest-v3.json"
PACKAGED = ROOT / "game/Content/WorldMakers/Visual/caribbean-rainforest-v3.json"


def fail(message: str) -> None:
    raise SystemExit(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def read(path: str) -> str:
    source = ROOT / path
    require(source.is_file(), f"Missing V3 file: {path}")
    return source.read_text(encoding="utf-8")


def require_tokens(path: str, *tokens: str) -> str:
    text = read(path)
    missing = [token for token in tokens if token not in text]
    require(not missing, f"{path} missing V3 contract tokens: {missing}")
    return text


def main() -> None:
    uproject = json.loads(read("game/WorldMakers.uproject"))
    plugins = {item.get("Name"): item.get("Enabled") for item in uproject.get("Plugins", [])}
    require(plugins.get("ProceduralMeshComponent") is True, "V3 requires ProceduralMeshComponent to be explicitly enabled")

    build_cs = read("game/Source/WorldMakers/WorldMakers.Build.cs")
    require('"ProceduralMeshComponent"' in build_cs, "WorldMakers module must depend on ProceduralMeshComponent")

    geometry_h = require_tokens(
        "game/Source/WorldMakers/Visual/WMProceduralEnvironmentGeometry.h",
        "FWMEnvironmentMeshData",
        "AppendIrregularGroundDisc",
        "AppendTaperedTrunk",
        "AppendFacetedEllipsoid",
        "AppendButtressRoot",
        "AppendLeafCluster",
        "AppendSinuousRiverStrip",
    )
    geometry_cpp = require_tokens(
        "game/Source/WorldMakers/Visual/WMProceduralEnvironmentGeometry.cpp",
        "AppendTriangle",
        "FVector::CrossProduct",
        "FRandomStream",
        "GetSafeNormal",
    )
    require("/Engine/BasicShapes/" not in geometry_h + geometry_cpp, "V3 procedural geometry vocabulary must not wrap Engine BasicShapes")

    rainforest_h = require_tokens(
        "game/Source/WorldMakers/Environment/WMCaribbeanRainforestPrototype.h",
        "bUseProceduralEnvironmentArt",
        "GroundArt",
        "TerrainArt",
        "BarkAndRootsArt",
        "FoliageArt",
        "StoneArt",
        "WaterArt",
        "SetEnvironmentArtPathEnabled",
    )
    rainforest_cpp = require_tokens(
        "game/Source/WorldMakers/Environment/WMCaribbeanRainforestPrototype.cpp",
        "UProceduralMeshComponent",
        "CreateMeshSection_LinearColor",
        "ConfigureRenderOnly",
        "SetCollisionEnabled(ECollisionEnabled::NoCollision)",
        "SetEnvironmentArtPathEnabled(bUseProceduralEnvironmentArt)",
        "AppendButtressRoot",
        "AppendLeafCluster",
        "AppendSinuousRiverStrip",
        "HeroBase",
        "EWMStylizedSurfaceRole::GroundEarth",
        "EWMStylizedSurfaceRole::Terrain",
        "EWMStylizedSurfaceRole::Bark",
        "EWMStylizedSurfaceRole::Foliage",
        "EWMStylizedSurfaceRole::Stone",
        "EWMStylizedSurfaceRole::Water",
    )
    require("/Engine/BasicShapes/" in rainforest_cpp, "V3 must retain an explicit reversible Engine-primitive fallback/collision path")
    require("SetVisibility(!bEnabled" in rainforest_cpp and "SetVisibility(bEnabled" in rainforest_cpp, "V3 render/fallback path must remain explicitly switchable")

    config = require_tokens(
        "game/Config/DefaultGame.ini",
        "bUseProceduralEnvironmentArt=True",
        '+DirectoriesToAlwaysStageAsNonUFS=(Path="WorldMakers/Visual")',
    )
    require("ProfileName=CaribbeanRainforestPrototype" in config, "V3 must remain anchored to the existing visual profile")

    require(CANONICAL.is_file() and PACKAGED.is_file(), "V3 canonical/staged environment manifests are required")
    require(CANONICAL.read_bytes() == PACKAGED.read_bytes(), "V3 canonical and staged environment manifests must be byte-equivalent")
    manifest = json.loads(CANONICAL.read_text(encoding="utf-8"))
    require(manifest.get("schemaVersion") == 1, "V3 environment manifest schemaVersion must be 1")
    require(manifest.get("renderPath") == "procedural-environment-art", "V3 manifest must declare the procedural render path")
    require(manifest.get("collisionPath") == "legacy-hidden-proxy", "V3 manifest must preserve collision/render separation")

    families = manifest.get("families", [])
    by_id = {family.get("id"): family for family in families}
    required_families = {"ground", "terrain", "trees", "understory", "rocks", "water-edge", "hero-ceiba"}
    require(required_families == set(by_id), f"V3 environment family set drifted: {sorted(required_families ^ set(by_id))}")
    require(by_id["trees"].get("variantCount", 0) >= 3, "V3 requires at least three tree silhouette variants")
    require(by_id["rocks"].get("variantCount", 0) >= 3, "V3 requires a non-trivial rock family")

    tiers = manifest.get("qualityTiers", {})
    for name in ("low", "mid", "high"):
        require(name in tiers, f"Missing V3 quality tier: {name}")
        for key in ("facetSegments", "groundSegments", "riverSegments", "leafCountPerCluster"):
            require(isinstance(tiers[name].get(key), int) and tiers[name][key] > 0, f"Invalid V3 tier value {name}.{key}")
    for key in ("facetSegments", "groundSegments", "riverSegments", "leafCountPerCluster"):
        require(tiers["low"][key] <= tiers["mid"][key] <= tiers["high"][key], f"V3 quality tiers must scale monotonically for {key}")

    composition = manifest.get("composition", {})
    require(composition.get("quietBuildClearing") is True, "V3 must preserve a quiet build clearing")
    require(composition.get("densityIncreasesTowardPerimeter") is True, "V3 must preserve perimeter framing")
    require(composition.get("waterBoundaryOnOneSide") is True, "V3 must preserve the one-sided water composition")
    require(composition.get("heroLandmarkCount") == 1, "V3 requires one deterministic hero landmark")

    tests = require_tokens(
        "game/Source/WorldMakers/Private/Tests/WMProceduralEnvironmentGeometryTests.cpp",
        "WorldMakers.Visual.Environment.GroundIsDeterministic",
        "WorldMakers.Visual.Environment.TreeHasTrunkRootsAndCanopy",
        "WorldMakers.Visual.Environment.UnderstoryUsesReadableLeafStar",
        "WorldMakers.Visual.Environment.RiverIsContinuousStrip",
    )
    require(tests.count("IMPLEMENT_SIMPLE_AUTOMATION_TEST") >= 4, "V3 requires at least four environment geometry automation tests")

    docs = require_tokens(
        "docs/v3-environment-art-biome-production.md",
        "Source-complete procedural environment art pass",
        "six render-only `UProceduralMeshComponent` families",
        "three crown families",
        "hidden collision/fallback proxies",
        "does **not** claim",
        "Issue #9",
    )
    require("hero ceiba" in docs.lower(), "V3 docs must describe the hero-ceiba landmark")

    roadmap = read("docs/visual-production-roadmap.md")
    require("V3 — Environment Art / Biome Production" in roadmap, "Visual roadmap lost V3")
    require("V3 status: source-complete procedural art pass" in roadmap, "Visual roadmap must mark V3 source status explicitly")

    workflow = read(".github/workflows/repo-quality.yml")
    require("python scripts/validate-v3-environment-art.py" in workflow, "Repository Quality must execute the V3 environment-art gate")

    print("V3 environment art passed: procedural render families, deterministic geometry, V2 surface integration, reversible collision fallback and staged handoff manifest are wired.")


if __name__ == "__main__":
    main()
