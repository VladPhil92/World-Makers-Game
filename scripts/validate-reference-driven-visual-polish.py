#!/usr/bin/env python3
"""Validate image-derived World Makers visual/animation polish contracts."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CANONICAL = ROOT / "content/visual/governance/reference-driven-visual-polish-v2.json"
PACKAGED = ROOT / "game/Content/WorldMakers/Visual/reference-driven-visual-polish-v2.json"
HEADER = ROOT / "game/Source/WorldMakers/Visual/WMReferenceVisualPolishRuntime.h"
CPP = ROOT / "game/Source/WorldMakers/Visual/WMReferenceVisualPolishRuntime.cpp"
TESTS = ROOT / "game/Source/WorldMakers/Private/Tests/WMReferenceVisualPolishTests.cpp"
DOC = ROOT / "docs/reference-driven-visual-animation-polish.md"
WORKFLOW = ROOT / ".github/workflows/repo-quality.yml"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit("Reference visual polish validation failed: " + message)


def main() -> None:
    required = (CANONICAL, PACKAGED, HEADER, CPP, TESTS, DOC, WORKFLOW)
    missing = [str(path.relative_to(ROOT)) for path in required if not path.is_file()]
    require(not missing, f"missing files: {missing}")
    require(CANONICAL.read_bytes() == PACKAGED.read_bytes(), "canonical and staged polish contracts must be byte-identical")

    data = json.loads(CANONICAL.read_text(encoding="utf-8"))
    require(data.get("schemaVersion") == 1, "schemaVersion must be 1")
    require(data.get("policyId") == "visual.reference-driven-polish.v2", "policy identity drifted")
    require(data.get("presentationOnly") is True and data.get("gameplayAuthority") is False, "visual polish must remain presentation-only")

    direction = data["direction"]
    landmarks = set(direction["signatureLandmarks"])
    require({"research-dome", "waterfalls", "bridges-and-arches", "renewable-energy-silhouettes"}.issubset(landmarks), "signature eco-futurist landmark set is incomplete")
    require("curved-modules" in direction["architecture"]["preferred"] and "domes" in direction["architecture"]["preferred"], "curved architecture language is missing")
    require("voxel-grid-buildings" in direction["architecture"]["avoidAsPrimaryIdentity"], "policy must reject voxel-grid identity")

    fp = data["firstPerson"]
    require(set(fp["modes"]) == {"explore", "build", "scan", "measure", "observe"}, "first-person mode vocabulary drifted")
    require(0 < float(fp["maxToolScreenFraction"]) <= 0.25, "tool footprint must remain compact")
    require(0 <= float(fp["maxCameraBobCm"]) <= 1.5, "camera bob ceiling invalid")
    require(0 <= float(fp["maxToolLagDegrees"]) <= 4.0, "tool lag ceiling invalid")

    hud = data["hud"]["rules"]
    require(hud["maxLargePanelsAtOnce"] <= 2, "HUD may not normalize concept-art panel saturation")
    for key in ("onlyOneContextPanelAtOnce", "buildPaletteOnlyInBuildMode", "sciencePanelOnlyDuringScienceContext", "exploreMustRemainVisuallyQuiet"):
        require(hud.get(key) is True, f"HUD contextual rule missing: {key}")
    require(hud.get("colorOnlyStateAllowed") is False, "HUD state may not depend on color alone")

    personalities = data["characters"]["motionPersonality"]
    require(set(personalities) == {"curious-explorer", "scientist-inventor", "nature-guardian", "knowledge-explorer"}, "motion personality set drifted")
    for name, style in personalities.items():
        require(0.80 <= float(style["strideScale"]) <= 1.15, f"{name} stride scale out of bounds")
        require(0.75 <= float(style["armSwingScale"]) <= 1.15, f"{name} arm swing scale out of bounds")
        require(0.80 <= float(style["headLookScale"]) <= 1.20, f"{name} head-look scale out of bounds")
        require(0.10 <= float(style["settleSeconds"]) <= 0.30, f"{name} settle out of bounds")
        require(0.0 <= float(style["secondaryMotionScale"]) <= 1.0, f"{name} secondary motion out of bounds")

    actions = set(data["animation"]["firstPersonActions"])
    require({"scan-anticipate", "scan-hold", "scan-settle", "build-point", "build-confirm", "measure-focus", "observe-focus"}.issubset(actions), "first-person interaction animation vocabulary is incomplete")
    require(data["animation"]["rootMotionDefault"] is False, "root motion must not become default movement authority")

    combined = HEADER.read_text(encoding="utf-8") + "\n" + CPP.read_text(encoding="utf-8")
    for token in ("EWMFirstPersonVisualMode", "EWMExplorerMotionPersonality", "FWMFirstPersonVisualProfile", "FWMExplorerMotionStyle", "ResolveFirstPersonProfile", "ResolveMotionStyle", "IsContextPanelAllowed"):
        require(token in combined, f"runtime contract missing: {token}")
    for forbidden in ("AddMovementInput", "SetActorLocation", "RecordComposableEvidence", "GrantReward", "SetGlobalTimeDilation"):
        require(forbidden not in combined, f"presentation runtime must not acquire gameplay authority: {forbidden}")
    require("CameraBobCm = 0.0f" in combined and "ToolLagDegrees = 0.0f" in combined, "Reduced Motion must remove first-person bob/lag")

    tests = TESTS.read_text(encoding="utf-8")
    for name in (
        "WorldMakers.Visual.ReferencePolish.FirstPersonModesRemainReadable",
        "WorldMakers.Visual.ReferencePolish.ContextHUDIsModeBound",
        "WorldMakers.Visual.ReferencePolish.ReducedMotionRemovesFirstPersonLag",
        "WorldMakers.Visual.ReferencePolish.MotionPersonalityChangesPoseNotGameplay",
    ):
        require(name in tests, f"missing automation test: {name}")

    doc = DOC.read_text(encoding="utf-8").lower()
    for phrase in ("research dome", "first-person", "no more than two large panels", "motion personalities", "reduced motion", "directional references"):
        require(phrase in doc, f"documentation missing: {phrase}")

    workflow = WORKFLOW.read_text(encoding="utf-8")
    require("python scripts/validate-reference-driven-visual-polish.py" in workflow, "Repository Quality must execute reference-driven polish gate")

    print("Reference-driven visual/animation polish validated: eco-futurist composition, contextual first-person HUD, bounded tool motion, distinct character motion personalities and Reduced Motion safeguards are present.")


if __name__ == "__main__":
    main()
