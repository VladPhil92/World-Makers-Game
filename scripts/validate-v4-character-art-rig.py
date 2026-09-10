#!/usr/bin/env python3
"""Validate V4 character art, rig, customization and production-handoff contracts."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CANONICAL = ROOT / "content/visual/character/avatar-rig-v4.json"
PACKAGED = ROOT / "game/Content/WorldMakers/Visual/avatar-rig-v4.json"

REQUIRED_FILES = (
    "game/Source/WorldMakers/Visual/WMAvatarArtTypes.h",
    "game/Source/WorldMakers/Visual/WMAvatarArtTypes.cpp",
    "game/Source/WorldMakers/Visual/WMProceduralAvatarGeometry.h",
    "game/Source/WorldMakers/Visual/WMProceduralAvatarGeometry.cpp",
    "game/Source/WorldMakers/Player/WMPlayerCharacter.h",
    "game/Source/WorldMakers/Player/WMPlayerCharacter.cpp",
    "game/Source/WorldMakers/Private/Tests/WMAvatarArtTests.cpp",
    "game/Config/DefaultGame.ini",
    "docs/v4-character-art-rig.md",
)

EXPECTED_JOINTS = [
    "root", "pelvis", "spine", "chest", "neck", "head", "jaw",
    "upperarm_l", "lowerarm_l", "hand_l", "upperarm_r", "lowerarm_r", "hand_r",
    "thigh_l", "calf_l", "foot_l", "thigh_r", "calf_r", "foot_r",
]
EXPECTED_SLOTS = {
    "body", "hair", "top", "bottom", "footwear", "head-accessory", "back-accessory", "hand-prop"
}


def fail(message: str) -> None:
    raise SystemExit(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def main() -> None:
    missing = [path for path in REQUIRED_FILES if not (ROOT / path).is_file()]
    require(not missing, f"Missing V4 files: {missing}")
    require(CANONICAL.is_file() and PACKAGED.is_file(), "V4 avatar manifest source/package is incomplete")
    require(CANONICAL.read_bytes() == PACKAGED.read_bytes(), "Packaged avatar rig manifest must be byte-equivalent to canonical JSON")

    payload = json.loads(CANONICAL.read_text(encoding="utf-8"))
    require(payload.get("schemaVersion") == 1, "V4 avatar manifest schemaVersion must be 1")
    require(payload.get("profileName") == "ChildExplorerV1", "V4 avatar profile name drifted")
    design = payload.get("designIntent", {})
    require(4.5 <= float(design.get("headsTall", 0)) <= 5.2, "Avatar silhouette must remain child-proportioned")
    require(design.get("collisionAuthority") == "ACharacter.CapsuleComponent", "Gameplay capsule must remain avatar collision authority")
    require(design.get("genderDefault") == "none", "Base avatar must not force a gender-default silhouette")

    joints = payload.get("joints", [])
    require([joint.get("id") for joint in joints] == EXPECTED_JOINTS, "V4 19-joint order/identity drifted")
    seen: set[str] = set()
    for joint in joints:
        joint_id = joint.get("id")
        parent = joint.get("parent")
        require(joint_id not in seen, f"Duplicate avatar joint: {joint_id}")
        require(parent is None or parent in seen, f"Avatar joint parent must precede child: {joint_id} -> {parent}")
        seen.add(joint_id)

    slots = payload.get("customizationSlots", [])
    require({slot.get("id") for slot in slots} == EXPECTED_SLOTS, "V4 customization slots drifted")
    for slot in slots:
        require(slot.get("anchor") in seen, f"Customization slot has unknown anchor: {slot.get('id')}")
        if slot.get("secondaryAnchor"):
            require(slot["secondaryAnchor"] in seen, f"Customization slot has unknown secondary anchor: {slot.get('id')}")

    privacy = payload.get("privacyAndSafety", {})
    for forbidden_flag in ("biometricCapture", "photoAvatarGeneration", "childVoiceFaceTraining"):
        require(privacy.get(forbidden_flag) is False, f"V4 privacy boundary must keep {forbidden_flag}=false")

    targets = payload.get("authoredTargets", {})
    for key in ("skeletalMesh", "skeleton", "animationBlueprint", "ikRig", "physicsAsset"):
        require(str(targets.get(key, "")).startswith("/Game/WorldMakers/Characters/Player/"), f"Missing authored target: {key}")

    types_h = read("game/Source/WorldMakers/Visual/WMAvatarArtTypes.h")
    types_cpp = read("game/Source/WorldMakers/Visual/WMAvatarArtTypes.cpp")
    for token in ("FWMAvatarProportions", "FWMAvatarPalette", "FWMAvatarArtBudget", "UWMAvatarVisualSettings", "FWMAvatarRigContract"):
        require(token in types_h, f"Avatar types missing token: {token}")
    require("Joints.Num() != 19" in types_cpp, "Runtime rig sanity must assert nineteen stable joints")

    geometry = read("game/Source/WorldMakers/Visual/WMProceduralAvatarGeometry.cpp")
    for token in ("BuildTorso", "BuildHead", "BuildHairCap", "BuildUpperArm", "BuildLowerArm", "BuildHand", "BuildThigh", "BuildCalf", "BuildFoot"):
        require(token in geometry, f"Procedural avatar geometry missing: {token}")
    require("/Engine/BasicShapes/" not in geometry, "Visible V4 avatar geometry must not wrap Engine BasicShapes")

    player_h = read("game/Source/WorldMakers/Player/WMPlayerCharacter.h")
    player_cpp = read("game/Source/WorldMakers/Player/WMPlayerCharacter.cpp")
    for token in ("AvatarRigRoot", "AvatarPelvisRig", "AvatarSpineRig", "AvatarHeadRig", "RefreshAvatarVisualPath", "IsProceduralAvatarActive"):
        require(token in player_h, f"Player character missing V4 rig API: {token}")
    for token in ("BuildProceduralAvatarArt", "GetSkeletalMeshAsset", "CharacterMovementComponent", "FWMPrototypeMotionStyle::Evaluate", "SetProceduralAvatarVisible"):
        require(token in player_cpp, f"Player character missing V4 implementation contract: {token}")
    require("GetCapsuleComponent()->SetCapsule" not in player_cpp, "V4 visual work must not rewrite gameplay capsule dimensions")

    config = read("game/Config/DefaultGame.ini")
    for token in ("[/Script/WorldMakers.WMAvatarVisualSettings]", "ProfileName=ChildExplorerV1", "bUseProceduralAvatarArt=True", "MaxSkinnedBones=48", "MaxSkinnedBones=64", "MaxSkinnedBones=96"):
        require(token in config, f"V4 config missing token: {token}")
    require('+DirectoriesToAlwaysStageAsNonUFS=(Path="WorldMakers/Visual")' in config, "V4 manifest must be staged in packaged builds")

    tests = read("game/Source/WorldMakers/Private/Tests/WMAvatarArtTests.cpp")
    for name in (
        "WorldMakers.Visual.Avatar.ProportionsAreChildReadable",
        "WorldMakers.Visual.Avatar.RigContractIsStable",
        "WorldMakers.Visual.Avatar.ProceduralGeometryFitsBudgets",
    ):
        require(name in tests, f"Missing V4 automation test: {name}")

    docs = read("docs/v4-character-art-rig.md")
    for token in ("19 stable joints", "Customization slots", "ACharacter::GetMesh()", "does **not** claim", "V5 owns authored locomotion"):
        require(token in docs, f"V4 documentation missing boundary token: {token}")

    roadmap = read("docs/visual-production-roadmap.md")
    require("## V4 — Character Art & Rig" in roadmap and "## V5 — Character & Interaction Animation" in roadmap, "Visual roadmap must retain V4/V5 sequence")

    workflow = read(".github/workflows/repo-quality.yml")
    require("python scripts/validate-v4-character-art-rig.py" in workflow, "Repository Quality must execute V4 gate")

    print("V4 character art and rig passed: five-head child silhouette, 19-joint hierarchy, modular slots, procedural fallback, skeletal handoff, budgets and privacy boundary are wired.")


if __name__ == "__main__":
    main()
