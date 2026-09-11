#!/usr/bin/env python3
"""Validate the World Makers First-Person Interaction Kit v1 source contract."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CANONICAL = ROOT / "content/visual/first-person/first-person-interaction-kit-v1.json"
PACKAGED = ROOT / "game/Content/WorldMakers/Visual/first-person-interaction-kit-v1.json"
RUNTIME_H = ROOT / "game/Source/WorldMakers/Visual/WMFirstPersonInteractionRuntime.h"
RUNTIME_CPP = ROOT / "game/Source/WorldMakers/Visual/WMFirstPersonInteractionRuntime.cpp"
COMPONENT_H = ROOT / "game/Source/WorldMakers/Visual/WMFirstPersonInteractionComponent.h"
COMPONENT_CPP = ROOT / "game/Source/WorldMakers/Visual/WMFirstPersonInteractionComponent.cpp"
PRESENTATION_H = ROOT / "game/Source/WorldMakers/Visual/WMPresentationSubsystem.h"
PRESENTATION_CPP = ROOT / "game/Source/WorldMakers/Visual/WMPresentationSubsystem.cpp"
WIDGET_H = ROOT / "game/Source/WorldMakers/UI/WMFirstPersonContextWidget.h"
WIDGET_CPP = ROOT / "game/Source/WorldMakers/UI/WMFirstPersonContextWidget.cpp"
TESTS = ROOT / "game/Source/WorldMakers/Private/Tests/WMFirstPersonInteractionTests.cpp"
DOC = ROOT / "docs/first-person-interaction-kit-v1.md"
CONFIG = ROOT / "game/Config/DefaultGame.ini"
WORKFLOW = ROOT / ".github/workflows/repo-quality.yml"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit("First-person interaction kit validation failed: " + message)


def main() -> None:
    required = (
        CANONICAL, PACKAGED, RUNTIME_H, RUNTIME_CPP, COMPONENT_H, COMPONENT_CPP,
        PRESENTATION_H, PRESENTATION_CPP, WIDGET_H, WIDGET_CPP, TESTS, DOC, CONFIG, WORKFLOW,
    )
    missing = [str(path.relative_to(ROOT)) for path in required if not path.is_file()]
    require(not missing, f"missing files: {missing}")
    require(CANONICAL.read_bytes() == PACKAGED.read_bytes(), "canonical and staged manifests must be byte-identical")

    data = json.loads(CANONICAL.read_text(encoding="utf-8"))
    require(data.get("schemaVersion") == 1, "schemaVersion must be 1")
    require(data.get("kitId") == "visual.first-person-interaction-kit.v1", "kit identity drifted")
    require(data.get("status") == "source-proxy-ready", "status must remain source-proxy-ready until authored binaries exist")
    require(data.get("presentationOnly") is True and data.get("gameplayAuthority") is False, "kit must remain presentation-only")

    modes = {entry["modeId"]: entry for entry in data.get("modes", [])}
    require(set(modes) == {
        "firstperson.explore", "firstperson.build", "firstperson.scan", "firstperson.measure", "firstperson.observe"
    }, "mode vocabulary drifted")
    require(modes["firstperson.build"]["toolSlot"] == "tool.build", "build tool slot missing")
    require(modes["firstperson.scan"]["toolSlot"] == "tool.scanner", "scanner slot missing")
    require(modes["firstperson.measure"]["toolSlot"] == "tool.measure", "measurement tool slot missing")
    require(modes["firstperson.observe"]["toolSlot"] is None, "observe mode must keep the tool footprint minimal")

    authored_slots = {entry["slotId"]: entry for entry in data.get("authoredAssetSlots", [])}
    expected_slots = {"arms.firstperson", "tool.scanner", "tool.build", "tool.measure", "device.wrist"}
    require(set(authored_slots) == expected_slots, "authored asset slot set is incomplete")
    require(all(entry.get("requiredForFinal") is True for entry in authored_slots.values()), "all authored slots must remain explicit final-production requirements")
    require(all(str(entry.get("assetPath", "")).startswith("/Game/WorldMakers/") for entry in authored_slots.values()), "authored asset paths must live under /Game/WorldMakers")

    actions = [entry.get("actionId") for entry in data.get("animationSlots", [])]
    required_actions = {
        "tool-raise", "tool-lower", "scan-anticipate", "scan-hold", "scan-settle",
        "build-point", "build-confirm", "measure-focus", "observe-focus",
    }
    require(set(actions) == required_actions, "nine-action first-person animation vocabulary drifted")
    require(len(actions) == len(set(actions)), "animation action ids must be unique")

    routing = data.get("semanticEventRouting", {})
    require(routing.get("gameplay.build.*") == "build-confirm", "build semantic route missing")
    require(routing.get("science.*") == "scan-hold", "science semantic route missing")
    require(routing.get("mission.measure.reveal") == "measure-focus", "measure semantic route missing")
    require(routing.get("world.observe.reveal") == "observe-focus", "observe semantic route missing")

    budgets = data.get("runtimeBudgets", {})
    require(int(budgets.get("maxLargePanels", 99)) <= 2, "large panel ceiling exceeds visual policy")
    require(int(budgets.get("maxContextPanels", 99)) == 1, "only one contextual panel is allowed")
    require(float(budgets.get("maxToolScreenFraction", 1.0)) <= 0.22, "tool footprint exceeds visual policy")
    require(float(budgets.get("maxCameraBobCm", 99.0)) <= 1.5, "camera bob ceiling exceeds visual policy")
    require(float(budgets.get("maxToolLagDegrees", 99.0)) <= 4.0, "tool lag ceiling exceeds visual policy")
    require(budgets.get("proxyCollision") is False and budgets.get("proxyCastsShadow") is False, "source proxies must be render-only")
    require(budgets.get("ownerOnlyVisibility") is True, "source proxies must remain owner-only")

    reduced = data.get("reducedMotion", {})
    require(float(reduced.get("cameraBobCm", -1.0)) == 0.0, "Reduced Motion must remove camera bob")
    require(float(reduced.get("toolLagDegrees", -1.0)) == 0.0, "Reduced Motion must remove tool lag")
    require(0.0 < float(reduced.get("poseAmplitudeScale", 0.0)) <= 0.5, "Reduced Motion pose amplitude must be strongly bounded")
    require(reduced.get("preserveInformationState") is True, "Reduced Motion must preserve information state")

    boundary = data.get("certificationBoundary", {})
    require(boundary.get("authoredAssetsPresent") is False, "manifest must not falsely claim authored binaries")
    require(boundary.get("nativeUnrealAssetValidationRequired") is True, "native authored-asset validation boundary missing")
    require(boundary.get("representativeTabletValidationRequired") is True, "representative-device validation boundary missing")

    runtime_text = RUNTIME_H.read_text(encoding="utf-8") + "\n" + RUNTIME_CPP.read_text(encoding="utf-8")
    for token in (
        "EWMFirstPersonInteractionAction", "FWMFirstPersonInteractionPose", "ToolRaise", "ToolLower", "Scan",
        "BuildPoint", "BuildConfirm", "MeasureFocus", "ObserveFocus", "TryParseAction", "DefaultActionForModeId",
    ):
        require(token in runtime_text, f"runtime contract missing: {token}")
    require("ViewModelBobCm = 0.0f" in runtime_text, "Reduced Motion must explicitly zero viewmodel bob")

    component_text = COMPONENT_H.read_text(encoding="utf-8") + "\n" + COMPONENT_CPP.read_text(encoding="utf-8")
    for token in (
        "FirstPersonViewModelRoot", "FirstPersonLeftHandProxy", "FirstPersonRightHandProxy",
        "FirstPersonToolProxy", "FirstPersonWristDeviceProxy", "SetOnlyOwnerSee(true)",
        "ECollisionEnabled::NoCollision", "SetCastShadow(false)", "PulseSemanticEvent", "SetPersistentModeById",
    ):
        require(token in component_text, f"viewmodel/source proxy contract missing: {token}")
    for forbidden in ("AddMovementInput", "SetActorLocation", "TryPlaceCurrentPiece", "RecordComposableEvidence", "GrantReward"):
        require(forbidden not in component_text, f"first-person presentation component acquired gameplay authority: {forbidden}")

    presentation_text = PRESENTATION_H.read_text(encoding="utf-8") + "\n" + PRESENTATION_CPP.read_text(encoding="utf-8")
    for token in (
        "SetFirstPersonInteractionMode", "SetFirstPersonInteractionModeById", "FirstPersonInteractionComponent",
        "ResolveFirstPersonActionForEvent", "gameplay.build.", "mission.measure.reveal", "world.observe.reveal", "science.",
    ):
        require(token in presentation_text, f"presentation bridge missing: {token}")
    require("TargetArmLength, 0.0f" in presentation_text, "first-person camera override must converge to zero spring-arm length")

    widget_text = WIDGET_H.read_text(encoding="utf-8") + "\n" + WIDGET_CPP.read_text(encoding="utf-8")
    for token in ("FirstPersonContextCard", "BUILD", "SCAN", "MEASURE", "OBSERVE", "SetContextAlpha"):
        require(token in widget_text, f"context HUD missing: {token}")

    tests = TESTS.read_text(encoding="utf-8")
    for name in (
        "WorldMakers.Visual.FirstPersonKit.ActionSequenceRemainsBounded",
        "WorldMakers.Visual.FirstPersonKit.ReducedMotionPreservesStateWithoutBob",
        "WorldMakers.Visual.FirstPersonKit.ActionVocabularyMatchesReferenceContract",
        "WorldMakers.Visual.FirstPersonKit.SemanticEventsResolveToContextModes",
    ):
        require(name in tests, f"missing Unreal automation test: {name}")

    doc = DOC.read_text(encoding="utf-8").lower()
    for phrase in (
        "first-person interaction kit", "source proxy", "semantic events", "reduced motion",
        "authored assets", "native unreal", "representative tablet",
    ):
        require(phrase in doc, f"documentation missing: {phrase}")

    config = CONFIG.read_text(encoding="utf-8")
    require('Path="WorldMakers/Visual"' in config, "packaged visual manifest directory is not staged")

    workflow = WORKFLOW.read_text(encoding="utf-8")
    require("python scripts/validate-first-person-interaction-kit.py" in workflow, "Repository Quality must execute first-person kit gate")

    print("First-Person Interaction Kit v1 validated: source viewmodel proxies, contextual HUD, semantic event routing, bounded interaction animation, Reduced Motion and authored-asset handoff are present.")


if __name__ == "__main__":
    main()
