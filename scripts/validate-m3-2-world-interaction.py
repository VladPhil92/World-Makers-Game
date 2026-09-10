#!/usr/bin/env python3
"""Validate M3.2 deliberate world interaction and discovery contracts."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CANONICAL = ROOT / "content/biomes/caribbean-rainforest/runtime.json"
PACKAGED = ROOT / "game/Content/WorldMakers/Biomes/biome.caribbean-rainforest.json"
SCHEMA = ROOT / "content/biomes/schema/biome-runtime.schema.json"

REQUIRED_FILES = [
    "game/Source/WorldMakers/Environment/WMInteractable.h",
    "game/Source/WorldMakers/Environment/WMEnvironmentalInteractableActor.h",
    "game/Source/WorldMakers/Environment/WMEnvironmentalInteractableActor.cpp",
    "game/Source/WorldMakers/Environment/WMInteractionComponent.h",
    "game/Source/WorldMakers/Environment/WMInteractionComponent.cpp",
    "game/Source/WorldMakers/Private/Tests/WMInteractionTests.cpp",
    "docs/m3-2-world-interaction.md",
]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def main() -> None:
    for relative in REQUIRED_FILES:
        require((ROOT / relative).is_file(), f"Missing M3.2 file: {relative}")

    require(CANONICAL.is_file() and PACKAGED.is_file() and SCHEMA.is_file(), "M3.2 biome runtime/schema/package is incomplete")
    require(CANONICAL.read_bytes() == PACKAGED.read_bytes(), "Packaged biome runtime must remain byte-equivalent to canonical JSON")

    payload = json.loads(CANONICAL.read_text(encoding="utf-8"))
    schema = json.loads(SCHEMA.read_text(encoding="utf-8"))
    points = payload.get("pointsOfInterest", [])
    require(len(points) >= 3, "M3.2 requires at least three deliberate Caribbean Rainforest POIs")

    observation_ids: list[str] = []
    prompt_keys: list[str] = []
    for point in points:
        require(point.get("requiresInteraction") is True, f"M3.2 POI must require deliberate interaction: {point.get('id')}")
        require(point.get("interactionMode") in {"observe", "inspect"}, f"Unsupported interaction mode for {point.get('id')}")
        require(100 <= point.get("interactionRadiusCm", 0) <= 1500, f"Invalid interaction radius for {point.get('id')}")
        require(20 <= point.get("focusRadiusCm", 0) <= 400, f"Invalid focus radius for {point.get('id')}")
        prompt_key = point.get("promptKey", "")
        observation_id = point.get("observationId", "")
        require(prompt_key.startswith("interaction."), f"POI prompt must be a stable localization key: {point.get('id')}")
        require(observation_id.startswith("observation."), f"POI observation must use a stable ID: {point.get('id')}")
        prompt_keys.append(prompt_key)
        observation_ids.append(observation_id)

    require(len(observation_ids) == len(set(observation_ids)), "Observation IDs must be unique")
    require(len(prompt_keys) == len(set(prompt_keys)), "Interaction prompt keys must be unique")

    poi_properties = schema["$defs"]["pointOfInterest"]["properties"]
    for field in (
        "requiresInteraction",
        "interactionMode",
        "interactionRadiusCm",
        "focusRadiusCm",
        "promptKey",
        "observationId",
    ):
        require(field in poi_properties, f"Biome schema missing M3.2 field: {field}")

    types_h = read("game/Source/WorldMakers/Environment/WMBiomeTypes.h")
    types_cpp = read("game/Source/WorldMakers/Environment/WMBiomeTypes.cpp")
    for token in (
        "bRequiresInteraction",
        "InteractionMode",
        "InteractionRadiusCm",
        "FocusRadiusCm",
        "PromptKey",
        "ObservationId",
        "IsWithinInteractionRange",
        "FindPointOfInterest",
        "RegisterObservation",
        "HasObserved",
        "ObservedIds",
    ):
        require(token in types_h or token in types_cpp, f"M3.2 semantic interaction model missing: {token}")
    require("ObservedIds.Contains" in types_cpp, "Observation registration must be idempotent")

    interface_h = read("game/Source/WorldMakers/Environment/WMInteractable.h")
    for token in ("UINTERFACE", "IWMInteractable", "GetInteractionPointId", "GetInteractionPromptKey", "GetInteractionObservationId", "CanInteract", "Interact"):
        require(token in interface_h, f"Reusable interaction contract missing: {token}")

    actor_h = read("game/Source/WorldMakers/Environment/WMEnvironmentalInteractableActor.h")
    actor_cpp = read("game/Source/WorldMakers/Environment/WMEnvironmentalInteractableActor.cpp")
    require("public IWMInteractable" in actor_h, "Environmental interaction actor must implement IWMInteractable")
    require("Configure(const FWMPointOfInterestDefinition&" in actor_h, "Environmental interaction anchors must be configured from biome data")
    require("RegisterDeliberateInteraction" in actor_cpp, "Interaction actor must route accepted intent through biome authority")

    subsystem_h = read("game/Source/WorldMakers/Environment/WMBiomeRuntimeSubsystem.h")
    subsystem_cpp = read("game/Source/WorldMakers/Environment/WMBiomeRuntimeSubsystem.cpp")
    for token in ("EnsureInteractionTargets", "RegisterDeliberateInteraction", "OnObservationRegistered", "GetObservedIds", "HasObserved"):
        require(token in subsystem_h or token in subsystem_cpp, f"Biome interaction authority missing: {token}")
    require("if (Point.bRequiresInteraction)" in subsystem_cpp and "continue;" in subsystem_cpp, "Passive location observation must skip deliberate POIs")
    require("IsWithinInteractionRange" in subsystem_cpp, "Biome authority must validate interaction distance")
    require("SpawnActor<AWMEnvironmentalInteractableActor>" in subsystem_cpp, "Interaction anchors must be generated from biome data")

    component_h = read("game/Source/WorldMakers/Environment/WMInteractionComponent.h")
    component_cpp = read("game/Source/WorldMakers/Environment/WMInteractionComponent.cpp")
    for token in ("RefreshFocus", "TryInteractFocused", "IsFocusCandidate", "MinimumFocusDot", "MinimumInteractionIntervalSeconds", "FocusedPointId", "FocusedObservationId"):
        require(token in component_h or token in component_cpp, f"Player interaction component missing: {token}")
    require("TActorIterator<AWMEnvironmentalInteractableActor>" in component_cpp, "Focus acquisition must search semantic interaction targets")
    require("FVector::DotProduct" in component_cpp, "Focus acquisition must require deliberate aim")
    require("LastInteractionTimeSeconds" in component_cpp, "Interaction must include a bounded anti-spam interval")

    decoupled_sources = interface_h + actor_h + actor_cpp + component_h + component_cpp + subsystem_h + subsystem_cpp
    require("WMMissionRuntimeSubsystem" not in decoupled_sources, "M3.2 interaction core must not import mission-specific runtime")

    player_h = read("game/Source/WorldMakers/Player/WMPlayerCharacter.h")
    player_cpp = read("game/Source/WorldMakers/Player/WMPlayerCharacter.cpp")
    require("UWMInteractionComponent" in player_h, "Player must expose interaction component")
    require("CreateDefaultSubobject<UWMInteractionComponent>" in player_cpp, "Player must create interaction component")
    require('BindAction(TEXT("ObserveWorld")' in player_cpp, "Explicit ObserveWorld input action must be bound")

    hud_h = read("game/Source/WorldMakers/UI/WMBuildHUDWidget.h")
    hud_cpp = read("game/Source/WorldMakers/UI/WMBuildHUDWidget.cpp")
    for token in ("BindInteractionComponent", "ObserveWorldButton", "HandleObserve", "InteractionStatusText"):
        require(token in hud_h or token in hud_cpp, f"Child-facing interaction HUD missing: {token}")
    require("LOCTEXT" in hud_cpp and "FocusedPromptKey" in hud_cpp, "Interaction prompts must resolve through localization-ready stable keys")

    default_input = read("game/Config/DefaultInput.ini")
    require('ActionName="ObserveWorld",Key=G' in default_input, "Keyboard ObserveWorld mapping missing")
    require('ActionName="ObserveWorld",Key=Gamepad_FaceButton_Left' in default_input, "Gamepad ObserveWorld mapping missing")

    tests = read("game/Source/WorldMakers/Private/Tests/WMInteractionTests.cpp")
    for test_name in (
        "WorldMakers.Interaction.Definition.DeliberateMetadata",
        "WorldMakers.Interaction.Focus.RequiresAimAndRange",
        "WorldMakers.Interaction.Observation.IdempotentStableIds",
    ):
        require(test_name in tests, f"Missing M3.2 automation test: {test_name}")

    repo_ci = read(".github/workflows/repo-quality.yml")
    unreal_ci = read(".github/workflows/unreal-ci.yml")
    require("validate-m3-2-world-interaction.py" in repo_ci and "validate-m3-2-world-interaction.py" in unreal_ci, "M3.2 validator must run in Repository Quality and Unreal CI")

    docs = read("docs/m3-2-world-interaction.md").lower()
    for boundary in ("issue #9", "explicit intent", "stable ids", "pii", "mission", "proximity"):
        require(boundary in docs, f"M3.2 documentation boundary missing: {boundary}")

    print(f"M3.2 world interaction validated: {len(points)} deliberate POIs, focus/range authority, stable observations, child HUD and CI contracts are present.")


if __name__ == "__main__":
    main()
