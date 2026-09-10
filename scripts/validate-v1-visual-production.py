#!/usr/bin/env python3
"""V1 visual production foundation source gate."""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    raise SystemExit(message)


def read(path: str) -> str:
    source = ROOT / path
    if not source.exists():
        fail(f"Missing V1 file: {path}")
    return source.read_text(encoding="utf-8")


def require_tokens(path: str, *tokens: str) -> str:
    text = read(path)
    missing = [token for token in tokens if token not in text]
    if missing:
        fail(f"{path} missing V1 contract tokens: {missing}")
    return text


def main() -> None:
    require_tokens(
        "game/Source/WorldMakers/Visual/WMVisualProfileSettings.h",
        "FWMAtmosphereLook",
        "SunIntensity",
        "SkyLightIntensity",
        "FogDensity",
        "FogHeightFalloff",
        "CameraFOVDegrees",
    )
    require_tokens(
        "game/Source/WorldMakers/Visual/WMVisualProfileSettings.cpp",
        "FWMAtmosphereLook::IsSane",
        "FMath::IsFinite",
    )

    rainforest = require_tokens(
        "game/Source/WorldMakers/Environment/WMCaribbeanRainforestPrototype.cpp",
        "USkyLightComponent",
        "USkyAtmosphereComponent",
        "UExponentialHeightFogComponent",
        "ApplyLookDevelopmentProfile",
        "SetAtmosphereSunLight",
        "SetFogDensity",
        "SetFogHeightFalloff",
    )
    if "/Engine/BasicShapes/" not in rainforest:
        fail("V1 must preserve explicit proxy geometry until V2/V3 authored assets replace it")

    require_tokens(
        "game/Source/WorldMakers/Visual/WMPrototypeMotionStyle.h",
        "FWMPrototypeMotionPose",
        "FWMPrototypeMotionStyle",
        "Evaluate",
    )
    motion_cpp = require_tokens(
        "game/Source/WorldMakers/Visual/WMPrototypeMotionStyle.cpp",
        "FMath::Clamp",
        "ArmSwingDegrees",
        "LegSwingDegrees",
        "bAirborne",
    )
    if "28.0f" not in motion_cpp or "24.0f" not in motion_cpp:
        fail("V1 proxy gait amplitudes must remain explicitly bounded")

    require_tokens(
        "game/Source/WorldMakers/Player/WMPlayerCharacter.cpp",
        "PrimaryActorTick.bCanEverTick = true",
        "UpdatePrototypeMotion",
        "FWMPrototypeMotionStyle::Evaluate",
        "SetFieldOfView",
        "CameraFOVDegrees",
    )

    tests = require_tokens(
        "game/Source/WorldMakers/Private/Tests/WMPrototypeMotionStyleTests.cpp",
        "WorldMakers.Visual.Motion.IdlePoseIsStable",
        "WorldMakers.Visual.Motion.LocomotionHasReadableOpposition",
        "WorldMakers.Visual.Motion.InputsRemainBounded",
    )
    if tests.count("IMPLEMENT_SIMPLE_AUTOMATION_TEST") < 3:
        fail("V1 requires at least three proxy-motion automation tests")

    require_tokens(
        "game/Source/WorldMakers/Private/Tests/WMVisualProfileTests.cpp",
        "Atmosphere.IsSane",
        "CameraFOVDegrees",
        "FogDensity",
    )

    config = require_tokens(
        "game/Config/DefaultGame.ini",
        "Atmosphere=(SunIntensity=",
        "SkyLightIntensity=",
        "FogDensity=",
        "CameraFOVDegrees=",
    )
    if "ProfileName=CaribbeanRainforestPrototype" not in config:
        fail("V1 look-development values must remain anchored to the explicit prototype profile")

    roadmap = require_tokens(
        "docs/visual-production-roadmap.md",
        "## V1 — Visual Production Foundation / Look Development",
        "## V2 — Materials & Surface Language",
        "## V3 — Environment Art / Biome Production",
        "## V4 — Character Art & Rig",
        "## V5 — Character & Interaction Animation",
        "## V6 — VFX & Fantastic/Scientific Feedback",
        "## V7 — Camera, Cinematics & UI Motion",
        "## V8 — Visual Optimization & Device Certification",
    )
    if "Silhouette → composition → motion → material → effects → detail" not in roadmap:
        fail("Visual production roadmap must preserve the visual priority principle")

    foundation = require_tokens(
        "docs/v1-visual-production-foundation.md",
        "primary warm sunlight",
        "Temporary procedural avatar motion",
        "does **not** claim",
        "authored master materials",
        "production child avatar",
        "Native visual approval",
    )
    if "Cube" not in foundation or "Sphere" not in foundation or "Cylinder" not in foundation:
        fail("V1 documentation must explicitly identify remaining primitive proxies")

    art_guide = read("docs/art-style-guide.md")
    for token in ("premium stylized 3D world", "not production art", "Production handoff"):
        if token not in art_guide:
            fail(f"Art style guide lost production boundary token: {token}")

    workflow = read(".github/workflows/repo-quality.yml")
    if "python scripts/validate-v1-visual-production.py" not in workflow:
        fail("Repository Quality must execute the V1 visual production gate")

    print("V1 visual production foundation passed: atmosphere/look profile, camera framing, bounded proxy motion, tests and V1-V8 production roadmap are wired.")


if __name__ == "__main__":
    main()
