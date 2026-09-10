#!/usr/bin/env python3
"""V2 materials and surface-language source gate."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    raise SystemExit(message)


def read(path: str) -> str:
    source = ROOT / path
    if not source.exists():
        fail(f"Missing V2 file: {path}")
    return source.read_text(encoding="utf-8")


def require_tokens(path: str, *tokens: str) -> str:
    text = read(path)
    missing = [token for token in tokens if token not in text]
    if missing:
        fail(f"{path} missing V2 contract tokens: {missing}")
    return text


def main() -> None:
    profile_h = require_tokens(
        "game/Source/WorldMakers/Visual/WMVisualProfileSettings.h",
        "EWMStylizedSurfaceRole",
        "FWMStylizedSurfaceResponse",
        "FWMStylizedSurfaceLook",
        "MaxMaterialSlotsPerMesh",
        "MaxTextureEdgePx",
        "MaxSampledTexturesPerMaterial",
        "PreviewValid",
        "PreviewInvalid",
        "MagicalAccent",
    )
    if profile_h.count("UPROPERTY") < 35:
        fail("V2 visual profile unexpectedly lost configured visual fields")

    require_tokens(
        "game/Source/WorldMakers/Visual/WMVisualProfileSettings.cpp",
        "FWMStylizedSurfaceResponse::IsSane",
        "FWMStylizedSurfaceLook::IsSane",
        "MaxMaterialSlotsPerMesh",
        "MaxSampledTexturesPerMaterial",
    )

    surface_cpp = require_tokens(
        "game/Source/WorldMakers/Visual/WMStylizedSurfaceLibrary.cpp",
        "SetVectorParameterValueOnMaterials",
        "SetCustomPrimitiveDataVector4",
        "SetCustomPrimitiveDataFloat",
        "OpacityIntent",
        "SurfaceRoleToString",
        "EWMStylizedSurfaceRole::Water",
        "EWMStylizedSurfaceRole::Foliage",
    )
    for index in range(4, 10):
        if str(index) not in surface_cpp:
            fail(f"V2 Custom Primitive Data contract appears to omit index {index}")

    rainforest = require_tokens(
        "game/Source/WorldMakers/Environment/WMCaribbeanRainforestPrototype.cpp",
        "/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial",
        "ApplySurfaceLanguage",
        "GroundEarth",
        "Terrain",
        "Bark",
        "Foliage",
        "Stone",
        "Water",
    )
    if rainforest.count("ApplyConfiguredSurface") < 6:
        fail("Rainforest proxy does not apply all six V2 environment surface roles")

    build_cpp = require_tokens(
        "game/Source/WorldMakers/Building/WMBuildPieceActor.cpp",
        "BuildEco",
        "BuildNeutral",
        "PreviewValid",
        "PreviewInvalid",
        "SetCustomDepthStencilValue",
        "bValid ? 1 : 2",
    )
    if "Color is supplemental only" not in build_cpp:
        fail("V2 placement feedback must explicitly preserve a non-color identity")

    tests = require_tokens(
        "game/Source/WorldMakers/Private/Tests/WMStylizedSurfaceTests.cpp",
        "WorldMakers.Visual.Surface.Semantics",
        "WorldMakers.Visual.Surface.StableRoleIdentity",
        "Water reads smoother than stone",
        "Foliage carries wind response",
    )
    if tests.count("IMPLEMENT_SIMPLE_AUTOMATION_TEST") < 2:
        fail("V2 requires at least two stylized-surface automation tests")

    config = require_tokens(
        "game/Config/DefaultGame.ini",
        "SurfaceResponse=(GroundRoughness=",
        "PreviewValid=(R=",
        "PreviewInvalid=(R=",
        "MaxTextureEdgePx=1024",
        "MaxTextureEdgePx=2048",
        "MaxSampledTexturesPerMaterial=10",
    )
    if config.count("MaxMaterialSlotsPerMesh=2") != 3:
        fail("All V2 visual tiers must keep the two-slot material ceiling")

    manifest_path = ROOT / "content/visual/materials/surface-language-v2.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    expected_roles = [
        "ground-earth",
        "terrain",
        "bark",
        "foliage",
        "stone",
        "water",
        "build-neutral",
        "build-eco",
        "preview-valid",
        "preview-invalid",
        "magical-accent",
    ]
    if manifest.get("schemaVersion") != 1 or manifest.get("surfaceRoles") != expected_roles:
        fail("V2 material manifest role contract is invalid")
    cp_data = manifest.get("customPrimitiveData", [])
    if [item.get("index") for item in cp_data] != list(range(10)):
        fail("V2 material manifest must reserve contiguous CPD indices 0..9")
    targets = {item.get("targetPath") for item in manifest.get("masterMaterials", [])}
    if targets != {
        "/Game/WorldMakers/Materials/M_WM_MasterSurface",
        "/Game/WorldMakers/Materials/M_WM_Water",
    }:
        fail("V2 master-material target paths changed unexpectedly")
    if not manifest.get("accessibility", {}).get("colorOnlyStateForbidden"):
        fail("V2 manifest must forbid color-only placement state")

    docs = require_tokens(
        "docs/v2-materials-surface-language.md",
        "M_WM_MasterSurface",
        "M_WM_Water",
        "Custom Primitive Data",
        "256 px/m",
        "stencil `1`",
        "stencil `2`",
        "does **not** claim",
    )
    if "opaque stylized fallback" not in docs:
        fail("V2 water documentation must preserve a low-tier fallback")

    roadmap = require_tokens(
        "docs/visual-production-roadmap.md",
        "## V2 — Materials & Surface Language",
        "## V3 — Environment Art / Biome Production",
    )
    if "Silhouette → composition → motion → material → effects → detail" not in roadmap:
        fail("Visual production principle was lost")

    art_guide = require_tokens(
        "docs/art-style-guide.md",
        "premium stylized 3D world",
        "Surface language",
        "M_WM_MasterSurface",
        "M_WM_Water",
    )

    workflow = read(".github/workflows/repo-quality.yml")
    if "python scripts/validate-v2-materials-surface.py" not in workflow:
        fail("Repository Quality must execute the V2 materials gate")

    print("V2 materials/surface language passed: semantic roles, CPD contract, proxy colors, build feedback, budgets, handoff manifest and tests are wired.")


if __name__ == "__main__":
    main()
