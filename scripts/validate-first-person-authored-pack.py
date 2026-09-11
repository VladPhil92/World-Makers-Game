#!/usr/bin/env python3
"""Validate the first-person authored asset production source contract."""
from __future__ import annotations

import hashlib
import json
import math
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CANONICAL = ROOT / "content/visual/first-person/first-person-authored-pack-v1.json"
STAGED = ROOT / "game/Content/WorldMakers/Visual/first-person-authored-pack-v1.json"
GENERATOR = ROOT / "scripts/generate-first-person-authored-source.py"
BLENDER = ROOT / "scripts/blender-build-first-person-authored.py"
IMPORTER = ROOT / "scripts/unreal-import-first-person-authored.py"
RUNTIME_H = ROOT / "game/Source/WorldMakers/Visual/WMFirstPersonAuthoredRuntime.h"
RUNTIME_CPP = ROOT / "game/Source/WorldMakers/Visual/WMFirstPersonAuthoredRuntime.cpp"
BRIDGE_H = ROOT / "game/Source/WorldMakers/Visual/WMFirstPersonAuthoredBridgeSubsystem.h"
BRIDGE_CPP = ROOT / "game/Source/WorldMakers/Visual/WMFirstPersonAuthoredBridgeSubsystem.cpp"
TESTS = ROOT / "game/Source/WorldMakers/Private/Tests/WMFirstPersonAuthoredTests.cpp"
DOC = ROOT / "docs/first-person-authored-asset-production-v1.md"
WORKFLOW = ROOT / ".github/workflows/first-person-authored-native.yml"
QUALITY = ROOT / ".github/workflows/repo-quality.yml"

REQUIRED_SLOTS = {"arms.firstperson", "tool.scanner", "tool.build", "tool.measure", "device.wrist"}
REQUIRED_ACTIONS = {
    "tool-raise", "tool-lower", "scan-anticipate", "scan-hold", "scan-settle",
    "build-point", "build-confirm", "measure-focus", "observe-focus",
}
REQUIRED_BONES = {"root", "upperarm_l", "lowerarm_l", "hand_l", "upperarm_r", "lowerarm_r", "hand_r"}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit("First-person authored pack validation failed: " + message)


def generated_bundle() -> tuple[dict, str, bytes]:
    with tempfile.TemporaryDirectory() as temp_dir:
        out_a = Path(temp_dir) / "a.json"
        out_b = Path(temp_dir) / "b.json"
        subprocess.run([sys.executable, str(GENERATOR), "--output", str(out_a)], check=True)
        subprocess.run([sys.executable, str(GENERATOR), "--output", str(out_b)], check=True)
        data_a = out_a.read_bytes()
        data_b = out_b.read_bytes()
        require(data_a == data_b, "generator output is not byte-deterministic")
        digest = hashlib.sha256(data_a).hexdigest()
        return json.loads(data_a), digest, data_a


def validate_mesh(source: dict, slot_id: str, lod_index: int, skinned: bool) -> int:
    vertices = source.get("vertices", [])
    normals = source.get("normals", [])
    uv0 = source.get("uv0", [])
    triangles = source.get("triangles", [])
    weights = source.get("weights", [])
    require(vertices and triangles, f"{slot_id} LOD{lod_index} must contain geometry")
    require(len(normals) == len(vertices), f"{slot_id} LOD{lod_index} normals must match vertices")
    require(len(uv0) == len(vertices), f"{slot_id} LOD{lod_index} UV0 must match vertices")
    require(len(weights) == len(vertices), f"{slot_id} LOD{lod_index} weights must match vertices")
    for tri in triangles:
        require(len(tri) == 3, f"{slot_id} LOD{lod_index} must be triangulated")
        require(all(isinstance(index, int) and 0 <= index < len(vertices) for index in tri), f"{slot_id} LOD{lod_index} triangle index out of range")
    for normal in normals:
        length = math.sqrt(sum(float(value) * float(value) for value in normal))
        require(0.8 <= length <= 1.2, f"{slot_id} LOD{lod_index} has non-unit normal")
    if skinned:
        for vertex_weights in weights:
            require(1 <= len(vertex_weights) <= 2, f"{slot_id} LOD{lod_index} must use 1-2 skin influences")
            total = sum(float(pair[1]) for pair in vertex_weights)
            require(abs(total - 1.0) <= 0.001, f"{slot_id} LOD{lod_index} skin weights must normalize to 1")
            require(all(pair[0] in REQUIRED_BONES for pair in vertex_weights), f"{slot_id} LOD{lod_index} references unknown bone")
    else:
        require(all(not vertex_weights for vertex_weights in weights), f"{slot_id} LOD{lod_index} static mesh must not carry skin weights")
    return len(triangles)


