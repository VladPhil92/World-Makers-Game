#!/usr/bin/env python3
"""M5.3 Science Simulation Core source gate."""

from __future__ import annotations

import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CANONICAL = "content/science/science-simulation-core-v1.json"
PACKAGED = "game/Content/WorldMakers/Science/science-simulation-core-v1.json"


def fail(message: str) -> None:
    raise SystemExit(message)


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def main() -> None:
    required_files = (
        CANONICAL,
        PACKAGED,
        "docs/m5-3-science-simulation-core.md",
        "game/Source/WorldMakers/Science/WMScienceSimulationCore.h",
        "game/Source/WorldMakers/Science/WMScienceSimulationCore.cpp",
        "game/Source/WorldMakers/Science/WMScienceSimulationSubsystem.h",
        "game/Source/WorldMakers/Science/WMScienceSimulationSubsystem.cpp",
        "game/Source/WorldMakers/Private/Tests/WMScienceSimulationTests.cpp",
    )
    missing = [path for path in required_files if not (ROOT / path).exists()]
    if missing:
        fail(f"Missing M5.3 files: {missing}")

    canonical = json.loads(read(CANONICAL))
    packaged = json.loads(read(PACKAGED))
    if canonical != packaged:
        fail("Packaged science catalog must exactly match canonical source")
    if canonical.get("schemaVersion") != 1 or canonical.get("prototypeOnly") is not True:
        fail("M5.3 science catalog must remain schemaVersion=1 and prototypeOnly=true")
    if canonical.get("safetyModel") != "virtual-only-no-real-world-procedure":
        fail("M5.3 chemistry content must explicitly lock the virtual-only safety model")

    serialized = json.dumps(canonical).lower()
    for forbidden in ("procedure", "instructions", "recipe", "do-at-home", "do at home"):
        if forbidden in serialized and forbidden != "procedure":
            fail(f"Science catalog contains procedural child-facing token: {forbidden}")
    # The safety model is intentionally allowed to contain the word 'procedure'; no procedure field is allowed.
    def walk(value: object) -> None:
        if isinstance(value, dict):
            for key, child in value.items():
                if key.lower() in {"procedure", "instructions", "recipe", "steps"}:
                    fail(f"Science catalog must not contain actionable procedure field: {key}")
                walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)
    walk(canonical)

    substances = canonical.get("substances", [])
    if len(substances) < 5:
        fail("M5.3 requires a minimally useful matter catalog")
    substance_by_id: dict[str, dict] = {}
    for item in substances:
        sid = item.get("substanceId")
        if not isinstance(sid, str) or not sid.startswith("substance.") or sid in substance_by_id:
            fail(f"Invalid or duplicate substance id: {sid!r}")
        if not isinstance(item.get("molarMassGPerMol"), (int, float)) or item["molarMassGPerMol"] <= 0:
            fail(f"Invalid molar mass for {sid}")
        if item.get("meltingPointC") >= item.get("boilingPointC"):
            fail(f"Invalid phase transition ordering for {sid}")
        if item.get("solubilityGPer100MlWater", -1) < 0:
            fail(f"Invalid solubility for {sid}")
        substance_by_id[sid] = item

    reactions = canonical.get("reactions", [])
    if not reactions:
        fail("M5.3 requires at least one reaction model")
    reaction_ids: set[str] = set()
    for reaction in reactions:
        rid = reaction.get("reactionId")
        if not isinstance(rid, str) or not rid.startswith("reaction.") or rid in reaction_ids:
            fail(f"Invalid or duplicate reaction id: {rid!r}")
        reaction_ids.add(rid)
        if not str(reaction.get("evidenceEventId", "")).startswith("science.chemistry."):
            fail(f"Reaction {rid} must emit stable chemistry evidence")

        def stoich_mass(side: str) -> float:
            components = reaction.get(side, [])
            if not components:
                fail(f"Reaction {rid} has empty {side}")
            total = 0.0
            seen: set[str] = set()
            for component in components:
                sid = component.get("substanceId")
                coefficient = component.get("coefficient")
                if sid not in substance_by_id or sid in seen:
                    fail(f"Reaction {rid} has unknown/duplicate {side} substance {sid!r}")
                if not isinstance(coefficient, int) or not 1 <= coefficient <= 64:
                    fail(f"Reaction {rid} has invalid coefficient for {sid}")
                seen.add(sid)
                total += substance_by_id[sid]["molarMassGPerMol"] * coefficient
            return total

        reactant_mass = stoich_mass("reactants")
        product_mass = stoich_mass("products")
        if not math.isclose(reactant_mass, product_mass, abs_tol=0.05):
            fail(f"Reaction {rid} is not mass-balanced: {reactant_mass} != {product_mass}")

    plants = canonical.get("plantSpecies", [])
    if not plants:
        fail("M5.3 requires botanical lifecycle configuration")
    for plant in plants:
        sid = plant.get("speciesId")
        if not isinstance(sid, str) or not sid.startswith("plant."):
            fail(f"Invalid plant species id: {sid!r}")
        minimum = plant.get("minTemperatureC")
        optimum = plant.get("optimalTemperatureC")
        maximum = plant.get("maxTemperatureC")
        if not (minimum < optimum < maximum):
            fail(f"Plant {sid} temperature suitability range is invalid")
        if plant.get("germinationHoursAtIdeal", 0) <= 0 or plant.get("baseGrowthUnitsPerHour", 0) <= 0:
            fail(f"Plant {sid} lifecycle rates must be positive")
        if not plant.get("floweringBiomass", 0) < plant.get("fruitingBiomass", 0):
            fail(f"Plant {sid} flowering must precede fruiting")

    core_h = read("game/Source/WorldMakers/Science/WMScienceSimulationCore.h")
    core_cpp = read("game/Source/WorldMakers/Science/WMScienceSimulationCore.cpp")
    for token in (
        "EWMMatterState",
        "ResolveMatterState",
        "DissolveInWater",
        "ExecuteReaction",
        "StepConstantForce",
        "GetKineticEnergyJoules",
        "FWMDirectCurrentCircuit",
        "EWMPlantStage",
        "GerminationProgressHours",
        "ResolveTemperatureSuitability",
        "BuildPlantEnvironmentDelta",
    ):
        if token not in core_h or token not in core_cpp:
            fail(f"Science core missing behavior contract: {token}")

    subsystem_h = read("game/Source/WorldMakers/Science/WMScienceSimulationSubsystem.h")
    subsystem_cpp = read("game/Source/WorldMakers/Science/WMScienceSimulationSubsystem.cpp")
    for token in ("UWMScienceSimulationSubsystem", "ReloadScienceCatalog", "science-simulation-core-v1.json"):
        if token not in subsystem_h and token not in subsystem_cpp:
            fail(f"Science runtime loader missing token: {token}")

    tests = read("game/Source/WorldMakers/Private/Tests/WMScienceSimulationTests.cpp")
    for test_name in (
        "WorldMakers.Science.Chemistry.MatterStateAndSaturation",
        "WorldMakers.Science.Chemistry.StoichiometryConservesMass",
        "WorldMakers.Science.Physics.ForceMomentumEnergyAndCircuit",
        "WorldMakers.Science.Botany.GerminationGrowthAndEcologyCoupling",
    ):
        if test_name not in tests:
            fail(f"Missing M5.3 automation test: {test_name}")

    game_ini = read("game/Config/DefaultGame.ini")
    if 'DirectoriesToAlwaysStageAsNonUFS=(Path="WorldMakers/Science")' not in game_ini:
        fail("Packaged builds must stage WorldMakers/Science")

    docs = read("docs/m5-3-science-simulation-core.md")
    for token in (
        "Matter and chemistry",
        "Physics",
        "Biology and botany",
        "Ecology coupling",
        "normalized gameplay proxy",
        "virtual-only",
        "M5.2",
    ):
        if token not in docs:
            fail(f"M5.3 documentation missing contract token: {token}")

    print(
        f"M5.3 Science Simulation Core passed: {len(substances)} substances, {len(reactions)} balanced reaction(s), "
        f"{len(plants)} plant model(s), deterministic physics/circuits, ecology coupling and virtual-only safety boundary are wired."
    )


if __name__ == "__main__":
    main()
