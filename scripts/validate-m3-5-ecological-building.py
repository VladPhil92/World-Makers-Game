#!/usr/bin/env python3
"""Validate M3.5 ecological building interventions and creative unlock contracts."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CANONICAL = ROOT / "content/biomes/caribbean-rainforest/build-interventions.json"
PACKAGED = ROOT / "game/Content/WorldMakers/Biomes/biome.caribbean-rainforest.build-interventions.json"
SCHEMA = ROOT / "content/biomes/schema/ecological-build-interventions.schema.json"

REQUIRED_FILES = (
    "docs/m3-5-ecological-building.md",
    "game/Source/WorldMakers/Building/WMBuildWorldStateSubsystem.h",
    "game/Source/WorldMakers/Building/WMBuildWorldStateSubsystem.cpp",
    "game/Source/WorldMakers/Building/WMBuildUnlockSubsystem.h",
    "game/Source/WorldMakers/Building/WMBuildUnlockSubsystem.cpp",
    "game/Source/WorldMakers/Environment/WMEcologicalBuildTypes.h",
    "game/Source/WorldMakers/Environment/WMEcologicalBuildTypes.cpp",
    "game/Source/WorldMakers/Environment/WMEcologicalBuildSubsystem.h",
    "game/Source/WorldMakers/Environment/WMEcologicalBuildSubsystem.cpp",
    "game/Source/WorldMakers/Private/Tests/WMEcologicalBuildTests.cpp",
)

INTERVENTION_IDS = [
    "intervention.ecosystem.soil-buffer",
    "intervention.ecosystem.shade-shelter",
    "intervention.ecosystem.habitat-garden",
]
REWARD_TO_PIECE = {
    "reward.eco.leaf-roof-unlock": "eco.leaf-roof",
    "reward.eco.rainforest-planter-unlock": "eco.rainforest-planter",
    "reward.eco.bamboo-bridge-unlock": "eco.bamboo-bridge",
}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def main() -> None:
    for path in REQUIRED_FILES:
        require((ROOT / path).is_file(), f"Missing M3.5 file: {path}")
    require(CANONICAL.is_file() and PACKAGED.is_file() and SCHEMA.is_file(), "M3.5 canonical/package/schema files are incomplete")
    require(CANONICAL.read_bytes() == PACKAGED.read_bytes(), "Packaged ecological build profile must be byte-equivalent to canonical source")

    payload = json.loads(CANONICAL.read_text(encoding="utf-8"))
    schema = json.loads(SCHEMA.read_text(encoding="utf-8"))
    require(payload.get("schemaVersion") == 1, "M3.5 ecological build schemaVersion must remain 1")
    require(payload.get("biomeId") == "biome.caribbean-rainforest", "M3.5 must target the Caribbean Rainforest biome")
    interventions = payload.get("interventions", [])
    require([item.get("interventionId") for item in interventions] == INTERVENTION_IDS, "M3.5 intervention sequence changed unexpectedly")

    expected_prerequisites = [[], [INTERVENTION_IDS[0]], [INTERVENTION_IDS[1]]]
    expected_rewards = list(REWARD_TO_PIECE)
    expected_requirements = [
        {"prototype.wall": 2},
        {"prototype.pillar": 2, "eco.leaf-roof": 1},
        {"eco.rainforest-planter": 2, "prototype.floor": 1},
    ]
    for index, intervention in enumerate(interventions):
        require(intervention.get("prerequisiteInterventionIds") == expected_prerequisites[index], f"Unexpected prerequisites for {INTERVENTION_IDS[index]}")
        require(intervention.get("rewardId") == expected_rewards[index], f"Unexpected creative reward for {INTERVENTION_IDS[index]}")
        requirements = {item.get("pieceId"): item.get("minCount") for item in intervention.get("requirements", [])}
        require(requirements == expected_requirements[index], f"Unexpected piece requirements for {INTERVENTION_IDS[index]}")
        require(100 <= intervention.get("radiusCm", 0) <= 1500, f"Invalid intervention radius for {INTERVENTION_IDS[index]}")
        delta = intervention.get("effectDelta", {})
        values = [delta.get(field) for field in ("vegetationHealth", "waterFlow", "soilProtection", "shadeCoverage")]
        require(all(isinstance(value, (int, float)) and -1 <= value <= 1 for value in values), f"Unbounded ecological delta for {INTERVENTION_IDS[index]}")
        require(any(abs(value) > 0 for value in values), f"Intervention {INTERVENTION_IDS[index]} must have a causal effect")

    schema_text = json.dumps(schema)
    for token in ("interventionId", "prerequisiteInterventionIds", "effectDelta", "rewardId", "minCount"):
        require(token in schema_text, f"M3.5 schema missing {token}")

    catalog_h = read("game/Source/WorldMakers/Building/WMBuildCatalogSettings.h")
    catalog_cpp = read("game/Source/WorldMakers/Building/WMBuildCatalogSettings.cpp")
    game_ini = read("game/Config/DefaultGame.ini")
    require("RequiredRewardId" in catalog_h and "RequiredRewardId.ToString().StartsWith" in catalog_cpp, "Build catalog must validate optional reward gates")
    for reward_id, piece_id in REWARD_TO_PIECE.items():
        require(piece_id in game_ini and reward_id in game_ini, f"Build catalog missing gated creative piece {piece_id}")

    rewards = json.loads(read("content/economy/rewards/default-rewards.json"))
    reward_by_id = {item.get("rewardId"): item for item in rewards}
    for reward_id, piece_id in REWARD_TO_PIECE.items():
        reward = reward_by_id.get(reward_id)
        require(reward is not None, f"Missing M3.5 Trust Economy reward {reward_id}")
        require(reward.get("type") == "creative_unlock", f"{reward_id} must be a creative_unlock")
        require(reward.get("payload", {}).get("unlockId") == piece_id, f"{reward_id} must unlock {piece_id}")
        require(reward.get("trigger", {}).get("event") == "environment.intervention-completed", f"{reward_id} must be intervention-driven")
        require(reward.get("deterministic") is True, f"{reward_id} must be deterministic")
        require(reward.get("purchasable") is False, f"{reward_id} must not be purchasable")
        require(reward.get("transferable") is False, f"{reward_id} must not be transferable")
        require(reward.get("convertibleToMoney") is False, f"{reward_id} must not be convertible")
        require(reward.get("expiresAt") is None and reward.get("streakRequired") is False, f"{reward_id} must not expire or require a streak")

    projection_h = read("game/Source/WorldMakers/Building/WMBuildWorldStateSubsystem.h")
    projection_cpp = read("game/Source/WorldMakers/Building/WMBuildWorldStateSubsystem.cpp")
    for token in ("FWMPlacedBuildPieceSnapshot", "PieceId", "LocationCm", "YawDegrees", "OnBuildWorldChanged", "PublishSnapshot"):
        require(token in projection_h or token in projection_cpp, f"Neutral Build world projection missing {token}")

    building_cpp = read("game/Source/WorldMakers/Building/WMBuildingComponent.cpp")
    building_h = read("game/Source/WorldMakers/Building/WMBuildingComponent.h")
    require("PublishBuildWorldSnapshot" in building_h and "BuildState->PublishSnapshot" in building_cpp, "Building mutations must publish a neutral world projection")
    require("WMBuildUnlockSubsystem" in building_cpp and "RequiredRewardId" in building_cpp and "IsRewardGranted" in building_cpp, "Building must enforce reward-gated pieces")
    require("PieceIds.RemoveAll" in building_cpp and "ResolvePieceSpec(PieceId" in building_cpp, "Piece cycling must hide locked pieces")
    require("ResolvePieceSpec(Record.PieceId" in building_cpp, "World load validation must enforce piece unlocks")
    require("Environment/" not in building_cpp and "WMEnvironmentStateSubsystem" not in building_cpp, "Building Core must not import Environment rules")

    unlock_h = read("game/Source/WorldMakers/Building/WMBuildUnlockSubsystem.h")
    unlock_cpp = read("game/Source/WorldMakers/Building/WMBuildUnlockSubsystem.cpp")
    for token in ("FWMBuildUnlockModel", "RequiredRewardId", "EnsureRewardGranted", "IsPieceUnlocked", "WM_CreativeUnlocks_Prototype"):
        require(token in unlock_h or token in unlock_cpp, f"Creative unlock subsystem missing {token}")
    save_block = unlock_h.split("class WORLDMAKERS_API UWMBuildUnlockSaveGame", 1)[1].split("UCLASS", 1)[0]
    require("TArray<FName> GrantedRewardIds" in save_block, "Creative unlock save must persist stable reward IDs")
    require("FString" not in save_block and "FText" not in save_block, "Creative unlock persistence must not contain free text")

    env_types_h = read("game/Source/WorldMakers/Environment/WMEnvironmentStateTypes.h")
    env_types_cpp = read("game/Source/WorldMakers/Environment/WMEnvironmentStateTypes.cpp")
    env_subsystem_h = read("game/Source/WorldMakers/Environment/WMEnvironmentStateSubsystem.h")
    for token in ("FWMEnvironmentStateDelta", "CanApplyTrustedEffect", "ApplyTrustedEffect", "ApplyDelta"):
        require(token in env_types_h or token in env_types_cpp or token in env_subsystem_h, f"Bounded trusted effect path missing {token}")
    require("UFUNCTION" not in env_subsystem_h.split("bool ApplyTrustedEffect", 1)[0].rsplit("\n", 2)[-2], "Trusted effect entrypoint must not be Blueprint callable")

    eco_h = read("game/Source/WorldMakers/Environment/WMEcologicalBuildSubsystem.h")
    eco_cpp = read("game/Source/WorldMakers/Environment/WMEcologicalBuildSubsystem.cpp")
    eco_types = read("game/Source/WorldMakers/Environment/WMEcologicalBuildTypes.cpp")
    for token in ("UWMBuildWorldStateSubsystem", "OnBuildWorldChanged", "ApplyTrustedEffect", "EnsureRewardGranted", "GetInterventionReadModel"):
        require(token in eco_h or token in eco_cpp, f"Ecological building runtime missing {token}")
    require("HorizontalDistanceSquared" in eco_types and "RadiusSquared" in eco_types, "Intervention satisfaction must be spatially bounded")
    require("CompletedInterventionIds.Contains" in eco_types, "Intervention completion must be idempotent")
    require("WMMissionRuntimeSubsystem" not in (eco_h + eco_cpp + eco_types), "Ecological building runtime must remain mission-agnostic")

    tests = read("game/Source/WorldMakers/Private/Tests/WMEcologicalBuildTests.cpp")
    for test_name in (
        "WorldMakers.Environment.Building.Definition.ParsesInterventionSequence",
        "WorldMakers.Environment.Building.Evaluation.SpatialAndSequential",
        "WorldMakers.Building.Unlocks.RewardGateIsIdempotent",
        "WorldMakers.Environment.Building.Effect.BoundedOneShot",
    ):
        require(test_name in tests, f"Missing M3.5 automation test {test_name}")

    repo_ci = read(".github/workflows/repo-quality.yml")
    unreal_ci = read(".github/workflows/unreal-ci.yml")
    require("validate-m3-5-ecological-building.py" in repo_ci and "validate-m3-5-ecological-building.py" in unreal_ci, "M3.5 validator must run in both CI workflows")

    docs = read("docs/m3-5-ecological-building.md").lower()
    for boundary in ("issue #9", "no coins", "save/load bypass", "stable reward ids", "m3.6"):
        require(boundary in docs, f"M3.5 documentation boundary missing: {boundary}")

    print("M3.5 ecological building validated: spatial interventions, bounded ecosystem effects, persistent creative unlocks and save/load gates are present.")


if __name__ == "__main__":
    main()
