#!/usr/bin/env python3
"""Validate the locked World Makers Unreal production baseline.

This source-level gate keeps the machine-readable production contract coherent
with the engine lock, project layout, LFS policy, canonical runtime performance
catalog and human-readable documentation. It does not claim native Unreal or
device certification.
"""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASELINE_PATH = ROOT / "content" / "production" / "unreal-production-baseline-v1.json"
PERFORMANCE_PATH = ROOT / "content" / "performance" / "tablet-performance-profiles.json"
DOC_PATH = ROOT / "docs" / "unreal-production-baseline-v1.md"
ENGINE_VERSION_PATH = ROOT / "game" / "UNREAL_ENGINE_VERSION"
UPROJECT_PATH = ROOT / "game" / "WorldMakers.uproject"
GITATTRIBUTES_PATH = ROOT / ".gitattributes"
README_PATH = ROOT / "README.md"

EXPECTED_SCHEMA = "worldmakers.unreal-production-baseline.v1"
EXPECTED_ENGINE = "5.8.2"
EXPECTED_PROJECT_PATH = "game/WorldMakers.uproject"
EXPECTED_MAP = "/Game/WorldMakers/Maps/WM_PrototypeCertification"

EXPECTED_ARCHITECTURE_BOUNDARIES = {
    "gameplay-rules-independent-from-rendering",
    "science-simulation-independent-from-vfx",
    "mission-evidence-independent-from-presentation",
    "save-state-independent-from-transient-uobject-details",
    "child-runtime-independent-from-payment-sdk",
    "visual-assets-replaceable-without-changing-stable-gameplay-ids",
}

