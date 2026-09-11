#!/usr/bin/env python3
"""C2 Planetary Commons & Living World Simulation source gate."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

CANONICAL = ROOT / "content/environment/planetary-commons-living-world-v1.json"
STAGED = ROOT / "game/Content/WorldMakers/Environment/planetary-commons-living-world-v1.json"
C1 = ROOT / "content/citizenship/xxii-century-citizenship-v1.json"

REQUIRED_FILES = (
    "docs/c2-planetary-commons-living-world.md",
    "content/environment/planetary-commons-living-world-v1.json",
    "game/Content/WorldMakers/Environment/planetary-commons-living-world-v1.json",
    "game/Source/WorldMakers/Environment/WMPlanetaryCommonsTypes.h",
    "game/Source/WorldMakers/Environment/WMPlanetaryCommonsTypes.cpp",
    "game/Source/WorldMakers/Environment/WMPlanetaryCommonsSubsystem.h",
    "game/Source/WorldMakers/Environment/WMPlanetaryCommonsSubsystem.cpp",
    "game/Source/WorldMakers/Private/Tests/WMPlanetaryCommonsTests.cpp",
)


def fail(message: str) -> None:
    raise SystemExit(message)


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def bounded(value: object) -> bool:
    return isinstance(value, (int, float)) and 0 <= float(value) <= 1


def main() -> None:
    missing = [path for path in REQUIRED_FILES if not (ROOT / path).exists()]
    if missing:
        fail(f"Missing C2 files: {missing}")

    if CANONICAL.read_bytes() != STAGED.read_bytes():
        fail("Canonical and packaged C2 simulation contracts must be byte-identical")

    c1 = json.loads(C1.read_text(encoding="utf-8"))
    data = json.loads(CANONICAL.read_text(encoding="utf-8"))

    if data.get("schemaVersion") != 1:
        fail("C2 schemaVersion must remain 1")
    if data.get("simulationId") != "simulation.planetary-commons-living-world.v1":
        fail("C2 simulationId drifted")
    if data.get("prototypeOnly") is not True or data.get("deterministicStepModel") is not True:
        fail("C2 must remain prototype-only and deterministic-step based")
    if data.get("privacyModel") != "stable-ids-no-child-free-text":
        fail("C2 privacy boundary drifted")

    c1_hooks = set(c1.get("simulationHooks", []))
    c2_hooks = set(data.get("simulationHooks", []))
    if c2_hooks != c1_hooks or len(c2_hooks) != 11:
        fail(f"C2 must implement exactly the 11 C1 simulation hooks: {sorted(c2_hooks)}")
    if set(data.get("derivedHooks", [])) != {"habitat-connectivity", "animal-stress"}:
        fail("C2 habitat connectivity and animal stress must remain derived")

    legacy = data.get("legacyAdapter", {})
    if legacy.get("sourceId") != "m3.4-reactive-ecosystem":
        fail("C2 must explicitly adapt M3.4 rather than replace it silently")
    if legacy.get("legacyRemainsLocalCareAuthority") is not True or legacy.get("c2OwnsCitizenshipSimulationHooks") is not True:
        fail("C2 authority boundary is incomplete")

    state = data.get("initialState", {})
    required_state = {
        "waterQuality", "airQuality", "soilHealth", "biodiversity", "energyReliability",
        "materialDemand", "computeEnergyDemand", "humanWellbeing", "civicLegitimacy",
    }
    if set(state) != required_state or not all(bounded(state[key]) for key in required_state):
        fail("C2 primary initial state must expose exactly nine bounded authored channels")
    if "animalStress" in state or "habitatConnectivity" in state:
        fail("Derived C2 hooks must not be authored in initialState")

    links = data.get("habitatLinks", [])
    if len(links) < 3:
        fail("C2 requires a nontrivial habitat-link network")
    link_ids = [item.get("linkId") for item in links]
    if len(link_ids) != len(set(link_ids)) or any(not item.get("linkId") or not bounded(item.get("initialPermeability")) for item in links):
        fail("Habitat links must have unique stable IDs and bounded permeability")

    species = data.get("speciesProfiles", [])
    if len(species) < 3:
        fail("C2 must model at least three distinct living-world profiles")
    species_ids = [item.get("speciesId") for item in species]
    if len(species_ids) != len(set(species_ids)):
        fail("Species IDs must be unique")
    if any(item.get("scientificReviewState") != "draft" for item in species):
        fail("Prototype species profiles must remain scientific-review draft")
    need_keys = (
        "minWaterQuality", "minAirQuality", "minSoilHealth", "minBiodiversity",
        "minHabitatConnectivity", "disturbanceSensitivity",
    )
    need_vectors = []
    for item in species:
        if not item.get("speciesId", "").startswith("species.prototype."):
            fail("Unreviewed C2 species must use explicit prototype IDs")
        if not all(bounded(item.get(key)) for key in need_keys):
            fail(f"Species needs must be bounded: {item.get('speciesId')}")
        need_vectors.append(tuple(item[key] for key in need_keys))
    if len(set(need_vectors)) != len(need_vectors):
        fail("C2 may not collapse distinct species into identical need profiles")

    interventions = data.get("interventions", [])
    if len(interventions) < 6:
        fail("C2 requires restorative, harmful and technology trade-off interventions")
    ids = [item.get("interventionId") for item in interventions]
    if len(ids) != len(set(ids)):
        fail("Intervention IDs must be unique")
    by_id = {item["interventionId"]: item for item in interventions}
    for required_id in (
        "intervention.riparian-buffer-restoration",
        "intervention.wildlife-corridor",
        "intervention.road-fragmentation",
        "intervention.wastewater-discharge",
        "intervention.renewable-microgrid",
        "intervention.compute-expansion",
    ):
        if required_id not in by_id:
            fail(f"Missing C2 intervention: {required_id}")

    road = by_id["intervention.road-fragmentation"]
    road_delta = road.get("delta", {})
    if not (road_delta.get("humanWellbeing", 0) > 0 and road_delta.get("biodiversity", 0) < 0):
        fail("Road prototype must encode a real human/ecological trade-off")
    if not any(value < 0 for value in road.get("habitatLinkDeltas", {}).values()):
        fail("Road prototype must fragment at least one habitat link")

    microgrid = by_id["intervention.renewable-microgrid"].get("delta", {})
    if not (microgrid.get("energyReliability", 0) > 0 and microgrid.get("airQuality", 0) > 0 and microgrid.get("materialDemand", 0) > 0):
        fail("Renewable microgrid must include reliability/air benefits and material demand")

    compute = by_id["intervention.compute-expansion"].get("delta", {})
    if compute.get("computeEnergyDemand", 0) <= 0:
        fail("Compute expansion must expose its energy demand")

    boundary = data.get("contentBoundary", {})
    for flag in (
        "speciesProfilesArePrototypeModels",
        "speciesProfilesAreNotConservationClaims",
        "scientificReviewRequiredBeforeProduction",
        "noGenericAnimalState",
        "noAutomaticMoralReward",
    ):
        if boundary.get(flag) is not True:
            fail(f"C2 content boundary missing: {flag}")

    raw = CANONICAL.read_text(encoding="utf-8")
    for forbidden in ("correctOptionId", "correctMoral", "premiumCurrency", "lootBox", "streak"):
        if forbidden in raw:
            fail(f"Forbidden C2 concept found: {forbidden}")

    types_h = read("game/Source/WorldMakers/Environment/WMPlanetaryCommonsTypes.h")
    for token in (
        "FWMPlanetaryCommonsSnapshot",
        "FWMSpeciesNeedDefinition",
        "FWMSpeciesStateSnapshot",
        "FWMPlanetaryCommonsModel",
        "SeedFromLegacyEcosystem",
        "ApplyLegacyProjectionDelta",
        "AdvanceStep",
        "GetHabitatLinkPermeability",
        "HabitatConnectivity",
        "AnimalStress",
    ):
        if token not in types_h:
            fail(f"C2 runtime header missing token: {token}")

    types_cpp = read("game/Source/WorldMakers/Environment/WMPlanetaryCommonsTypes.cpp")
    for hook in c2_hooks:
        if f'TEXT("{hook}")' not in types_cpp:
            fail(f"C2 runtime does not expose hook: {hook}")
    for token in (
        "DeriveHabitatConnectivity",
        "DeriveSpeciesStress",
        "MaterialSoilPressureRate",
        "ComputeReliabilityPressureRate",
        "EcologicalTarget",
        "WellbeingTarget",
    ):
        if token not in types_cpp:
            fail(f"C2 cross-system propagation missing token: {token}")

    subsystem_h = read("game/Source/WorldMakers/Environment/WMPlanetaryCommonsSubsystem.h")
    subsystem_cpp = read("game/Source/WorldMakers/Environment/WMPlanetaryCommonsSubsystem.cpp")
    for token in ("UWMPlanetaryCommonsSubsystem", "OnPlanetaryCommonsChanged", "SyncFromLegacyEcosystem"):
        if token not in subsystem_h:
            fail(f"C2 subsystem header missing token: {token}")
    for token in (
        "InitializeDependency<UWMEnvironmentStateSubsystem>",
        "OnEnvironmentStateChanged.AddDynamic",
        "ApplyLegacyProjectionDelta",
        "planetary-commons-living-world-v1.json",
    ):
        if token not in subsystem_cpp:
            fail(f"C2 legacy/native integration missing token: {token}")

    tests = read("game/Source/WorldMakers/Private/Tests/WMPlanetaryCommonsTests.cpp")
    for test_name in (
        "WorldMakers.Citizenship.C2.Definition.IsSaneAndDerivesHooks",
        "WorldMakers.Citizenship.C2.LegacyM34.ProjectsAcceptedChangesAsDeltas",
        "WorldMakers.Citizenship.C2.Causality.InfrastructureTradeoffPropagates",
        "WorldMakers.Citizenship.C2.LivingWorld.SpeciesRemainDistinct",
        "WorldMakers.Citizenship.C2.LivingWorld.RestorationCanReduceStress",
        "WorldMakers.Citizenship.C2.Determinism.LongRunRemainsBounded",
    ):
        if test_name not in tests:
            fail(f"Missing C2 automation test: {test_name}")

    docs = read("docs/c2-planetary-commons-living-world.md")
    for token in (
        "Every action enters a web of relations",
        "no generic animal",
        "simulation prototypes, not conservation claims",
        "deterministic",
        "civic-legitimacy",
        "Sovereignty of Hospitality",
        "native Unreal",
    ):
        if token not in docs:
            fail(f"C2 documentation missing boundary: {token}")

    repo_quality = read(".github/workflows/repo-quality.yml")
    unreal_ci = read(".github/workflows/unreal-ci.yml")
    command = "python scripts/validate-c2-planetary-commons-living-world.py"
    if command not in repo_quality:
        fail("Repository Quality does not execute C2 gate")
    if command not in unreal_ci:
        fail("Unreal CI project-validation does not execute C2 gate")

    print(
        "C2 Planetary Commons & Living World Simulation passed: 11 C1 hooks, derived habitat/species state, "
        "M3.4 delta projection, deterministic cross-system coupling, explicit trade-offs and scientific-review boundaries are wired."
    )


if __name__ == "__main__":
    main()
