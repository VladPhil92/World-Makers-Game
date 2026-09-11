#!/usr/bin/env python3
"""Blender-side builder for the deterministic World Makers first-person source bundle.

Run with Blender in background mode, for example:
  blender -b --python scripts/blender-build-first-person-authored.py -- \
    --bundle Build/FirstPersonAuthored/first-person-authored-source-v1.json \
    --output-dir Build/FirstPersonAuthored/FBX
"""
from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

import bpy


def arguments():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--bundle", required=True)
    parser.add_argument("--output-dir", required=True)
    return parser.parse_args(argv)


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for collection in (bpy.data.meshes, bpy.data.armatures, bpy.data.materials, bpy.data.actions):
        pass


def material(name, spec):
    existing = bpy.data.materials.get(name)
    if existing:
        return existing
    mat = bpy.data.materials.new(name)
    color = spec.get("baseColor", [0.5, 0.5, 0.5])
    mat.diffuse_color = (color[0], color[1], color[2], 1.0)
    mat.roughness = float(spec.get("roughness", 0.5))
    return mat


def build_mesh_object(name, source, materials):
    data = bpy.data.meshes.new(name + "_Mesh")
    data.from_pydata(source["vertices"], [], source["triangles"])
    data.update()
    obj = bpy.data.objects.new(name, data)
    bpy.context.collection.objects.link(obj)

    material_name = source["material"]
    data.materials.append(materials[material_name])

    uv_layer = data.uv_layers.new(name="UVMap")
    uv0 = source["uv0"]
    for polygon in data.polygons:
        for loop_index in polygon.loop_indices:
            vertex_index = data.loops[loop_index].vertex_index
            uv_layer.data[loop_index].uv = uv0[vertex_index]
    return obj


def build_armature(bundle):
    armature_data = bpy.data.armatures.new("SKEL_WM_FirstPersonArms")
    armature = bpy.data.objects.new("SKEL_WM_FirstPersonArms", armature_data)
    bpy.context.collection.objects.link(armature)
    bpy.context.view_layer.objects.active = armature
    armature.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    edit_bones = {}
    by_id = {item["id"]: item for item in bundle["skeleton"]["bones"]}
    for item in bundle["skeleton"]["bones"]:
        bone = armature_data.edit_bones.new(item["id"])
        head = item["headCm"]
        bone.head = head
        child_heads = [child["headCm"] for child in bundle["skeleton"]["bones"] if child.get("parent") == item["id"]]
        if child_heads:
            tail = child_heads[0]
            if sum((tail[i] - head[i]) ** 2 for i in range(3)) < 1.0:
                tail = [head[0] + 4.0, head[1], head[2]]
        else:
            tail = [head[0] + 6.0, head[1], head[2]]
        bone.tail = tail
        edit_bones[item["id"]] = bone
    for bone_id, item in by_id.items():
        parent_id = item.get("parent")
        if parent_id:
            edit_bones[bone_id].parent = edit_bones[parent_id]
    bpy.ops.object.mode_set(mode="OBJECT")
    armature.select_set(False)
    return armature


def apply_skin(obj, armature, source):
    for bone in armature.data.bones:
        obj.vertex_groups.new(name=bone.name)
    for vertex_index, weights in enumerate(source["weights"]):
        for bone_name, weight in weights:
            obj.vertex_groups[bone_name].add([vertex_index], float(weight), "REPLACE")
    modifier = obj.modifiers.new("Armature", "ARMATURE")
    modifier.object = armature
    obj.parent = armature


def export_selected(path, object_types=("MESH", "ARMATURE"), bake_anim=False):
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.export_scene.fbx(
        filepath=str(path),
        use_selection=True,
        object_types=set(object_types),
        apply_unit_scale=True,
        global_scale=1.0,
        add_leaf_bones=False,
        bake_anim=bake_anim,
        bake_anim_use_all_actions=False,
        bake_anim_use_nla_strips=False,
        bake_anim_simplify_factor=0.0,
    )


def select_only(*objects):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    if objects:
        bpy.context.view_layer.objects.active = objects[0]


