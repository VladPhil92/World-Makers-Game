#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import math
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "content/visual/authored/character-p3-source-pack.json"
GENERATOR = ROOT / "scripts/generate-p3-character-source.py"
V4 = ROOT / "content/visual/character/avatar-rig-v4.json"
P1 = ROOT / "content/visual/authored/authored-assets-p1.json"
BLENDER = ROOT / "scripts/blender/build-p3-character-fbx.py"
UNREAL = ROOT / "scripts/unreal/import-p3-character-assets.py"
NATIVE_WORKFLOW = ROOT / ".github/workflows/p3-character-native-import.yml"
REPO_WORKFLOW = ROOT / ".github/workflows/repo-quality.yml"
DOC = ROOT / "docs/p3-authored-character-cosmetics.md"
ROADMAP = ROOT / "docs/authored-production-roadmap.md"

EXPECTED_JOINTS = [
    "root","pelvis","spine","chest","neck","head","jaw",
    "upperarm_l","lowerarm_l","hand_l","upperarm_r","lowerarm_r","hand_r",
    "thigh_l","calf_l","foot_l","thigh_r","calf_r","foot_r",
]
EXPECTED_SLOTS = [
    "body","hair","top","bottom","footwear","head-accessory","back-accessory","hand-prop"
]


def fail(message: str) -> None:
    raise SystemExit("P3 validation failed: " + message)


def normalized_markdown(text: str) -> str:
    return re.sub(r"[`*_#]", "", text).lower()


def finite3(v) -> bool:
    return isinstance(v, list) and len(v) == 3 and all(isinstance(x, (int, float)) and math.isfinite(x) for x in v)


