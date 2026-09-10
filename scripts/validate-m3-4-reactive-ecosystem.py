#!/usr/bin/env python3
"""Validate M3.4 reactive ecosystem state and care-action contracts."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CANONICAL = ROOT / "content/biomes/caribbean-rainforest/ecosystem.json"
PACKAGED = ROOT / "game/Content/WorldMakers/Biomes/biome.caribbean-rainforest.ecosystem.json"
SCHEMA = ROOT / "content/biomes/schema/ecosystem-state.schema.json"

REQUIRED_FILES = [
    "game/Source/WorldMakers/Environment/WMEnvironmentStateTypes.h",
    "game/Source/WorldMakers/Environment/WMEnvironmentStateTypes.cpp",
    "game/Source/WorldMakers/Environment/WMEnvironmentStateSubsystem.h",
    "game/Source/WorldMakers/Environment/WMEnvironmentStateSubsystem.cpp",
    "game/Source/WorldMakers/Environment/WMEnvironmentActionActor.h",
    "game/Source/WorldMakers/Environment/WMEnvironmentActionActor.cpp",
    "game/Source/WorldMakers/Environment/WMEnvironmentReactionProxyActor.h",
    "game/Source/WorldMakers/Environment/WMEnvironmentReactionProxyActor.cpp",
    "game/Source/WorldMakers/Private/Tests/WMEnvironmentStateTests.cpp",
    "docs/m3-4-reactive-ecosystem-state.md",
]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def main() -> None:
    for relative in REQUIRED_FILES:
        require((ROOT / relative).is_file(), f"Missing M3.4 file: {relative}")

    require(CANONICAL.is_file() and PACKAGED.is_file() and SCHEMA.is_file(), "M3.4 ecosystem source/schema/package is incomplete")
    require(CANONICAL.read_bytes() == PACKAGED.read_bytes(), "Packaged ecosystem profile must be byte-equivalent to canonical JSON")

    profile = json.loads(CANONICAL.read_text(encoding="utf-8"))
    schema = json.loads(SCHEMA.read_text(encoding="utf-8"))
    require(profile.get("schemaVersion") == 1, "M3.4 ecosystem schemaVersion must be 1")
    require(profile.get("biomeId") == "biome.caribbean-rainforest", "M3.4 biome ID must remain stable")
    require(profile.get("prototypeOnly") is True, "M3.4 remains prototypeOnly before native certification")
    require(schema.get("type") == "object" and "action" in schema.get("$defs", {}), "Ecosystem schema must define state/action contracts")

    baseline = profile.get("baseline", {})
    state_keys = {"vegetationHealth", "waterFlow", "soilProtection", "shadeCoverage"}
    require(set(baseline) == state_keys, "Baseline must contain exactly four primary ecosystem dimensions")
    require("habitatQuality" not in baseline, "Habitat quality must be derived, never authored")
    require(all(isinstance(baseline[k], (int, float)) and 0 <= baseline[k] <= 1 for k in state_keys), "Baseline ecosystem values must be normalized")

    actions = profile.get("actions", [])
    require(len(actions) >= 3, "M3.4 requires at least three explicit care actions")
    action_ids = [item.get("actionId") for item in actions]
    target_ids = [item.get("targetId") for item in actions]
    prompt_keys = [item.get("promptKey") for item in actions]
    require(len(action_ids) == len(set(action_ids)) and all(str(v).startswith("action.") for v in action_ids), "Care action IDs must be unique stable IDs")
    require(len(target_ids) == len(set(target_ids)) and all(str(v).startswith("target.") for v in target_ids), "Care target IDs must be unique stable IDs")
    require(len(prompt_keys) == len(set(prompt_keys)) and all(str(v).startswith("interaction.") for v in prompt_keys), "Care prompt keys must be unique localization IDs")
    for action in actions:
        require(action.get("maxApplications") == 1, f"Prototype care action must be one-shot and non-farmable: {action.get('actionId')}")
        require(100 <= action.get("interactionRadiusCm", 0) <= 1500, f"Invalid care interaction radius: {action.get('actionId')}")
        require(20 <= action.get("focusRadiusCm", 0) <= 400, f"Invalid care focus radius: {action.get('actionId')}")
        delta = action.get("delta", {})
        require(set(delta) == state_keys, f"Care delta must cover all primary dimensions: {action.get('actionId')}")
        require(any(abs(float(delta[k])) > 0 for k in state_keys), f"Care action must mutate ecosystem state: {action.get('actionId')}")
        require(all(-1 <= float(delta[k]) <= 1 for k in state_keys), f"Care deltas must be bounded: {action.get('actionId')}")

    required_actions = {
        "action.ecosystem.protect-soil",
        "action.ecosystem.restore-shade",
        "action.ecosystem.clear-water-path",
    }
    require(required_actions.issubset(set(action_ids)), "M3.4 canonical care actions are incomplete")

    bands = profile.get("reactionBands", [])
    require(len(bands) >= 3, "M3.4 requires stressed/recovering/thriving reaction bands")
    maxima = [float(item.get("maxHabitatQuality", -1)) for item in bands]
    require(maxima == sorted(maxima) and len(maxima) == len(set(maxima)) and maxima[-1] == 1.0, "Reaction bands must be strictly ordered and terminate at 1.0")
    reaction_ids = [item.get("reactionId") for item in bands]
    for expected in ("reaction.ecosystem.stressed", "reaction.ecosystem.recovering", "reaction.ecosystem.thriving"):
        require(expected in reaction_ids, f"Missing ecosystem reaction band: {expected}")

    final = dict(baseline)
    for action in actions:
        for key in state_keys:
            final[key] = min(1.0, max(0.0, final[key] + float(action["delta"][key])))
    final_habitat = sum(final.values()) / 4.0
    require(abs(final_habitat - 0.70) < 1e-6, "Canonical three-action path must deterministically reach habitatQuality 0.70")

    types_h = read("game/Source/WorldMakers/Environment/WMEnvironmentStateTypes.h")
    types_cpp = read("game/Source/WorldMakers/Environment/WMEnvironmentStateTypes.cpp")
    for token in (
        "FWMEnvironmentStateSnapshot",
        "FWMEnvironmentActionDefinition",
        "FWMEnvironmentStateDefinition",
        "FWMEnvironmentStateModel",
        "DeriveHabitatQuality",
        "ReactionId",
        "MaxApplications",
        "ApplicationCounts",
        "FMath::Clamp",
    ):
        require(token in types_h or token in types_cpp, f"Reactive ecosystem model missing: {token}")
    require("HabitatQuality = FWMEnvironmentStateDefinition::DeriveHabitatQuality" in types_cpp, "Habitat quality must be derived from primary state")
    require("ApplicationCounts.FindRef(ActionId) < Action->MaxApplications" in types_cpp, "Care action caps must be enforced by the state model")

    subsystem_h = read("game/Source/WorldMakers/Environment/WMEnvironmentStateSubsystem.h")
    subsystem_cpp = read("game/Source/WorldMakers/Environment/WMEnvironmentStateSubsystem.cpp")
    for token in ("UWorldSubsystem", "ReloadProfileCatalog", "ActivateProfile", "ApplyAction", "EnsureActionTargets", "OnEnvironmentStateChanged", "OnEnvironmentReaction"):
        require(token in subsystem_h or token in subsystem_cpp, f"Environment state subsystem missing: {token}")
    require("*.ecosystem.json" in subsystem_cpp, "Environment state must load packaged data-driven profiles")
    require("SpawnActor<AWMEnvironmentActionActor>" in subsystem_cpp, "Care interaction anchors must be generated from ecosystem profile data")

    interface_h = read("game/Source/WorldMakers/Environment/WMInteractable.h")
    require("GetInteractionActionId" in interface_h and "GetInteractionMode" in interface_h, "Reusable interaction contract must distinguish observation and care actions")

    observation_actor = read("game/Source/WorldMakers/Environment/WMEnvironmentalInteractableActor.cpp")
    action_actor_h = read("game/Source/WorldMakers/Environment/WMEnvironmentActionActor.h")
    action_actor_cpp = read("game/Source/WorldMakers/Environment/WMEnvironmentActionActor.cpp")
    require("UWMEnvironmentStateSubsystem" not in observation_actor, "Observation interactions must not mutate ecosystem state")
    require("public IWMInteractable" in action_actor_h and "GetInteractionActionId" in action_actor_h, "Care-action actor must implement reusable interaction contract")
    require("EnvironmentState->ApplyAction(ActionId)" in action_actor_cpp, "Care action must route through environment-state authority")

    interaction_h = read("game/Source/WorldMakers/Environment/WMInteractionComponent.h")
    interaction_cpp = read("game/Source/WorldMakers/Environment/WMInteractionComponent.cpp")
    for token in ("FocusedInteractionMode", "FocusedActionId", "LastActionId"):
        require(token in interaction_h, f"Interaction read state missing M3.4 token: {token}")
    require("TActorIterator<AWMEnvironmentalInteractableActor>" in interaction_cpp, "M3.2 observation focus path must remain intact")
    require("TActorIterator<AWMEnvironmentActionActor>" in interaction_cpp, "M3.4 care targets must participate in focus acquisition")

    proxy_h = read("game/Source/WorldMakers/Environment/WMEnvironmentReactionProxyActor.h")
    proxy_cpp = read("game/Source/WorldMakers/Environment/WMEnvironmentReactionProxyActor.cpp")
    for token in ("OnReactionVisualChanged", "CurrentReactionId", "OnEnvironmentStateChanged"):
        require(token in proxy_h or token in proxy_cpp, f"Authored visual reaction proxy missing: {token}")

    environment_sources = "\n".join(path.read_text(encoding="utf-8") for path in (ROOT / "game/Source/WorldMakers/Environment").glob("*.*") if path.suffix in {".h", ".cpp"})
    require("Mission/WMMission" not in environment_sources, "Environment domain must remain mission-agnostic")

    hud_h = read("game/Source/WorldMakers/UI/WMBuildHUDWidget.h")
    hud_cpp = read("game/Source/WorldMakers/UI/WMBuildHUDWidget.cpp")
    for token in ("EnvironmentStatusText", "InteractionActionText", "ResolveInteractionActionLabel", "reaction.ecosystem.stressed", "reaction.ecosystem.recovering", "reaction.ecosystem.thriving"):
        require(token in hud_h or token in hud_cpp, f"Visible ecosystem consequence/read state missing: {token}")

    tests = read("game/Source/WorldMakers/Private/Tests/WMEnvironmentStateTests.cpp")
    for test_name in (
        "WorldMakers.Environment.Definition.ParsesReactiveEcosystemProfile",
        "WorldMakers.Environment.State.DeterministicBoundedTransitions",
        "WorldMakers.Environment.State.ReactionBands",
    ):
        require(test_name in tests, f"Missing M3.4 automation test: {test_name}")

    repo_ci = read(".github/workflows/repo-quality.yml")
    unreal_ci = read(".github/workflows/unreal-ci.yml")
    require("validate-m3-4-reactive-ecosystem.py" in repo_ci and "validate-m3-4-reactive-ecosystem.py" in unreal_ci, "M3.4 validator must run in Repository Quality and Unreal CI")

    docs = read("docs/m3-4-reactive-ecosystem-state.md").lower()
    for boundary in ("issue #9", "observation", "care action", "derived", "stable ids", "pii", "mission-agnostic"):
        require(boundary in docs, f"M3.4 documentation boundary missing: {boundary}")

    print(f"M3.4 reactive ecosystem validated: {len(actions)} care actions, {len(bands)} reaction bands, derived habitat state, visible HUD feedback, visual proxy and CI contracts are present.")


if __name__ == "__main__":
    main()
