#!/usr/bin/env python3
"""Validate V5 character/interaction animation runtime and production handoff."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CANONICAL = ROOT / "content/visual/character/character-animation-v5.json"
PACKAGED = ROOT / "game/Content/WorldMakers/Visual/character-animation-v5.json"

EXPECTED_STATES = [
    "locomotion.idle",
    "locomotion.start",
    "locomotion.walk",
    "locomotion.run",
    "locomotion.stop",
    "locomotion.turn-in-place",
    "locomotion.jump",
    "locomotion.fall",
    "locomotion.land",
]
EXPECTED_ACTIONS = {
    "interaction.build-place",
    "interaction.build-remove",
    "interaction.build-move",
    "interaction.measure",
    "interaction.observe",
    "interaction.inspect",
    "interaction.pickup",
    "interaction.science-manipulate",
}

REQUIRED_FILES = (
    "game/Source/WorldMakers/Visual/WMCharacterAnimationRuntime.h",
    "game/Source/WorldMakers/Visual/WMCharacterAnimationRuntime.cpp",
    "game/Source/WorldMakers/Visual/WMCharacterAnimationComponent.h",
    "game/Source/WorldMakers/Visual/WMCharacterAnimationComponent.cpp",
    "game/Source/WorldMakers/Private/Tests/WMCharacterAnimationRuntimeTests.cpp",
    "game/Source/WorldMakers/Player/WMPlayerCharacter.h",
    "game/Source/WorldMakers/Player/WMPlayerCharacter.cpp",
    "game/Config/DefaultInput.ini",
    "docs/v5-character-interaction-animation.md",
)


def fail(message: str) -> None:
    raise SystemExit(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def read(path: str) -> str:
    source = ROOT / path
    require(source.is_file(), f"Missing V5 file: {path}")
    return source.read_text(encoding="utf-8")


def require_tokens(path: str, *tokens: str) -> str:
    text = read(path)
    missing = [token for token in tokens if token not in text]
    require(not missing, f"{path} missing V5 contract tokens: {missing}")
    return text


def main() -> None:
    missing = [path for path in REQUIRED_FILES if not (ROOT / path).is_file()]
    require(not missing, f"Missing V5 files: {missing}")
    require(CANONICAL.is_file() and PACKAGED.is_file(), "V5 animation manifest source/package is incomplete")
    require(CANONICAL.read_bytes() == PACKAGED.read_bytes(), "Packaged V5 animation manifest must be byte-equivalent to canonical JSON")

    payload = json.loads(CANONICAL.read_text(encoding="utf-8"))
    require(payload.get("schemaVersion") == 1, "V5 animation manifest schemaVersion must be 1")
    require(payload.get("runtimeId") == "animation.character.v1", "V5 runtime identity drifted")
    require(payload.get("profile") == "ChildExplorerV1", "V5 must target the V4 ChildExplorerV1 rig")
    require(payload.get("movementAuthority") == "CharacterMovementComponent", "Animation must not own gameplay movement")
    require(payload.get("rootMotionDefault") is False, "Root motion must remain disabled by default")
    require(payload.get("locomotionStates") == EXPECTED_STATES, "V5 locomotion state order/identity drifted")

    actions = payload.get("interactionActions", [])
    action_ids = {item.get("id") for item in actions}
    require(action_ids == EXPECTED_ACTIONS, f"V5 interaction action set drifted: {sorted(EXPECTED_ACTIONS ^ action_ids)}")
    for action in actions:
        duration = action.get("defaultDurationSeconds")
        require(isinstance(duration, (int, float)) and 0.18 <= float(duration) <= 1.25, f"Invalid V5 interaction duration: {action.get('id')}")
        require(str(action.get("layer", "")).startswith("upper-body"), f"Interaction must remain an upper-body layer: {action.get('id')}")

    layering = payload.get("layering", {})
    require(layering.get("interactionDoesNotOwnMovement") is True, "Interaction animation must not own movement")
    require(layering.get("locomotionContinuesDuringUpperBodyActions") is True, "Upper-body actions must preserve locomotion")

    source_pose = payload.get("proceduralSourcePose", {})
    require(source_pose.get("enabled") is True, "V5 must keep a source-visible procedural pose until authored clips exist")
    require(source_pose.get("poseBoundsRequired") is True, "V5 source poses must remain hard-bounded")
    driven = set(source_pose.get("drivenJoints", []))
    for joint in ("root", "pelvis", "chest", "head", "upperarm_l", "lowerarm_r", "thigh_l", "calf_r", "foot_l"):
        require(joint in driven, f"V5 source pose is missing representative driven joint: {joint}")

    read_model = set(payload.get("animBPReadModel", []))
    for field in ("locomotionStateId", "speedAlpha", "isAirborne", "stateAgeSeconds", "aimYawDegrees", "aimPitchDegrees", "interactionActionId", "interactionAlpha"):
        require(field in read_model, f"AnimBP read-model missing field: {field}")

    targets = payload.get("authoredTargets", {})
    require(targets.get("animationBlueprint") == "/Game/WorldMakers/Characters/Player/ABP_WM_ChildExplorer", "V5 AnimBP target drifted")
    for key, value in targets.items():
        require(str(value).startswith("/Game/WorldMakers/Characters/Player/"), f"Authored animation target outside player namespace: {key}")

    tiers = payload.get("qualityTiers", {})
    for tier in ("low", "mid", "high"):
        require(tier in tiers, f"Missing V5 quality tier: {tier}")
    require(tiers["low"]["targetPoseHz"] <= tiers["mid"]["targetPoseHz"] <= tiers["high"]["targetPoseHz"], "V5 pose-rate tiers must scale monotonically")
    require(tiers["low"]["maxConcurrentAdditiveLayers"] <= tiers["mid"]["maxConcurrentAdditiveLayers"] <= tiers["high"]["maxConcurrentAdditiveLayers"], "V5 layer budgets must scale monotonically")

    safety = payload.get("accessibilityAndSafety", {})
    require(safety.get("supportsReducedMotion") is True, "V5 production contract must support reduced motion")
    for flag in ("cameraShakeRequired", "flashingFeedbackRequired", "manipulativeCelebrationLoops", "commerceAnimationHooks", "childBiometricMotionCaptureRequired"):
        require(safety.get(flag) is False, f"V5 safety boundary must keep {flag}=false")

    boundary = payload.get("certificationBoundary", {})
    require(boundary.get("sourceRuntimeCanBeValidatedInCI") is True, "V5 source runtime should be CI-validatable")
    require(boundary.get("authoredAnimationAssetsPresent") is False, "V5 must not claim authored animation assets that are not present")
    require(boundary.get("nativeUnrealVisualReviewRequired") is True, "V5 must keep native visual review as a certification boundary")

    runtime_h = require_tokens(
        "game/Source/WorldMakers/Visual/WMCharacterAnimationRuntime.h",
        "EWMLocomotionState",
        "EWMCharacterInteractionAction",
        "FWMCharacterAnimationInput",
        "FWMCharacterAnimationPose",
        "FWMCharacterAnimationRuntime",
        "TriggerInteraction",
        "TryParseInteractionAction",
    )
    runtime_cpp = require_tokens(
        "game/Source/WorldMakers/Visual/WMCharacterAnimationRuntime.cpp",
        "EWMLocomotionState::Start",
        "EWMLocomotionState::Stop",
        "EWMLocomotionState::TurnInPlace",
        "EWMLocomotionState::Jump",
        "EWMLocomotionState::Fall",
        "EWMLocomotionState::Land",
        "InteractionAlpha",
        "FMath::FInterpTo",
        "FWMCharacterAnimationPose::IsBounded",
    )
    require("RootMotion" not in runtime_h + runtime_cpp, "Pure V5 runtime must not implement root-motion gameplay authority")

    component_h = require_tokens(
        "game/Source/WorldMakers/Visual/WMCharacterAnimationComponent.h",
        "UWMCharacterAnimationComponent",
        "TriggerActionById",
        "GetLocomotionStateId",
        "GetInteractionActionId",
        "GetAimYawDegrees",
        "GetAimPitchDegrees",
        "IsProductionAnimBPExpected",
    )
    component_cpp = require_tokens(
        "game/Source/WorldMakers/Visual/WMCharacterAnimationComponent.cpp",
        "CharacterMovement",
        "IsFalling",
        "GetControlRotation",
        "ApplyProceduralPose",
        "AvatarLowerArmLeftRig",
        "AvatarCalfRightRig",
        "GetSkeletalMeshAsset",
    )

    player_h = read("game/Source/WorldMakers/Player/WMPlayerCharacter.h")
    player_cpp = read("game/Source/WorldMakers/Player/WMPlayerCharacter.cpp")
    require("CharacterAnimationComponent" in player_h, "Player must expose the V5 animation bridge")
    for token in (
        "CreateDefaultSubobject<UWMCharacterAnimationComponent>",
        "EWMCharacterInteractionAction::BuildPlace",
        "EWMCharacterInteractionAction::BuildRemove",
        "EWMCharacterInteractionAction::BuildMove",
        "EWMCharacterInteractionAction::Measure",
        "EWMCharacterInteractionAction::Observe",
        "FWMPrototypeMotionStyle::Evaluate",
    ):
        require(token in player_cpp, f"Player V5 integration missing: {token}")
    require("TryPlaceCurrentPiece() && CharacterAnimationComponent" in player_cpp, "Build-place animation must require gameplay success")
    require("TryRemoveTargetPiece() && CharacterAnimationComponent" in player_cpp, "Build-remove animation must require gameplay success")
    require("CapturePointFromView() && CharacterAnimationComponent" in player_cpp, "Measurement animation must require gameplay success")
    require("TryInteractFocused() && CharacterAnimationComponent" in player_cpp, "Observe animation must require gameplay success")
    require("GetCapsuleComponent()->SetCapsule" not in player_cpp, "V5 animation must not rewrite gameplay capsule dimensions")

    input_config = read("game/Config/DefaultInput.ini")
    require('ActionName="Jump",Key=SpaceBar' in input_config, "V5 must expose keyboard jump input")
    require('ActionName="Jump",Key=Gamepad_FaceButton_Top' in input_config, "V5 must expose gamepad jump input")
    require('BindAction(TEXT("Jump"), IE_Pressed' in player_cpp and 'BindAction(TEXT("Jump"), IE_Released' in player_cpp, "Player must bind jump press/release")

    tests = read("game/Source/WorldMakers/Private/Tests/WMCharacterAnimationRuntimeTests.cpp")
    for name in (
        "WorldMakers.Visual.Animation.LocomotionTransitionsAreDeterministic",
        "WorldMakers.Visual.Animation.JumpFallLandSequenceIsExplicit",
        "WorldMakers.Visual.Animation.InteractionLayerDoesNotOwnMovement",
        "WorldMakers.Visual.Animation.LookAndPoseRemainBounded",
        "WorldMakers.Visual.Animation.StableActionIdsRoundTrip",
    ):
        require(name in tests, f"Missing V5 automation test: {name}")
    require(tests.count("IMPLEMENT_SIMPLE_AUTOMATION_TEST") >= 5, "V5 requires at least five animation automation tests")

    docs = require_tokens(
        "docs/v5-character-interaction-animation.md",
        "CharacterMovementComponent owns movement. Animation explains movement.",
        "Nine explicit states are stable",
        "eight stable semantic actions",
        "does **not** claim",
        "V6 — VFX & Fantastic/Scientific Feedback",
    )
    require("source-complete" in docs.lower(), "V5 docs must state source-complete boundary")

    roadmap = read("docs/visual-production-roadmap.md")
    require("V5 status: source-complete animation-runtime pass" in roadmap, "Visual roadmap must mark V5 source status explicitly")
    require("## V6 — VFX & Fantastic/Scientific Feedback" in roadmap, "Visual roadmap must preserve V6 next phase")

    game_config = read("game/Config/DefaultGame.ini")
    require('+DirectoriesToAlwaysStageAsNonUFS=(Path="WorldMakers/Visual")' in game_config, "V5 packaged manifest requires WorldMakers/Visual staging")

    workflow = read(".github/workflows/repo-quality.yml")
    require("python scripts/validate-v5-character-animation.py" in workflow, "Repository Quality must execute V5 animation gate")

    print("V5 character animation passed: nine locomotion states, jump/fall/land, upper-body gameplay actions, bounded source pose, AnimBP read-model and authored handoff are wired.")


if __name__ == "__main__":
    main()
