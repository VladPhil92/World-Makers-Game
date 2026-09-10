#!/usr/bin/env python3
"""Validate V6 causal VFX, science adapters, budgets and production handoff."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CANONICAL = ROOT / "content/visual/vfx/vfx-feedback-v6.json"
PACKAGED = ROOT / "game/Content/WorldMakers/Visual/vfx-feedback-v6.json"

EXPECTED_EFFECTS = {
    "gameplay.build.place",
    "gameplay.build.remove",
    "gameplay.build.move",
    "mission.measure.reveal",
    "world.observe.reveal",
    "science.chemistry.dissolution",
    "science.chemistry.saturation",
    "science.chemistry.filtration",
    "science.chemistry.reaction",
    "science.physics.force",
    "science.physics.circuit-flow",
    "science.biology.cell-energy",
    "science.biology.plant-growth",
    "science.ecology.recovery",
    "science.ecology.stress",
    "fantasy.portal.open",
    "fantasy.rune.activate",
}

REQUIRED_FILES = (
    "game/Source/WorldMakers/Visual/WMVFXRuntime.h",
    "game/Source/WorldMakers/Visual/WMVFXRuntime.cpp",
    "game/Source/WorldMakers/Visual/WMProceduralVFXGeometry.h",
    "game/Source/WorldMakers/Visual/WMProceduralVFXGeometry.cpp",
    "game/Source/WorldMakers/Visual/WMProceduralVFXActor.h",
    "game/Source/WorldMakers/Visual/WMProceduralVFXActor.cpp",
    "game/Source/WorldMakers/Visual/WMVFXSubsystem.h",
    "game/Source/WorldMakers/Visual/WMVFXSubsystem.cpp",
    "game/Source/WorldMakers/Science/WMScienceVFXAdapter.h",
    "game/Source/WorldMakers/Science/WMScienceVFXAdapter.cpp",
    "game/Source/WorldMakers/Private/Tests/WMVFXRuntimeTests.cpp",
    "docs/v6-vfx-scientific-fantastic-feedback.md",
)

FORBIDDEN_MANIFEST_KEYS = {
    "reward", "rewardId", "progress", "progression", "evidence", "evidenceEventId",
    "freeText", "childText", "childVoice", "biometric", "procedure", "recipe", "steps",
}


def fail(message: str) -> None:
    raise SystemExit(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def read(path: str) -> str:
    source = ROOT / path
    require(source.is_file(), f"Missing V6 file: {path}")
    return source.read_text(encoding="utf-8")


def require_tokens(path: str, *tokens: str) -> str:
    text = read(path)
    missing = [token for token in tokens if token not in text]
    require(not missing, f"{path} missing V6 contract tokens: {missing}")
    return text


def walk_keys(value):
    if isinstance(value, dict):
        for key, child in value.items():
            yield key
            yield from walk_keys(child)
    elif isinstance(value, list):
        for child in value:
            yield from walk_keys(child)


def main() -> None:
    missing = [path for path in REQUIRED_FILES if not (ROOT / path).is_file()]
    require(not missing, f"Missing V6 files: {missing}")
    require(CANONICAL.is_file() and PACKAGED.is_file(), "V6 manifest source/package is incomplete")
    require(CANONICAL.read_bytes() == PACKAGED.read_bytes(), "Packaged V6 manifest must be byte-equivalent to canonical JSON")

    payload = json.loads(CANONICAL.read_text(encoding="utf-8"))
    require(payload.get("schemaVersion") == 1, "V6 manifest schemaVersion must be 1")
    require(payload.get("runtimeId") == "vfx.feedback.v1", "V6 runtime identity drifted")
    require(payload.get("presentationOnly") is True, "VFX must remain presentation-only")
    require(payload.get("gameplayAuthority") is False, "VFX must not own gameplay authority")
    require(payload.get("evidenceAuthority") is False, "VFX must not own evidence authority")
    require(payload.get("sourceFallback") == "procedural-mesh", "V6 source fallback drifted")
    require(payload.get("authoredNiagaraAssetsPresent") is False, "V6 must not claim Niagara assets that are absent")

    keys = set(walk_keys(payload))
    require(not (FORBIDDEN_MANIFEST_KEYS & keys), f"V6 manifest contains forbidden authority/PII/procedure keys: {sorted(FORBIDDEN_MANIFEST_KEYS & keys)}")

    principles = payload.get("principles", {})
    for key in ("causalFeedbackOnly", "noFakeScienceOutcome", "noRewardAuthority", "noChildAuthoredPayload", "supportsReducedMotion"):
        require(principles.get(key) is True, f"V6 principle must remain true: {key}")
    for key in ("flashRequired", "cameraShakeRequired", "commerceHooks"):
        require(principles.get(key) is False, f"V6 safety boundary must remain false: {key}")

    effects = payload.get("effects", [])
    effect_ids = [item.get("id") for item in effects]
    require(len(effect_ids) == len(set(effect_ids)), "V6 effect IDs must be unique")
    require(set(effect_ids) == EXPECTED_EFFECTS, f"V6 effect vocabulary drifted: {sorted(EXPECTED_EFFECTS ^ set(effect_ids))}")
    for effect in effects:
        require(effect.get("domain") in {"gameplay", "chemistry", "physics", "biology", "ecology", "fantasy"}, f"Invalid V6 domain: {effect.get('id')}")
        require(effect.get("shape") in {"ring", "burst", "directional", "halo"}, f"Invalid V6 shape: {effect.get('id')}")
        require(str(effect.get("niagaraTarget", "")).startswith("/Game/WorldMakers/VFX/NS_WM_"), f"Invalid Niagara handoff target: {effect.get('id')}")

    tiers = payload.get("qualityTiers", {})
    for tier in ("low", "mid", "high"):
        require(tier in tiers, f"Missing V6 quality tier: {tier}")
        for key in ("maxActiveProxyEffects", "maxEventsPerSecond", "maxNiagaraParticlesPerEffect"):
            require(isinstance(tiers[tier].get(key), int) and tiers[tier][key] > 0, f"Invalid V6 tier value: {tier}.{key}")
        for key in ("maxDurationSeconds", "maxIntensity"):
            require(isinstance(tiers[tier].get(key), (int, float)) and tiers[tier][key] > 0, f"Invalid V6 tier value: {tier}.{key}")
    for key in ("maxActiveProxyEffects", "maxEventsPerSecond", "maxDurationSeconds", "maxIntensity", "maxNiagaraParticlesPerEffect"):
        require(tiers["low"][key] <= tiers["mid"][key] <= tiers["high"][key], f"V6 quality tiers must scale monotonically for {key}")

    boundary = payload.get("certificationBoundary", {})
    require(boundary.get("sourceRuntimeCanBeValidatedInCI") is True, "V6 source runtime should be CI-validatable")
    require(boundary.get("proceduralFallbackPresent") is True, "V6 requires a source-visible fallback")
    require(boundary.get("niagaraSystemsAuthored") is False, "V6 must not claim authored Niagara")
    require(boundary.get("nativeUnrealVisualReviewRequired") is True, "V6 must preserve native visual review boundary")
    require(boundary.get("representativeTabletProfilingRequired") is True, "V6 must preserve tablet profiling boundary")

    runtime_h = require_tokens(
        "game/Source/WorldMakers/Visual/WMVFXRuntime.h",
        "EWMVFXDomain",
        "EWMVFXShape",
        "FWMVFXEvent",
        "FWMVFXStyle",
        "FWMVFXBudget",
        "TryAccept",
        "ResolveStyle",
    )
    runtime_cpp = require_tokens(
        "game/Source/WorldMakers/Visual/WMVFXRuntime.cpp",
        "gameplay.build.place",
        "science.chemistry.saturation",
        "science.physics.force",
        "science.biology.cell-energy",
        "science.ecology.recovery",
        "fantasy.portal.open",
        "MaxEventsPerSecond",
        "MaxActiveProxyEffects",
        "0.35f",
    )
    for forbidden_call in ("GrantReward", "RecordComposableEvidence", "CompleteMission", "AddCurrency"):
        require(forbidden_call not in runtime_h + runtime_cpp, f"V6 runtime must not own authority: {forbidden_call}")

    geometry_h = read("game/Source/WorldMakers/Visual/WMProceduralVFXGeometry.h")
    geometry_cpp = require_tokens(
        "game/Source/WorldMakers/Visual/WMProceduralVFXGeometry.cpp",
        "BuildRing",
        "BuildBurst",
        "BuildDirectionalChevron",
        "BuildHalo",
        "AddTri",
    )
    require("/Engine/BasicShapes/" not in geometry_h + geometry_cpp, "V6 procedural geometry must not wrap Engine BasicShapes")

    actor_cpp = require_tokens(
        "game/Source/WorldMakers/Visual/WMProceduralVFXActor.cpp",
        "CreateMeshSection_LinearColor",
        "SetLifeSpan",
        "SetActorScale3D",
        "EWMVFXShape::Directional",
        "Event.MotionScale",
    )
    require("SetActorEnableCollision(false)" in actor_cpp, "VFX fallback must remain collision-free")

    subsystem_h = require_tokens(
        "game/Source/WorldMakers/Visual/WMVFXSubsystem.h",
        "UWMVFXSubsystem",
        "EmitSemanticEvent",
        "SetReducedMotion",
        "HandleBuildWorldChanged",
        "HandleEnvironmentStateChanged",
    )
    subsystem_cpp = require_tokens(
        "game/Source/WorldMakers/Visual/WMVFXSubsystem.cpp",
        "InitializeDependency<UWMBuildWorldStateSubsystem>",
        "InitializeDependency<UWMEnvironmentStateSubsystem>",
        "OnBuildWorldChanged.AddDynamic",
        "OnEnvironmentStateChanged.AddDynamic",
        "FindUnmatched",
        "gameplay.build.place",
        "gameplay.build.remove",
        "gameplay.build.move",
        "science.ecology.recovery",
        "science.ecology.stress",
    )
    for forbidden_call in ("GrantReward", "RecordComposableEvidence", "CompleteMission", "AddCurrency"):
        require(forbidden_call not in subsystem_cpp, f"VFX subsystem must not own authority: {forbidden_call}")

    adapter_h = read("game/Source/WorldMakers/Science/WMScienceVFXAdapter.h")
    adapter_cpp = require_tokens(
        "game/Source/WorldMakers/Science/WMScienceVFXAdapter.cpp",
        "FromDissolution",
        "FromFiltration",
        "FromReaction",
        "FromForce",
        "FromCircuit",
        "FromCell",
        "FromPlant",
        "FromEnvironmentDelta",
        "Result.bAccepted",
        "ReactionExtentMol",
        "ForceNewtons.GetSafeNormal",
        "EnergyProducedUnits",
        "GrowthUnits",
    )
    require("procedure" not in adapter_h.lower() + adapter_cpp.lower(), "Science VFX adapter must not add real-world procedures")

    tests = read("game/Source/WorldMakers/Private/Tests/WMVFXRuntimeTests.cpp")
    for name in (
        "WorldMakers.Visual.VFX.SemanticStylesAreStable",
        "WorldMakers.Visual.VFX.BudgetsAndReducedMotionAreBounded",
        "WorldMakers.Visual.VFX.ProceduralFallbackGeometryIsValid",
        "WorldMakers.Visual.VFX.ScienceResultsDriveSemanticsAndMagnitude",
        "WorldMakers.Visual.VFX.BiologyAndEcologyFeedbackRemainCausal",
    ):
        require(name in tests, f"Missing V6 automation test: {name}")
    require(tests.count("IMPLEMENT_SIMPLE_AUTOMATION_TEST") >= 5, "V6 requires at least five automation tests")

    docs = require_tokens(
        "docs/v6-vfx-scientific-fantastic-feedback.md",
        "Visual effects explain causality; they do not invent it.",
        "17",
        "SetReducedMotion(true)",
        "authoredNiagaraAssetsPresent=false",
        "does not claim",
        "V7 — Camera, Cinematics & UI Motion",
    )
    require("source-complete" in docs.lower(), "V6 docs must state source-complete boundary")

    roadmap = read("docs/visual-production-roadmap.md")
    require("V6 status: source-complete causal VFX pass" in roadmap, "Visual roadmap must mark V6 source status explicitly")
    require("## V7 — Camera, Cinematics & UI Motion" in roadmap, "Visual roadmap must preserve V7 next phase")

    game_config = read("game/Config/DefaultGame.ini")
    require('+DirectoriesToAlwaysStageAsNonUFS=(Path="WorldMakers/Visual")' in game_config, "V6 manifest requires WorldMakers/Visual staging")

    workflow = read(".github/workflows/repo-quality.yml")
    require("python scripts/validate-v6-vfx-feedback.py" in workflow, "Repository Quality must execute V6 gate")

    print("V6 VFX feedback passed: 17 causal semantics, bounded procedural fallback, build/ecology subscriptions, science adapters, reduced-motion behavior and Niagara handoff are wired.")


if __name__ == "__main__":
    main()
