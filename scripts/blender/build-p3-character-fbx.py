"""Build the P3 Child Explorer rig and FBX payload inside Blender.

Usage:
  blender --background --python scripts/blender/build-p3-character-fbx.py -- \
    --source-dir artifacts/p3/source --output-dir artifacts/p3/fbx

The source bundle remains authoritative. This script does not claim Unreal .uasset
certification; it only converts the deterministic P3 source into DCC/native payloads.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import bpy
from mathutils import Vector


def parse_args():
    argv = sys.argv
    if "--" in argv:
        argv = argv[argv.index("--") + 1 :]
    else:
        argv = []
    ap = argparse.ArgumentParser()
    ap.add_argument("--source-dir", required=True)
    ap.add_argument("--output-dir", required=True)
    return ap.parse_args(argv)


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)


def ensure_material(name, rgb):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.diffuse_color = (float(rgb[0]), float(rgb[1]), float(rgb[2]), 1.0)
    return mat


def build_armature(bundle):
    arm_data = bpy.data.armatures.new("SKEL_WM_ChildExplorer")
    arm_obj = bpy.data.objects.new("SKEL_WM_ChildExplorer", arm_data)
    bpy.context.collection.objects.link(arm_obj)
    bpy.context.view_layer.objects.active = arm_obj
    arm_obj.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")

    joint_by_id = {j["id"]: j for j in bundle["skeleton"]["joints"]}
    children = {jid: [] for jid in joint_by_id}
    for j in bundle["skeleton"]["joints"]:
        if j["parent"]:
            children[j["parent"]].append(j["id"])

    for joint in bundle["skeleton"]["joints"]:
        b = arm_data.edit_bones.new(joint["id"])
        b.head = Vector(joint["headCm"])
        child_ids = children[joint["id"]]
        if child_ids:
            child_heads = [Vector(joint_by_id[c]["headCm"]) for c in child_ids]
            target = sum(child_heads, Vector()) / len(child_heads)
            if (target - b.head).length < 1.0:
                target = b.head + Vector((0.0, 0.0, 5.0))
            b.tail = target
        else:
            if joint["id"].startswith("foot_"):
                b.tail = b.head + Vector((12.0, 0.0, 0.0))
            elif joint["id"].startswith("hand_"):
                b.tail = b.head + Vector((6.0, 0.0, -2.0))
            elif joint["id"] == "jaw":
                b.tail = b.head + Vector((4.0, 0.0, -2.0))
            else:
                b.tail = b.head + Vector((0.0, 0.0, 6.0))
    for joint in bundle["skeleton"]["joints"]:
        if joint["parent"]:
            arm_data.edit_bones[joint["id"]].parent = arm_data.edit_bones[joint["parent"]]

    bpy.ops.object.mode_set(mode="OBJECT")
    arm_obj.select_set(False)
    return arm_obj


def build_mesh(module, lod, materials, armature):
    verts = [tuple(v) for v in lod["vertices"]]
    faces = [tuple(f) for f in lod["triangles"]]
    mesh = bpy.data.meshes.new(f"{module['assetName']}_LOD{lod['lod']}_Mesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update()

    obj = bpy.data.objects.new(f"{module['assetName']}_LOD{lod['lod']}", mesh)
    bpy.context.collection.objects.link(obj)

    uv_layer = mesh.uv_layers.new(name="UVMap")
    for poly in mesh.polygons:
        poly.use_smooth = True
        for loop_index in poly.loop_indices:
            vertex_index = mesh.loops[loop_index].vertex_index
            uv_layer.data[loop_index].uv = lod["uv0"][vertex_index]

    mesh.materials.append(materials[module["material"]])

    if module["kind"] == "skinned":
        for joint in armature.data.bones:
            obj.vertex_groups.new(name=joint.name)
        for vertex_index, influences in enumerate(lod["weights"]):
            for bone, weight in influences:
                obj.vertex_groups[bone].add([vertex_index], float(weight), "REPLACE")
        modifier = obj.modifiers.new(name="Armature", type="ARMATURE")
        modifier.object = armature
        obj.parent = armature

    return obj


def export_selected(path, objects):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.export_scene.fbx(
        filepath=str(path),
        use_selection=True,
        apply_unit_scale=True,
        bake_space_transform=False,
        object_types={"ARMATURE", "MESH"},
        add_leaf_bones=False,
        bake_anim=False,
        mesh_smooth_type="FACE",
        axis_forward="-Y",
        axis_up="Z",
    )
    bpy.ops.object.select_all(action="DESELECT")


def main():
    args = parse_args()
    source_dir = Path(args.source_dir).resolve()
    output_dir = Path(args.output_dir).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    source_path = source_dir / "WM_ChildExplorer_P3.source.json"
    if not source_path.exists():
        raise RuntimeError(f"missing source bundle: {source_path}")

    bundle = json.loads(source_path.read_text(encoding="utf-8"))
    clear_scene()
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 0.01

    materials = {name: ensure_material(name, rgb) for name, rgb in bundle["materials"].items()}
    armature = build_armature(bundle)
    report = {"schemaVersion": 1, "source": str(source_path), "exports": []}

    for module in bundle["modules"]:
        for lod in module["lods"]:
            obj = build_mesh(module, lod, materials, armature)
            fbx_path = output_dir / f"{module['assetName']}_LOD{lod['lod']}.fbx"
            selection = [obj, armature] if module["kind"] == "skinned" else [obj]
            export_selected(fbx_path, selection)
            report["exports"].append({
                "slot": module["slot"],
                "assetName": module["assetName"],
                "lod": lod["lod"],
                "kind": module["kind"],
                "triangleCount": lod["triangleCount"],
                "path": fbx_path.name,
            })
            bpy.data.objects.remove(obj, do_unlink=True)

    (output_dir / "p3-blender-export-report.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )
    print(f"P3 Blender export complete: {len(report['exports'])} FBX payload(s).")


if __name__ == "__main__":
    main()