EXPECTED_CAPABILITIES = {
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

EXPECTED_GATES = {
    "G0": {
        "name": "Source and architecture readiness",
        "requires": {
            "hosted-source-ci-green",
            "production-baseline-validator-green",
            "lfs-policy-clean",
            "ue-source-preflight-green",
        },
    },
    "G1": {
        "name": "Native Unreal readiness",
        "requires": {
            "worldmakerseditor-win64-development-build-green",
            "worldmakers-automation-green",
            "no-known-fatal-compiler-errors",
        },
    },
    "G2": {
        "name": "Authored vertical slice",
        "requires": {
            "certification-map-committed-via-lfs",
            "first-person-route-playable",
            "authored-assets-resolve-at-stable-paths",
        },
    },
    "G3": {
        "name": "Visual and animation target",
        "requires": {
            "lighting-material-animation-vfx-quality-review",
            "reduced-motion-review",
            "no-placeholder-proxies-in-certified-route",
        },
    },
    "G4": {
        "name": "Performance certification",
        "requires": {
            "desktop-reference-budget-pass",
            "representative-ipados-evidence",
            "representative-android-evidence",
            "fail-closed-certification-result",
        },
    },
}

PERFORMANCE_BINDINGS = {
    "tabletLow": "performance.tablet.low",
    "tabletMedium": "performance.tablet.medium",
    "tabletHigh": "performance.tablet.high",
    "desktopReference": "performance.desktop.reference",
}


def fail(message: str) -> None:
    raise SystemExit(f"ERROR: {message}")


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def load_text(path: Path) -> str:
    require(path.is_file(), f"required file missing: {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8")


def load_json(path: Path, label: str) -> dict:
    try:
        value = json.loads(load_text(path))
    except json.JSONDecodeError as exc:
        fail(f"{label} JSON is invalid: {exc}")
    require(isinstance(value, dict), f"{label} root must be an object")
    return value


def close_number(left: object, right: object, tolerance: float = 0.01) -> bool:
    try:
        return abs(float(left) - float(right)) < tolerance
    except (TypeError, ValueError):
        return False


def validate_performance_contract(baseline: dict) -> None:
    catalog = load_json(PERFORMANCE_PATH, "runtime performance catalog")
    profiles = catalog.get("profiles")
    require(isinstance(profiles, list), "runtime performance catalog profiles must be a list")

    by_id: dict[str, dict] = {}
    for profile in profiles:
        require(isinstance(profile, dict), "runtime performance profile must be an object")
        profile_id = profile.get("id")
        require(isinstance(profile_id, str) and profile_id, "runtime performance profile id is missing")
        require(profile_id not in by_id, f"duplicate runtime performance profile id: {profile_id}")
        by_id[profile_id] = profile

    rendering = baseline.get("rendering", {})
    require(rendering.get("visualTarget") == "premium-stylized-high-fidelity", "visual target drifted")

    for baseline_tier, profile_id in PERFORMANCE_BINDINGS.items():
        require(baseline_tier in rendering, f"baseline rendering tier missing: {baseline_tier}")
        require(profile_id in by_id, f"runtime performance profile missing: {profile_id}")
        tier = rendering[baseline_tier]
        profile = by_id[profile_id]
        require(tier.get("targetFps") == profile.get("targetFps"), f"{baseline_tier} FPS disagrees with {profile_id}")
        require(
            close_number(tier.get("maxP95FrameMs"), profile.get("frameTimeBudgetMs")),
            f"{baseline_tier} p95 frame budget disagrees with {profile_id}",
        )
        require(
            tier.get("screenPercentage") == profile.get("screenPercentage"),
            f"{baseline_tier} screen percentage disagrees with {profile_id}",
        )


def validate_gates(baseline: dict) -> None:
    gates = baseline.get("productionGates")
    require(isinstance(gates, list), "productionGates must be a list")
    require(len(gates) == len(EXPECTED_GATES), "productionGates must contain exactly G0-G4 once")

    by_id: dict[str, dict] = {}
    for gate in gates:
        require(isinstance(gate, dict), "production gate must be an object")
        gate_id = gate.get("id")
        require(isinstance(gate_id, str), "production gate id is missing")
        require(gate_id not in by_id, f"duplicate production gate: {gate_id}")
        by_id[gate_id] = gate

    require(set(by_id) == set(EXPECTED_GATES), "production gate IDs drifted from G0-G4")
    for gate_id, expected in EXPECTED_GATES.items():
        gate = by_id[gate_id]
        require(gate.get("name") == expected["name"], f"{gate_id} name drifted")
        requirements = gate.get("requires")
        require(isinstance(requirements, list), f"{gate_id} requires must be a list")
        require(set(requirements) == expected["requires"], f"{gate_id} required evidence drifted")
        require(len(requirements) == len(expected["requires"]), f"{gate_id} contains duplicate evidence requirements")


def main() -> int:
    baseline = load_json(BASELINE_PATH, "production baseline")

    require(baseline.get("schema") == EXPECTED_SCHEMA, "unexpected production baseline schema")
    require(baseline.get("version") == 1, "production baseline version must remain 1")
    require(baseline.get("status") == "locked-for-production-readiness", "production baseline must remain locked")

    engine = baseline.get("engine", {})
    require(engine.get("family") == "5.8", "engine family must remain 5.8")
    require(engine.get("certificationPatch") == EXPECTED_ENGINE, "certification patch must remain 5.8.2")
    require(engine.get("projectPath") == EXPECTED_PROJECT_PATH, "project path drifted")
    require(engine.get("certificationMap") == EXPECTED_MAP, "certification map path drifted")
    require(load_text(ENGINE_VERSION_PATH).strip() == EXPECTED_ENGINE, "UNREAL_ENGINE_VERSION mismatch")
    require(UPROJECT_PATH.is_file(), "WorldMakers.uproject is missing")

    product = baseline.get("product", {})
    require(product.get("primaryRuntimeView") == "first-person", "primary runtime view must remain first-person")
    require(product.get("initialBiome") == "caribbean-rainforest", "initial production biome drifted")
    require(product.get("designRule") == "learning-is-structural-to-play", "core product design rule drifted")

    architecture = baseline.get("architecture", {})
    require(architecture.get("authoritativeGameplay") == "cpp-and-data-driven-runtime", "authoritative gameplay boundary drifted")
    require(architecture.get("blueprintRole") == "composition-presentation-and-safe-extension", "Blueprint production role drifted")
    require(architecture.get("prohibitedPattern") == "blueprint-only-authoritative-gameplay", "Blueprint-only authority prohibition missing")
    boundaries = architecture.get("requiredBoundaries")
    require(isinstance(boundaries, list), "architecture.requiredBoundaries must be a list")
    require(set(boundaries) == EXPECTED_ARCHITECTURE_BOUNDARIES, "mandatory architecture boundaries drifted")
    require(len(boundaries) == len(EXPECTED_ARCHITECTURE_BOUNDARIES), "architecture.requiredBoundaries contains duplicates")

    validate_performance_contract(baseline)

    first_person = set(baseline.get("animation", {}).get("firstPerson", []))
    for token in ("authored-skeletal-arms", "authored-tools", "animation-blueprint", "ik-rig", "control-rig", "motion-warping-where-contextual", "reduced-motion-support"):
        require(token in first_person, f"first-person animation contract missing: {token}")

    full_body = set(baseline.get("animation", {}).get("fullBodyHighFidelity", []))
    for token in ("motion-matching-evaluation", "full-body-ik", "terrain-adaptation"):
        require(token in full_body, f"full-body animation contract missing: {token}")

    world = baseline.get("world", {})
    require(world.get("streaming") == "world-partition-target", "world streaming contract drifted")
    require(world.get("proceduralGeneration") == "pcg-assisted-not-fully-procedural", "PCG contract drifted")
    require(world.get("authorshipRule") == "procedural-macro-distribution-plus-handcrafted-points-of-interest", "world authorship rule drifted")

    capabilities = baseline.get("verticalSlice", {}).get("requiredCapabilities")
    require(isinstance(capabilities, list), "verticalSlice.requiredCapabilities must be a list")
    require(set(capabilities) == EXPECTED_CAPABILITIES, "vertical-slice required capabilities drifted")
    require(len(capabilities) == len(EXPECTED_CAPABILITIES), "vertical-slice required capabilities contain duplicates")

    validate_gates(baseline)

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
        "save contracts independent from transient UObject implementation details",
        "visual assets replaceable without changing stable gameplay IDs",
    ):
        require(token in doc, f"production baseline document missing required token: {token}")

    readme = load_text(README_PATH)
    require("docs/unreal-production-baseline-v1.md" in readme, "README must reference the production baseline")
    require("content/production/unreal-production-baseline-v1.json" in readme, "README must reference the machine-readable baseline")

    print("World Makers Unreal Production Baseline v1: PASS")
    print(f"  Engine: {EXPECTED_ENGINE}")
    print(f"  Project: {EXPECTED_PROJECT_PATH}")
    print(f"  Certification map: {EXPECTED_MAP}")
    print("  Runtime performance catalog: coherent")
    print("  Architecture boundaries: coherent")
    print("  Gates: G0 -> G1 -> G2 -> G3 -> G4")
    print("  NOTE: source governance only; native/device certification remains separate")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
