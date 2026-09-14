#!/usr/bin/env python3
"""Validate the locked World Makers Unreal production baseline.

This is intentionally a source-level contract. It prevents drift between the
machine-readable production baseline, engine lock, project layout, LFS policy
and documentation. It does not claim native Unreal certification.
"""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASELINE = ROOT / "content/production/unreal-production-baseline-v1.json"
DOC = ROOT / "docs/unreal-production-baseline-v1.md"
ENGINE = ROOT / "game/UNREAL_ENGINE_VERSION"
UPROJECT = ROOT / "game/WorldMakers.uproject"
ATTRS = ROOT / ".gitattributes"
README = ROOT / "README.md"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"ERROR: {message}")


def text(path: Path) -> str:
    require(path.is_file(), f"missing {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8")


baseline = json.loads(text(BASELINE))
require(baseline.get("schema") == "worldmakers.unreal-production-baseline.v1", "schema drift")
require(baseline.get("status") == "locked-for-production-readiness", "baseline is not locked")

engine = baseline["engine"]
require(engine["family"] == "5.8", "engine family drift")
require(engine["certificationPatch"] == "5.8.2", "engine patch drift")
require(engine["projectPath"] == "game/WorldMakers.uproject", "project path drift")
require(engine["certificationMap"] == "/Game/WorldMakers/Maps/WM_PrototypeCertification", "certification map drift")
require(text(ENGINE).strip() == "5.8.2", "UNREAL_ENGINE_VERSION mismatch")
require(UPROJECT.is_file(), "WorldMakers.uproject missing")

require(baseline["product"]["primaryRuntimeView"] == "first-person", "runtime view drift")
require(baseline["product"]["initialBiome"] == "caribbean-rainforest", "initial biome drift")
require(baseline["architecture"]["authoritativeGameplay"] == "cpp-and-data-driven-runtime", "gameplay authority drift")
require(baseline["architecture"]["blueprintRole"] == "composition-presentation-and-safe-extension", "Blueprint role drift")
require(baseline["rendering"]["visualTarget"] == "premium-stylized-high-fidelity", "visual target drift")

product = baseline["product"]
require(product["primaryRuntimeView"] == "first-person", "runtime view drift")
require(product["initialBiome"] == "caribbean-rainforest", "initial biome drift")

architecture = baseline["architecture"]
require(architecture["authoritativeGameplay"] == "cpp-and-data-driven-runtime", "authoritative gameplay boundary drift")
require(architecture["blueprintRole"] == "composition-presentation-and-safe-extension", "Blueprint role drift")
require(architecture["prohibitedPattern"] == "blueprint-only-authoritative-gameplay", "Blueprint authority prohibition missing")

rendering = baseline["rendering"]
require(rendering["visualTarget"] == "premium-stylized-high-fidelity", "visual target drift")
for tier, fps, p95 in (
    ("tabletLow", 30, 33.34),
    ("tabletMedium", 30, 33.34),
    ("tabletHigh", 60, 16.67),
    ("desktopReference", 60, 16.67),
):
    value = baseline["rendering"][tier]
    require(value["targetFps"] == fps, f"{tier} FPS drift")
    require(abs(float(value["maxP95FrameMs"]) - p95) < 0.01, f"{tier} p95 drift")
    require(rendering[tier]["targetFps"] == fps, f"{tier} FPS drift")
    require(abs(float(rendering[tier]["maxP95FrameMs"]) - p95) < 0.01, f"{tier} p95 drift")

first_person = set(baseline["animation"]["firstPerson"])
for token in ("authored-skeletal-arms", "animation-blueprint", "ik-rig", "control-rig", "reduced-motion-support"):
    require(token in first_person, f"animation contract missing {token}")