def main() -> None:
    required_files = (CANONICAL, STAGED, GENERATOR, BLENDER, IMPORTER, RUNTIME_H, RUNTIME_CPP, BRIDGE_H, BRIDGE_CPP, TESTS, DOC, WORKFLOW, QUALITY)
    missing = [str(path.relative_to(ROOT)) for path in required_files if not path.is_file()]
    require(not missing, f"missing files: {missing}")
    require(CANONICAL.read_bytes() == STAGED.read_bytes(), "canonical and staged authored contracts must be byte-identical")

    contract = json.loads(CANONICAL.read_text(encoding="utf-8"))
    require(contract.get("schemaVersion") == 1, "schemaVersion must be 1")
    require(contract.get("packId") == "visual.first-person-authored-pack.v1", "packId drifted")
    require(contract.get("presentationOnly") is True and contract.get("gameplayAuthority") is False, "authored bridge must remain presentation-only")
    require(contract["coordinateContract"] == {"units": "centimeters", "forward": "+X", "right": "+Y", "up": "+Z"}, "coordinate contract drifted")

    skeleton = contract["skeleton"]
    require(skeleton["boneCount"] == 7, "first-person skeleton must contain exactly seven bones")
    require(set(skeleton["requiredBones"]) == REQUIRED_BONES, "first-person skeleton bone IDs drifted")
    require(skeleton["maxSkinInfluences"] == 2, "first-person source skin influence ceiling must remain 2")
    require(skeleton["rootMotion"] is False, "root motion must remain disabled")

    assets = contract["assets"]
    require(len(assets) == 5 and {item["slotId"] for item in assets} == REQUIRED_SLOTS, "five stable authored asset slots are required")
    require(all(item["minimumLods"] == 3 for item in assets), "all five authored assets require three source LODs")
    require(all(item["authoredPresent"] is False for item in assets), "source phase must not claim native assets are approved")
    require(all(str(item["unrealPath"]).startswith("/Game/WorldMakers/") for item in assets), "all assets require stable /Game/WorldMakers paths")

    animations = contract["animations"]
    require(len(animations) == 9 and {item["actionId"] for item in animations} == REQUIRED_ACTIONS, "nine stable animation slots are required")
    require(all(item["authoredPresent"] is False for item in animations), "source phase must not claim native animation assets are approved")
    require(sum(1 for item in animations if item["loop"]) == 1 and next(item for item in animations if item["loop"])["actionId"] == "scan-hold", "scan-hold must be the only looping action")

    takeover = contract["takeoverPolicy"]
    require(takeover["allOrProxy"] is True and takeover["partialAuthoredTakeoverAllowed"] is False, "partial authored takeover must remain forbidden")
    require(takeover["requiresAllFiveAssets"] is True and takeover["requiresAllNineAnimations"] is True, "full-set takeover requirements drifted")
    require(takeover["collisionAuthority"] == "character-capsule" and takeover["assetCollision"] == "none", "first-person art must not acquire collision authority")

    bundle, digest, _ = generated_bundle()
    require(digest == contract["expectedGeneratedSha256"], f"generated SHA mismatch: expected {contract['expectedGeneratedSha256']} got {digest}")
    require(bundle["schemaVersion"] == 1 and bundle["bundleId"] == "visual.first-person-authored-source.v1", "generated bundle identity drifted")
    require({bone["id"] for bone in bundle["skeleton"]["bones"]} == REQUIRED_BONES, "generated source skeleton drifted")
    require(set(bundle["assets"]) == REQUIRED_SLOTS, "generated source asset set drifted")

    for slot_id, asset in bundle["assets"].items():
        require(len(asset["lods"]) == 3, f"{slot_id} must contain exactly three source LODs")
        triangle_counts = [validate_mesh(lod, slot_id, index, slot_id == "arms.firstperson") for index, lod in enumerate(asset["lods"])]
        require(triangle_counts[0] > triangle_counts[-1], f"{slot_id} must reduce triangle count by the final LOD")
        require(all(triangle_counts[index] >= triangle_counts[index + 1] for index in range(2)), f"{slot_id} LOD triangle counts must be non-increasing")

    generated_animations = bundle["animations"]
    require(len(generated_animations) == 9 and {item["actionId"] for item in generated_animations} == REQUIRED_ACTIONS, "generated animation set drifted")
    contract_by_action = {item["actionId"]: item for item in animations}
    for clip in generated_animations:
        action_id = clip["actionId"]
        require(clip["fps"] == 30, f"{action_id} must be authored at 30 fps")
        require(abs(float(clip["durationSeconds"]) - float(contract_by_action[action_id]["durationSeconds"])) <= 0.001, f"{action_id} duration drifted")
        require(bool(clip["loop"]) == bool(contract_by_action[action_id]["loop"]), f"{action_id} loop contract drifted")
        require(len(clip["keys"]) >= 3, f"{action_id} requires authored animation keys")
        for key in clip["keys"]:
            root = key["bones"].get("root")
            require(root and root["locationCm"] == [0, 0, 0], f"{action_id} may not introduce root translation")

    runtime = RUNTIME_H.read_text(encoding="utf-8") + "\n" + RUNTIME_CPP.read_text(encoding="utf-8")
    bridge = BRIDGE_H.read_text(encoding="utf-8") + "\n" + BRIDGE_CPP.read_text(encoding="utf-8")
    for token in ("FWMFirstPersonAuthoredAvailability", "RequiredAnimationCount = 9", "CanTakeOver", "SK_WM_FirstPersonArms", "SM_WM_Scanner", "SM_WM_BuildTool", "SM_WM_MeasureTool", "SM_WM_WristDevice"):
        require(token in runtime, f"authored runtime missing contract token: {token}")
    for token in ("WMEnableFirstPersonAuthored", "ApplyAuthoredTakeover", "ApplyProxyFallback", "PlayAnimation", "SetCollisionEnabled(ECollisionEnabled::NoCollision)"):
        require(token in bridge, f"authored bridge missing safety/integration token: {token}")
    for forbidden in ("AddMovementInput", "TryPlaceCurrentPiece", "RecordComposableEvidence", "GrantReward", "SetGlobalTimeDilation"):
        require(forbidden not in bridge, f"authored bridge must not acquire gameplay authority: {forbidden}")

    tests = TESTS.read_text(encoding="utf-8")
    for name in (
        "WorldMakers.Visual.FirstPersonAuthored.PathsMatchStableSlots",
        "WorldMakers.Visual.FirstPersonAuthored.TakeoverIsAllOrProxy",
        "WorldMakers.Visual.FirstPersonAuthored.AnimationContractIsComplete",
    ):
        require(name in tests, f"missing Unreal Automation test: {name}")

    blender_text = BLENDER.read_text(encoding="utf-8")
    for token in ("SK_WM_FirstPersonArms_LOD", "SM_WM_Scanner", "SM_WM_BuildTool", "SM_WM_MeasureTool", "SM_WM_WristDevice", "bpy.ops.export_scene.fbx"):
        require(token in blender_text, f"Blender handoff missing: {token}")
    importer_text = IMPORTER.read_text(encoding="utf-8")
    for token in ("AssetImportTask", "allFiveAssetsImported", "allNineAnimationsImported", "humanReviewApproved"):
        require(token in importer_text, f"Unreal importer missing: {token}")
    require("authoredPresent" not in importer_text, "Unreal importer must never mutate authoredPresent")

    workflow_text = WORKFLOW.read_text(encoding="utf-8")
    require("BLENDER_EXE" in workflow_text and "UE_EDITOR_CMD" in workflow_text and "WMEnableFirstPersonAuthored" in workflow_text, "native workflow must require Blender, Unreal and explicit takeover review")
    quality_text = QUALITY.read_text(encoding="utf-8")
    require("python scripts/validate-first-person-authored-pack.py" in quality_text, "Repository Quality must execute first-person authored gate")

    doc = DOC.read_text(encoding="utf-8").lower()
    for phrase in ("all-or-proxy", "seven-bone", "human deformation/camera review", "representative tablet", "-wmenablefirstpersonauthored"):
        require(phrase in doc, f"documentation missing: {phrase}")

    print(f"First-person authored asset pack validated: 5 source assets, 3 LODs each, 7-bone skinned arms, 9 animation clips, SHA-256 {digest}, explicit all-or-proxy takeover and native Blender/Unreal handoff are present.")


if __name__ == "__main__":
    main()
