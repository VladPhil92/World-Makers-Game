#!/usr/bin/env python3
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
    require(rendering[tier]["targetFps"] == fps, f"{tier} FPS drift")
    require(abs(float(rendering[tier]["maxP95FrameMs"]) - p95) < 0.01, f"{tier} p95 drift")

first_person = set(baseline["animation"]["firstPerson"])
for token in ("authored-skeletal-arms", "animation-blueprint", "ik-rig", "control-rig", "reduced-motion-support"):
    require(token in first_person, f"animation contract missing {token}")

required_capabilities = {
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

readme = text(README)
require("docs/unreal-production-baseline-v1.md" in readme, "README baseline link missing")
require("content/production/unreal-production-baseline-v1.json" in readme, "README machine baseline link missing")

print("World Makers Unreal Production Baseline v1: PASS")
print("Engine 5.8.2 | first-person | premium-stylized-high-fidelity | G0 -> G4")
print("Source governance only; native Unreal certification remains separate.")