def main() -> None:
    for path in (MANIFEST, GENERATOR, V4, P1, BLENDER, UNREAL, NATIVE_WORKFLOW, REPO_WORKFLOW, DOC, ROADMAP):
        if not path.exists():
            fail(f"missing required file {path.relative_to(ROOT)}")

    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    v4 = json.loads(V4.read_text(encoding="utf-8"))
    p1 = json.loads(P1.read_text(encoding="utf-8"))

    if manifest.get("schemaVersion") != 1:
        fail("unsupported P3 schemaVersion")
    for key, expected in (("units","centimeters"),("upAxis","Z"),("forwardAxis","X"),("retargetPose","neutral-a-pose")):
        if manifest.get(key) != expected:
            fail(f"{key} does not match V4 production coordinates")
    if manifest.get("baseHeightCm") != 158.0 or manifest.get("headsTall") != 5.0:
        fail("P3 changed the V4 five-head / 158 cm silhouette")

    v4_joints = [j["id"] for j in v4["joints"]]
    if v4_joints != EXPECTED_JOINTS:
        fail("V4 joint contract no longer matches the expected 19-joint handoff")
    v4_slots = [s["id"] for s in v4["customizationSlots"]]
    if v4_slots != EXPECTED_SLOTS:
        fail("V4 customization slot contract changed")
    if manifest["skeleton"].get("jointCount") != len(EXPECTED_JOINTS):
        fail("P3 must preserve exactly 19 joints")
    if manifest["skeleton"].get("maxSkinInfluences") != 4:
        fail("P3 skin influence ceiling must remain four")

    modules = manifest.get("modules", [])
    if [m.get("slot") for m in modules] != EXPECTED_SLOTS:
        fail("P3 must define all eight V4 customization slots in stable order")
    if len({m["objectPath"] for m in modules}) != len(modules):
        fail("P3 module object paths must be unique")
    for m in modules:
        if m["kind"] not in {"SkeletalMesh","StaticMesh"}:
            fail(f"{m['slot']} has unsupported kind")
        if int(m["lodCount"]) < 2:
            fail(f"{m['slot']} must have authored LODs")
        if int(m["materialSlots"]) < 1 or int(m["materialSlots"]) > 4:
            fail(f"{m['slot']} material-slot budget is invalid")
        if not m["objectPath"].startswith("/Game/WorldMakers/Characters/Player/"):
            fail(f"{m['slot']} escaped the World Makers player namespace")

    body = modules[0]
    p1_body = next((a for a in p1["assets"] if a["id"] == "character.player.child-explorer"), None)
    if not p1_body or p1_body["kind"] != "SkeletalMesh":
        fail("P1 Child Explorer SkeletalMesh target is missing")
    if body["objectPath"] != p1_body["objectPath"]:
        fail("P3 body target diverges from P1")
    if body["lodCount"] < p1_body["minLods"] or body["materialSlots"] > p1_body["maxMaterialSlots"]:
        fail("P3 body violates P1 LOD/material budgets")
    if p1_body.get("authoredPresent") is not False:
        fail("P3 must not pre-authorize the native character asset")

    boundary = manifest.get("productionBoundary", {})
    expected_boundary = {
        "sourceGeometryPresent": True,
        "sourceSkinWeightsPresent": True,
        "nativeUassetsPresent": False,
        "nativeImportRequired": True,
        "deformationReviewRequired": True,
        "deviceReviewRequired": True,
    }
    if boundary != expected_boundary:
        fail("P3 production boundary must remain fail-closed")

    runtime = manifest.get("runtime", {})
    if runtime.get("bodyAuthority") != "ACharacter.GetMesh":
        fail("P3 body must use ACharacter.GetMesh")
    if runtime.get("collisionAuthority") != "ACharacter.CapsuleComponent":
        fail("P3 may not move gameplay collision to the visual mesh")
    if runtime.get("proceduralFallbackRequired") is not True or runtime.get("rootMotionDefault") is not False:
        fail("P3 changed fallback or movement authority")

    privacy = manifest.get("privacyAndSafety", {})
    if privacy.get("biometricCapture") or privacy.get("photoAvatarGeneration") or privacy.get("childVoiceFaceTraining"):
        fail("P3 introduced prohibited child biometric/avatar-training behavior")

    expected_hash = manifest.get("source", {}).get("sha256", "")
    if not re.fullmatch(r"[0-9a-f]{64}", expected_hash):
        fail("P3 source hash is invalid")

    with tempfile.TemporaryDirectory(prefix="wm-p3-a-") as a_tmp, tempfile.TemporaryDirectory(prefix="wm-p3-b-") as b_tmp:
        a_dir, b_dir = Path(a_tmp), Path(b_tmp)
        subprocess.run([sys.executable, str(GENERATOR), "--output-dir", str(a_dir), "--verify"], check=True, cwd=ROOT)
        subprocess.run([sys.executable, str(GENERATOR), "--output-dir", str(b_dir), "--verify"], check=True, cwd=ROOT)
        a_bytes = (a_dir / manifest["source"]["bundleFilename"]).read_bytes()
        b_bytes = (b_dir / manifest["source"]["bundleFilename"]).read_bytes()
        if a_bytes != b_bytes:
            fail("P3 source generator is not deterministic")
        if hashlib.sha256(a_bytes).hexdigest() != expected_hash:
            fail("P3 generated source hash does not match manifest")
        source = json.loads(a_bytes)

    source_joints = source.get("skeleton", {}).get("joints", [])
    if [j.get("id") for j in source_joints] != EXPECTED_JOINTS:
        fail("generated source does not preserve the 19 V4 joints")
    for joint in source_joints:
        if not finite3(joint.get("headCm")):
            fail(f"joint {joint.get('id')} has invalid rest position")
    if source.get("baseHeightCm") != 158.0 or source.get("headsTall") != 5.0:
        fail("generated source changed avatar proportions")

    source_modules = source.get("modules", [])
    if [m.get("slot") for m in source_modules] != EXPECTED_SLOTS:
        fail("generated source modules do not match P3 manifest")

    known_bones = set(EXPECTED_JOINTS)
    for source_module, target in zip(source_modules, modules):
        if source_module["assetName"] != target["assetName"] or len(source_module["lods"]) != target["lodCount"]:
            fail(f"{target['slot']} generated LOD contract mismatch")
        prior_triangles = None
        for lod in source_module["lods"]:
            verts, normals, uv0, weights, tris = (
                lod["vertices"], lod["normals"], lod["uv0"], lod["weights"], lod["triangles"]
            )
            if not verts or len(verts) != len(normals) or len(verts) != len(uv0) or len(verts) != len(weights):
                fail(f"{target['slot']} LOD{lod['lod']} vertex attribute streams are inconsistent")
            if not tris or lod.get("triangleCount") != len(tris):
                fail(f"{target['slot']} LOD{lod['lod']} triangle count is invalid")
            if prior_triangles is not None and len(tris) >= prior_triangles:
                fail(f"{target['slot']} authored LOD chain is not strictly reducing")
            prior_triangles = len(tris)
            for n in normals:
                if not finite3(n):
                    fail(f"{target['slot']} has invalid normal")
                length = math.sqrt(sum(float(x)*float(x) for x in n))
                if not 0.98 <= length <= 1.02:
                    fail(f"{target['slot']} has non-unit source normal")
            for uv in uv0:
                if not isinstance(uv, list) or len(uv) != 2 or not all(isinstance(x,(int,float)) and math.isfinite(x) for x in uv):
                    fail(f"{target['slot']} has invalid UV0")
            max_index = len(verts) - 1
            if any(len(t) != 3 or min(t) < 0 or max(t) > max_index for t in tris):
                fail(f"{target['slot']} has invalid triangle indices")
            if source_module["kind"] == "skinned":
                for influences in weights:
                    if not 1 <= len(influences) <= 4:
                        fail(f"{target['slot']} has invalid skin influence count")
                    if any(bone not in known_bones or weight <= 0.0 for bone, weight in influences):
                        fail(f"{target['slot']} references invalid bone/weight")
                    if not math.isclose(sum(float(w) for _,w in influences), 1.0, abs_tol=1e-4):
                        fail(f"{target['slot']} skin weights do not normalize to one")
            elif any(influences for influences in weights):
                fail(f"{target['slot']} static attachment unexpectedly contains skin weights")

    body_lod0 = source_modules[0]["lods"][0]
    z_values = [v[2] for v in body_lod0["vertices"]]
    if min(z_values) < -0.01 or not math.isclose(max(z_values), 158.0, abs_tol=0.01):
        fail("P3 body does not preserve the 0–158 cm silhouette envelope")
    if body_lod0["triangleCount"] > v4["lodBudgets"]["high"]["maxTriangles"]:
        fail("P3 body LOD0 exceeds V4 high triangle ceiling")
    if source_modules[0]["lods"][-1]["triangleCount"] > v4["lodBudgets"]["low"]["maxTriangles"]:
        fail("P3 body lowest LOD exceeds V4 low triangle ceiling")

    blender_text = BLENDER.read_text(encoding="utf-8")
    for token in ("vertex_groups", "ARMATURE", "add_leaf_bones=False", "UVMap"):
        if token not in blender_text:
            fail(f"Blender bridge missing required rig/export token: {token}")
    unreal_text = UNREAL.read_text(encoding="utf-8")
    for token in ("FBXIT_SKELETAL_MESH", "SkeletalMeshEditorSubsystem", "create_physics_asset", "authoredPresentMutated"):
        if token not in unreal_text:
            fail(f"Unreal import bridge missing token: {token}")

    native_workflow = NATIVE_WORKFLOW.read_text(encoding="utf-8")
    for token in ("workflow_dispatch", "UNREAL_SELF_HOSTED_ENABLED", "BLENDER_EXE", "validate-p3-character-assets.py", "build-p3-character-fbx.py", "import-p3-character-assets.py"):
        if token not in native_workflow:
            fail(f"P3 native workflow missing: {token}")
    repo_workflow = REPO_WORKFLOW.read_text(encoding="utf-8")
    if "Validate P3 Authored Character Cosmetics" not in repo_workflow:
        fail("Repository Quality does not run the P3 gate")

    doc = normalized_markdown(DOC.read_text(encoding="utf-8"))
    for phrase in ("19 joints","eight customization slots","leader pose","procedural fallback","native .uasset","p4"):
        if phrase not in doc:
            fail(f"documentation missing required contract phrase: {phrase}")
    roadmap = normalized_markdown(ROADMAP.read_text(encoding="utf-8"))
    if "p3 — authored character + modular cosmetics" not in roadmap or "source-complete" not in roadmap:
        fail("authored production roadmap does not record P3 source completion")

    print(
        "P3 authored character source validated: 19-joint rig, 8 modular slots, deterministic skinned geometry, "
        "strictly reducing LODs, Blender/Unreal bridges, privacy and fail-closed native boundary."
    )


if __name__ == "__main__":
    main()
