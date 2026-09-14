#!/usr/bin/env python3
"""Validate G3 visual-fidelity source contracts without claiming native certification."""
from __future__ import annotations

import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "content/production/g3-visual-fidelity-v1.json"
REVIEW = ROOT / "content/production/g3-visual-review-template.json"
G2 = ROOT / "content/production/g2-authored-vertical-slice-v1.json"
P4 = ROOT / "content/visual/authored/p4-motion-vfx-presentation.json"
P5 = ROOT / "content/visual/authored/p5-art-polish-certification.json"
ASSESSOR = ROOT / "scripts/assess-g3-visual-fidelity.py"
ORCHESTRATOR = ROOT / "scripts/run-g3-visual-certification.ps1"
INSPECTOR = ROOT / "scripts/unreal/inspect-g3-visual-map.py"
INVENTORY = ROOT / "scripts/unreal/collect-p5-native-inventory.py"
WORKFLOW = ROOT / ".github/workflows/g3-visual-fidelity.yml"
LAUNCHER = ROOT / "WorldMakers-G3-Certify.cmd"
DOC = ROOT / "docs/g3-visual-fidelity-presentation.md"

EXPECTED_EVIDENCE = {
    "visual-contact-sheet",
    "animation-review",
    "vfx-overdraw",
    "camera-ui-review",
}


def fail(message: str) -> None:
    print(f"ERROR: {message}")
    raise SystemExit(1)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def text(path: Path) -> str:
    require(path.is_file(), f"missing required file: {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8-sig")


def data(path: Path) -> dict:
    try:
        return json.loads(text(path))
    except json.JSONDecodeError as exc:
        fail(f"invalid JSON in {path.relative_to(ROOT)}: {exc}")


def tokens(path: Path, required: tuple[str, ...]) -> None:
    payload = text(path)
    for token in required:
        require(token in payload, f"{path.relative_to(ROOT)} missing required token: {token}")


def main() -> int:
    contract = data(CONTRACT)
    g2 = data(G2)
    p4 = data(P4)
    p5 = data(P5)
    review = data(REVIEW)

    require(contract.get("schema") == "worldmakers.g3-visual-fidelity.v1", "G3 schema drift")
    require(contract.get("status") == "source-ready-native-visual-review-required", "G3 status drift")
    require(contract.get("engine", {}).get("expectedVersion") == "5.8.2", "G3 must remain locked to UE 5.8.2")
    require(contract.get("certificationMap") == g2.get("map", {}).get("packagePath"), "G3 map must be the G2 certification map")

    deps = contract.get("dependencies", {})
    require(deps.get("g2Contract") == "content/production/g2-authored-vertical-slice-v1.json", "G3 G2 dependency drift")
    require(deps.get("p4Manifest") == "content/visual/authored/p4-motion-vfx-presentation.json", "G3 P4 dependency drift")
    require(deps.get("p5PolishContract") == "content/visual/authored/p5-art-polish-certification.json", "G3 P5 dependency drift")
    require(deps.get("p5NativeInventoryCollector") == "scripts/unreal/collect-p5-native-inventory.py", "G3 must reuse the P5 native inventory collector")

    quality = contract.get("qualityBar", {})
    require(quality.get("characterAnimation", {}).get("requiredAnimationClipCount") == p4.get("animation", {}).get("clipCount") == 17, "G3 animation count must match P4")
    require(quality.get("vfx", {}).get("requiredEffectCount") == p4.get("vfx", {}).get("effectCount") == 17, "G3 VFX count must match P4")
    require(quality.get("presentation", {}).get("requiredLevelSequenceCount") == p4.get("presentation", {}).get("sequenceCount") == 2, "G3 Level Sequence count must match P4")
    require(quality.get("characterAnimation", {}).get("maxObservedFootSlideCm") == p5["polishTargets"]["characterAnimation"]["maxObservedFootSlideCm"], "G3 foot-slide threshold must match P5")
    require(quality.get("environment", {}).get("maxTexturePoolOverBudgetMB") == p5["polishTargets"]["environment"]["maxTexturePoolOverBudgetMB"], "G3 texture pool threshold must match P5")
    require(quality.get("presentation", {}).get("maxRevealDurationSeconds") == p5["polishTargets"]["presentation"]["maxRevealDurationSeconds"], "G3 reveal ceiling must match P5")
    require(quality.get("presentation", {}).get("inputLockAllowed") is False, "G3 must forbid input lock")
    require(quality.get("presentation", {}).get("forcedViewTargetAllowed") is False, "G3 must forbid forced ViewTarget")
    require(quality.get("presentation", {}).get("globalTimeScaleChangeAllowed") is False, "G3 must forbid global time-scale changes")

    require(set(contract.get("requiredEvidenceKinds", [])) == EXPECTED_EVIDENCE, "G3 evidence-kind set drift")
    cert = contract.get("certification", {})
    for field in (
        "requiresG2CertifiedSameCommit",
        "requiresNativeInventory",
        "requiresNativeMapInspection",
        "requiresHumanVisualReview",
        "requiresEvidenceHashes",
    ):
        require(cert.get(field) is True, f"G3 certification must require {field}")
    require(cert.get("resultStates") == ["CERTIFIED", "BLOCKED", "NON_CERTIFYING_PASS"], "G3 result states drift")
    require("android-and-ipados-performance-certification" in contract.get("gateBoundary", {}).get("g4Owns", []), "G4 performance boundary must remain explicit")

    require(review.get("schemaVersion") == 1 and review.get("status") == "pending", "G3 review template must remain pending")
    require(review.get("buildCommit") == "", "G3 review template may not pre-bind a commit")
    require(review.get("unrealVersion") == "5.8.2", "G3 review template Unreal version drift")
    artifact_kinds = {item.get("kind") for item in review.get("artifacts", [])}
    require(artifact_kinds == EXPECTED_EVIDENCE, "G3 review artifact set drift")
    require(all(not item.get("file") and not item.get("sha256") for item in review.get("artifacts", [])), "G3 review template may not contain fabricated evidence")

    tokens(ASSESSOR, ("worldmakers.g3-visual-fidelity-result.v1", "validate_inventory", "G2 must be CERTIFIED", "sha256_file", "CERTIFIED"))
    tokens(INSPECTOR, ("/Game/WorldMakers/Maps/WM_PrototypeCertification", "WM_G2_Rainforest", "DirectionalLightComponent", "SkyLightComponent", "SkyAtmosphereComponent", "ExponentialHeightFogComponent"))
    tokens(INVENTORY, ("WM_P5_NATIVE_INVENTORY", "animationAssets", "vfxAssets", "presentationAssets"))
    tokens(ORCHESTRATOR, ("run-g2-authored-certification.ps1", "collect-p5-native-inventory.py", "inspect-g3-visual-map.py", "assess-g3-visual-fidelity.py", "clean current main"))
    tokens(WORKFLOW, ("G3 Visual Fidelity", "validate-g3-visual-fidelity.py", "G3_NATIVE_VISUAL_ENABLED", "self-hosted"))
    tokens(LAUNCHER, ("run-g3-visual-certification.ps1",))
    tokens(DOC, ("G3", "G2", "G4", "WorldMakers-G3-Certify.cmd", "visual-contact-sheet"))

    print("World Makers G3 Visual Fidelity source contract: PASS")
    print("  Native visual certification: intentionally not claimed by source CI")
    print("  Device performance certification: reserved for G4")
    return 0


if __name__ == "__main__":
    sys.exit(main())