def build_static_assets(bundle, materials, output_dir):
    names = {
        "tool.scanner": "SM_WM_Scanner",
        "tool.build": "SM_WM_BuildTool",
        "tool.measure": "SM_WM_MeasureTool",
        "device.wrist": "SM_WM_WristDevice",
    }
    for slot_id, unreal_name in names.items():
        for lod_index, source in enumerate(bundle["assets"][slot_id]["lods"]):
            obj = build_mesh_object(f"{unreal_name}_LOD{lod_index}", source, materials)
            select_only(obj)
            export_selected(Path(output_dir) / f"{unreal_name}_LOD{lod_index}.fbx", ("MESH",), False)
            bpy.data.objects.remove(obj, do_unlink=True)


def build_arms(bundle, materials, armature, output_dir):
    for lod_index, source in enumerate(bundle["assets"]["arms.firstperson"]["lods"]):
        obj = build_mesh_object(f"SK_WM_FirstPersonArms_LOD{lod_index}", source, materials)
        apply_skin(obj, armature, source)
        select_only(obj, armature)
        export_selected(Path(output_dir) / f"SK_WM_FirstPersonArms_LOD{lod_index}.fbx", ("MESH", "ARMATURE"), False)
        bpy.data.objects.remove(obj, do_unlink=True)


def set_pose_key(pose_bone, spec, frame):
    pose_bone.rotation_mode = "XYZ"
    location = spec.get("locationCm", [0, 0, 0])
    rotation = spec.get("rotationDeg", [0, 0, 0])
    pose_bone.location = location
    pose_bone.rotation_euler = [math.radians(value) for value in rotation]
    pose_bone.keyframe_insert(data_path="location", frame=frame)
    pose_bone.keyframe_insert(data_path="rotation_euler", frame=frame)


def build_animations(bundle, armature, output_dir):
    file_names = {
        "tool-raise": "A_FP_ToolRaise",
        "tool-lower": "A_FP_ToolLower",
        "scan-anticipate": "A_FP_ScanAnticipate",
        "scan-hold": "A_FP_ScanHold",
        "scan-settle": "A_FP_ScanSettle",
        "build-point": "A_FP_BuildPoint",
        "build-confirm": "A_FP_BuildConfirm",
        "measure-focus": "A_FP_MeasureFocus",
        "observe-focus": "A_FP_ObserveFocus",
    }
    bpy.context.scene.render.fps = 30
    for clip in bundle["animations"]:
        action_id = clip["actionId"]
        action = bpy.data.actions.new(file_names[action_id])
        armature.animation_data_create()
        armature.animation_data.action = action
        for pose_bone in armature.pose.bones:
            pose_bone.location = (0, 0, 0)
            pose_bone.rotation_mode = "XYZ"
            pose_bone.rotation_euler = (0, 0, 0)
        for key in clip["keys"]:
            for bone_name, spec in key["bones"].items():
                pose_bone = armature.pose.bones.get(bone_name)
                if pose_bone:
                    set_pose_key(pose_bone, spec, int(key["frame"]))
        bpy.context.scene.frame_start = 0
        bpy.context.scene.frame_end = int(clip["frameCount"]) - 1
        select_only(armature)
        export_selected(Path(output_dir) / f"{file_names[action_id]}.fbx", ("ARMATURE",), True)
        armature.animation_data.action = None


def main():
    args = arguments()
    bundle = json.loads(Path(args.bundle).read_text(encoding="utf-8"))
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    clear_scene()
    bpy.context.scene.unit_settings.system = "METRIC"
    bpy.context.scene.unit_settings.scale_length = 0.01
    materials = {name: material(name, spec) for name, spec in bundle["materials"].items()}
    armature = build_armature(bundle)
    build_arms(bundle, materials, armature, output_dir)
    build_static_assets(bundle, materials, output_dir)
    build_animations(bundle, armature, output_dir)
    print(f"First-person authored FBX export complete: {output_dir}")


if __name__ == "__main__":
    main()
