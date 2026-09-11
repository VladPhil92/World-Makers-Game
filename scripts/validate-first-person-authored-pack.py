#!/usr/bin/env python3
"""Validate the first-person authored asset production contract in pre/post activation states."""
from __future__ import annotations

import ast
import hashlib
import json
import math
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CANONICAL = ROOT / "content/visual/first-person/first-person-authored-pack-v1.json"
STAGED = ROOT / "game/Content/WorldMakers/Visual/first-person-authored-pack-v1.json"
ACTIVATION = ROOT / "content/visual/first-person/first-person-native-activation-v1.json"
ACTIVATION_STAGED = ROOT / "game/Content/WorldMakers/Visual/first-person-native-activation-v1.json"
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
SHA1_RE = re.compile(r"^[0-9a-f]{40}$")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit("First-person authored pack validation failed: " + message)


def generated_bundle() -> tuple[dict, str]:
    with tempfile.TemporaryDirectory() as temp_dir:
        out_a = Path(temp_dir) / "a.json"
        out_b = Path(temp_dir) / "b.json"
        subprocess.run([sys.executable, str(GENERATOR), "--output", str(out_a)], check=True)
        subprocess.run([sys.executable, str(GENERATOR), "--output", str(out_b)], check=True)
        a = out_a.read_bytes()
        b = out_b.read_bytes()
        require(a == b, "generator output is not byte-deterministic")
        return json.loads(a), hashlib.sha256(a).hexdigest()


def importer_mutates_authored_present(source: str) -> bool:
    tree = ast.parse(source)
    for node in ast.walk(tree):
        if isinstance(node, (ast.Assign, ast.AnnAssign, ast.AugAssign)):
            targets = node.targets if isinstance(node, ast.Assign) else [node.target]
            for target in targets:
                for child in ast.walk(target):
                    if isinstance(child, ast.Subscript) and isinstance(child.slice, ast.Constant) and child.slice.value == "authoredPresent":
                        return True
        if isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute) and node.func.attr in {"update", "setdefault", "__setitem__"}:
            for arg in node.args:
                if isinstance(arg, ast.Constant) and arg.value == "authoredPresent":
                    return True
                if isinstance(arg, ast.Dict) and any(isinstance(key, ast.Constant) and key.value == "authoredPresent" for key in arg.keys if key is not None):
                    return True
    return False


def validate_mesh(lod: dict, slot_id: str, index: int, skinned: bool) -> int:
    vertices = lod.get("vertices", [])
    normals = lod.get("normals", [])
    uv0 = lod.get("uv0", [])
    triangles = lod.get("triangles", [])
    weights = lod.get("weights", [])
    require(vertices and triangles, f"{slot_id} LOD{index} must contain geometry")
    require(len(normals) == len(vertices) == len(uv0) == len(weights), f"{slot_id} LOD{index} stream lengths differ")
    for tri in triangles:
        require(len(tri) == 3 and all(isinstance(i, int) and 0 <= i < len(vertices) for i in tri), f"{slot_id} LOD{index} has invalid triangle")
    for normal in normals:
        length = math.sqrt(sum(float(v) * float(v) for v in normal))
        require(0.8 <= length <= 1.2, f"{slot_id} LOD{index} has non-unit normal")
    if skinned:
        for influence_set in weights:
            require(1 <= len(influence_set) <= 2, f"{slot_id} LOD{index} must use 1-2 influences")
            require(abs(sum(float(pair[1]) for pair in influence_set) - 1.0) <= 0.001, f"{slot_id} LOD{index} weights must normalize")
            require(all(pair[0] in REQUIRED_BONES for pair in influence_set), f"{slot_id} LOD{index} references unknown bone")
    else:
        require(all(not influence_set for influence_set in weights), f"{slot_id} LOD{index} static mesh carries skin weights")
    return len(triangles)