required = {
required_capabilities = {
import sys

ROOT = Path(__file__).resolve().parents[1]
BASELINE_PATH = ROOT / "content" / "production" / "unreal-production-baseline-v1.json"
DOC_PATH = ROOT / "docs" / "unreal-production-baseline-v1.md"
ENGINE_VERSION_PATH = ROOT / "game" / "UNREAL_ENGINE_VERSION"
UPROJECT_PATH = ROOT / "game" / "WorldMakers.uproject"
GITATTRIBUTES_PATH = ROOT / ".gitattributes"
README_PATH = ROOT / "README.md"

EXPECTED_SCHEMA = "worldmakers.unreal-production-baseline.v1"
EXPECTED_ENGINE = "5.8.2"
EXPECTED_PROJECT_PATH = "game/WorldMakers.uproject"
EXPECTED_MAP = "/Game/WorldMakers/Maps/WM_PrototypeCertification"
EXPECTED_VIEW = "first-person"

REQUIRED_CAPABILITIES = {
    "spawn-and-first-person-control",
    "observe-and-scan",
    "collect-resource",
    "perform-science-interaction",
    "craft-or-transform-material",
    "build-or-place-intervention",
    "trigger-visible-ecosystem-consequence",
    "complete-mission-evidence",
    "save-and-load-state",
}
require(required <= set(baseline["verticalSlice"]["requiredCapabilities"]), "vertical-slice capability drift")
require(required_capabilities <= set(baseline["verticalSlice"]["requiredCapabilities"]), "vertical-slice capability drift")
require({g["id"] for g in baseline["productionGates"]} == {"G0", "G1", "G2", "G3", "G4"}, "production gate drift")

attrs = text(ATTRS)
require("*.uasset filter=lfs" in attrs, "uasset LFS rule missing")
require("*.umap filter=lfs" in attrs, "umap LFS rule missing")

doc = text(DOC)
for token in (
    "Unreal Engine 5.8.2",
    "/Game/WorldMakers/Maps/WM_PrototypeCertification",
    "premium stylized high fidelity",
    "Motion Matching",
    "World Partition",
    "PCG",
    "G0 — Source and architecture readiness",
    "G4 — Performance certification",
):
    require(token in doc, f"baseline document missing {token}")

print("World Makers Unreal Production Baseline v1: PASS")
print("Engine 5.8.2 | first-person | premium-stylized-high-fidelity | G0 -> G4")
print("Source governance only; native Unreal certification remains separate.")
readme = text(README)
require("docs/unreal-production-baseline-v1.md" in readme, "README baseline link missing")
require("content/production/unreal-production-baseline-v1.json" in readme, "README machine baseline link missing")

print("World Makers Unreal Production Baseline v1: PASS")
print("Engine 5.8.2 | first-person | premium-stylized-high-fidelity | G0 -> G4")
print("Source governance only; native Unreal certification remains separate.")

EXPECTED_GATES = {"G0", "G1", "G2", "G3", "G4"}


def fail(message: str) -> None:
    print(f"ERROR: {message}")
    raise SystemExit(1)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def load_text(path: Path) -> str:
    require(path.is_file(), f"required file missing: {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8")


def main() -> int:
    require(BASELINE_PATH.is_file(), "production baseline JSON is missing")
    try:
        baseline = json.loads(BASELINE_PATH.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        fail(f"baseline JSON is invalid: {exc}")

    require(baseline.get("schema") == EXPECTED_SCHEMA, "unexpected baseline schema")
    require(baseline.get("status") == "locked-for-production-readiness", "baseline must be locked")

    engine = baseline.get("engine", {})
    require(engine.get("family") == "5.8", "engine family must remain 5.8")
    require(engine.get("certificationPatch") == EXPECTED_ENGINE, "certification patch must remain 5.8.2")
    require(engine.get("projectPath") == EXPECTED_PROJECT_PATH, "project path drifted")
    require(engine.get("certificationMap") == EXPECTED_MAP, "certification map path drifted")

    product = baseline.get("product", {})
    require(product.get("primaryRuntimeView") == EXPECTED_VIEW, "primary runtime view must remain first-person")
    require(product.get("initialBiome") == "caribbean-rainforest", "initial production biome drifted")

    architecture = baseline.get("architecture", {})
    require(architecture.get("authoritativeGameplay") == "cpp-and-data-driven-runtime", "authoritative gameplay boundary drifted")
    require(architecture.get("blueprintRole") == "composition-presentation-and-safe-extension", "Blueprint production role drifted")
    require(architecture.get("prohibitedPattern") == "blueprint-only-authoritative-gameplay", "Blueprint-only authority prohibition missing")

    rendering = baseline.get("rendering", {})
    require(rendering.get("visualTarget") == "premium-stylized-high-fidelity", "visual target drifted")
    for tier_name, fps, p95 in (
        ("tabletLow", 30, 33.34),
        ("tabletMedium", 30, 33.34),
        ("tabletHigh", 60, 16.67),
        ("desktopReference", 60, 16.67),
    ):
        tier = rendering.get(tier_name, {})
        require(tier.get("targetFps") == fps, f"{tier_name} FPS target drifted")
        require(abs(float(tier.get("maxP95FrameMs", -1)) - p95) < 0.01, f"{tier_name} p95 target drifted")

    animation = baseline.get("animation", {})
    first_person = set(animation.get("firstPerson", []))
    for required in {"authored-skeletal-arms", "animation-blueprint", "ik-rig", "control-rig", "reduced-motion-support"}:
        require(required in first_person, f"first-person animation contract missing: {required}")

    vertical_slice = baseline.get("verticalSlice", {})
    capabilities = set(vertical_slice.get("requiredCapabilities", []))
    missing_capabilities = sorted(REQUIRED_CAPABILITIES - capabilities)
    require(not missing_capabilities, f"vertical-slice capabilities missing: {', '.join(missing_capabilities)}")

    gates = {gate.get("id") for gate in baseline.get("productionGates", []) if isinstance(gate, dict)}
    require(gates == EXPECTED_GATES, f"production gates must be exactly {sorted(EXPECTED_GATES)}")

    engine_version = load_text(ENGINE_VERSION_PATH).strip()
    require(engine_version == EXPECTED_ENGINE, f"UNREAL_ENGINE_VERSION is {engine_version!r}, expected {EXPECTED_ENGINE!r}")
    require(UPROJECT_PATH.is_file(), "WorldMakers.uproject is missing")

    gitattributes = load_text(GITATTRIBUTES_PATH)
    require("*.uasset filter=lfs" in gitattributes, "*.uasset must remain tracked by Git LFS")
    require("*.umap filter=lfs" in gitattributes, "*.umap must remain tracked by Git LFS")

    doc = load_text(DOC_PATH)
    for token in (
        "Unreal Engine 5.8.2",
        EXPECTED_MAP,
        "premium stylized high fidelity",
        "Motion Matching",
        "World Partition",
        "PCG",
        "G0 — Source and architecture readiness",
        "G4 — Performance certification",
    ):
        require(token in doc, f"production baseline document missing required token: {token}")

    readme = load_text(README_PATH)
    require("docs/unreal-production-baseline-v1.md" in readme, "README must reference the production baseline")
    require("content/production/unreal-production-baseline-v1.json" in readme, "README must reference the machine-readable baseline")

    print("World Makers Unreal Production Baseline v1: PASS")
    print(f"  Engine: {EXPECTED_ENGINE}")
    print(f"  Project: {EXPECTED_PROJECT_PATH}")
    print(f"  Certification map: {EXPECTED_MAP}")
    print("  Gates: G0 -> G1 -> G2 -> G3 -> G4")
    print("  NOTE: this is source governance, not native Unreal certification")
    return 0


if __name__ == "__main__":
    sys.exit(main())