def main() -> None:
    required = (CANONICAL, STAGED, ACTIVATION, ACTIVATION_STAGED, GENERATOR, BLENDER, IMPORTER, RUNTIME_H, RUNTIME_CPP, BRIDGE_H, BRIDGE_CPP, TESTS, DOC, WORKFLOW, QUALITY)
    missing = [str(p.relative_to(ROOT)) for p in required if not p.is_file()]
    require(not missing, f"missing files: {missing}")
    require(CANONICAL.read_bytes() == STAGED.read_bytes(), "canonical/staged authored pack parity failed")
    require(ACTIVATION.read_bytes() == ACTIVATION_STAGED.read_bytes(), "canonical/staged activation parity failed")

    contract = json.loads(CANONICAL.read_text(encoding="utf-8"))
    activation = json.loads(ACTIVATION.read_text(encoding="utf-8"))
    require(contract.get("schemaVersion") == 1 and contract.get("packId") == "visual.first-person-authored-pack.v1", "pack identity drifted")
    require(contract.get("presentationOnly") is True and contract.get("gameplayAuthority") is False, "pack must remain presentation-only")
    require(contract["coordinateContract"] == {"units": "centimeters", "forward": "+X", "right": "+Y", "up": "+Z"}, "coordinate contract drifted")

    skeleton = contract["skeleton"]
    require(skeleton["boneCount"] == 7 and set(skeleton["requiredBones"]) == REQUIRED_BONES, "seven-bone first-person skeleton drifted")
    require(skeleton["maxSkinInfluences"] == 2 and skeleton["rootMotion"] is False, "skinning/root-motion contract drifted")

    assets = contract["assets"]
    animations = contract["animations"]
    require(len(assets) == 5 and {item["slotId"] for item in assets} == REQUIRED_SLOTS, "five stable authored asset slots are required")
    require(len(animations) == 9 and {item["actionId"] for item in animations} == REQUIRED_ACTIONS, "nine stable animation slots are required")
    require(all(item["minimumLods"] == 3 for item in assets), "all five assets require three LODs")
    require(all(str(item["unrealPath"]).startswith("/Game/WorldMakers/") for item in assets), "stable Unreal paths drifted")
    require(sum(1 for item in animations if item["loop"]) == 1 and next(item for item in animations if item["loop"])["actionId"] == "scan-hold", "scan-hold must remain the only looping action")

    flags = [bool(item["authoredPresent"]) for item in assets + animations]
    all_off = not any(flags)
    all_on = all(flags)
    require(all_off or all_on, "partial first-person authored activation is forbidden")
    if all_off:
        require(activation.get("status") == "blocked" and activation.get("activated") is False, "all-off pack requires blocked activation manifest")
        require(contract["certificationBoundary"].get("authoredAssetsPresent") is False, "all-off pack must report authoredAssetsPresent=false")
    else:
        require(activation.get("status") == "activated" and activation.get("activated") is True, "all-on pack requires activated manifest")
        require(SHA1_RE.fullmatch(str(activation.get("targetCommitSha", ""))) is not None, "activated manifest requires reviewed source SHA")
        for key in ("allFiveAssetsApproved", "allNineAnimationsApproved", "humanReviewApproved", "deviceReviewApproved"):
            require(activation.get(key) is True, f"activated manifest approval missing: {key}")
        require(contract["certificationBoundary"].get("authoredAssetsPresent") is True, "all-on pack must report authoredAssetsPresent=true")

    takeover = contract["takeoverPolicy"]
    require(takeover["allOrProxy"] is True and takeover["partialAuthoredTakeoverAllowed"] is False, "all-or-proxy contract drifted")
    require(takeover["requiresAllFiveAssets"] is True and takeover["requiresAllNineAnimations"] is True, "takeover completeness drifted")
    require(takeover["collisionAuthority"] == "character-capsule" and takeover["assetCollision"] == "none", "art must not acquire collision authority")

    bundle, digest = generated_bundle()
    require(digest == contract["expectedGeneratedSha256"], f"generated SHA mismatch: expected {contract['expectedGeneratedSha256']} got {digest}")
    require(bundle["schemaVersion"] == 1 and bundle["bundleId"] == "visual.first-person-authored-source.v1", "generated bundle identity drifted")
    require({bone["id"] for bone in bundle["skeleton"]["bones"]} == REQUIRED_BONES, "generated skeleton drifted")
    require(set(bundle["assets"]) == REQUIRED_SLOTS, "generated asset set drifted")
    for slot_id, asset in bundle["assets"].items():
        require(len(asset["lods"]) == 3, f"{slot_id} must contain three LODs")
        counts = [validate_mesh(lod, slot_id, i, slot_id == "arms.firstperson") for i, lod in enumerate(asset["lods"])]
        require(counts[0] > counts[-1] and all(counts[i] >= counts[i + 1] for i in range(2)), f"{slot_id} LOD reduction invalid")

    generated_animations = bundle["animations"]
    require(len(generated_animations) == 9 and {item["actionId"] for item in generated_animations} == REQUIRED_ACTIONS, "generated animation set drifted")
    contract_by_action = {item["actionId"]: item for item in animations}
    for clip in generated_animations:
        action = clip["actionId"]
        require(clip["fps"] == 30 and len(clip["keys"]) >= 3, f"{action} animation source invalid")
        require(abs(float(clip["durationSeconds"]) - float(contract_by_action[action]["durationSeconds"])) <= 0.001, f"{action} duration drifted")
        require(bool(clip["loop"]) == bool(contract_by_action[action]["loop"]), f"{action} loop contract drifted")
        for key in clip["keys"]:
            require(key["bones"].get("root", {}).get("locationCm") == [0, 0, 0], f"{action} introduced root translation")

    runtime = RUNTIME_H.read_text(encoding="utf-8") + "\n" + RUNTIME_CPP.read_text(encoding="utf-8")
    bridge = BRIDGE_H.read_text(encoding="utf-8") + "\n" + BRIDGE_CPP.read_text(encoding="utf-8")
    for token in ("FWMFirstPersonAuthoredAvailability", "RequiredAnimationCount = 9", "CanTakeOver", "SK_WM_FirstPersonArms", "SM_WM_Scanner", "SM_WM_BuildTool", "SM_WM_MeasureTool", "SM_WM_WristDevice"):
        require(token in runtime, f"authored runtime missing: {token}")
    for token in ("WMEnableFirstPersonAuthored", "ApplyAuthoredTakeover", "ApplyProxyFallback", "PlayAnimation", "SetCollisionEnabled(ECollisionEnabled::NoCollision)"):
        require(token in bridge, f"authored bridge missing: {token}")
    for forbidden in ("AddMovementInput", "TryPlaceCurrentPiece", "RecordComposableEvidence", "GrantReward", "SetGlobalTimeDilation"):
        require(forbidden not in bridge, f"authored bridge acquired gameplay authority: {forbidden}")

    tests = TESTS.read_text(encoding="utf-8")
    for name in ("WorldMakers.Visual.FirstPersonAuthored.PathsMatchStableSlots", "WorldMakers.Visual.FirstPersonAuthored.TakeoverIsAllOrProxy", "WorldMakers.Visual.FirstPersonAuthored.AnimationContractIsComplete"):
        require(name in tests, f"missing automation test: {name}")

    blender_text = BLENDER.read_text(encoding="utf-8")
    for token in ("SK_WM_FirstPersonArms_LOD", "SM_WM_Scanner", "SM_WM_BuildTool", "SM_WM_MeasureTool", "SM_WM_WristDevice", "bpy.ops.export_scene.fbx"):
        require(token in blender_text, f"Blender handoff missing: {token}")
    importer_text = IMPORTER.read_text(encoding="utf-8")
    for token in ("AssetImportTask", "allFiveAssetsImported", "allNineAnimationsImported", "humanReviewApproved"):
        require(token in importer_text, f"Unreal importer missing: {token}")
    require(not importer_mutates_authored_present(importer_text), "Unreal importer must never mutate authoredPresent")

    workflow_text = WORKFLOW.read_text(encoding="utf-8")
    require("BLENDER_EXE" in workflow_text and "UE_EDITOR_CMD" in workflow_text and "WMEnableFirstPersonAuthored" in workflow_text, "native workflow contract drifted")
    require("python scripts/validate-first-person-authored-pack.py" in QUALITY.read_text(encoding="utf-8"), "Repository Quality must execute authored gate")
    doc = DOC.read_text(encoding="utf-8").lower()
    for phrase in ("all-or-proxy", "seven-bone", "human deformation/camera review", "representative tablet", "-wmenablefirstpersonauthored"):
        require(phrase in doc, f"documentation missing: {phrase}")

    state = "all-on/activated" if all_on else "all-off/blocked"
    print(f"First-person authored asset pack validated in {state} state: 5 assets, 9 animations, deterministic SHA-256 {digest}, all-or-proxy takeover and native handoff are intact.")


if __name__ == "__main__":
    main()
