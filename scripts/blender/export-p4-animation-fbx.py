"""Run in Blender background mode to bake P4 clip curves onto the P3 armature and export animation FBX files."""
from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

import bpy


def parse_args():
    argv = sys.argv
    argv = argv[argv.index("--") + 1:] if "--" in argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--rig-fbx", required=True)
    parser.add_argument("--source", required=True)
    parser.add_argument("--output-dir", required=True)
    return parser.parse_args(argv)


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for action in list(bpy.data.actions):
        bpy.data.actions.remove(action)


def import_rig(path: Path):
    bpy.ops.import_scene.fbx(filepath=str(path), automatic_bone_orientation=False)
    armatures = [obj for obj in bpy.context.scene.objects if obj.type == "ARMATURE"]
    if len(armatures) != 1:
        raise RuntimeError(f"expected exactly one P3 armature, found {len(armatures)}")
    armature = armatures[0]
    for obj in list(bpy.context.scene.objects):
        if obj != armature:
            bpy.data.objects.remove(obj, do_unlink=True)
    return armature


def reset_pose(armature):
    for bone in armature.pose.bones:
        bone.rotation_mode = "XYZ"
        bone.rotation_euler = (0.0, 0.0, 0.0)
        bone.location = (0.0, 0.0, 0.0)


def bake_clip(armature, clip):
    fps = int(clip["fps"])
    duration = float(clip["durationSeconds"])
    scene = bpy.context.scene
    scene.render.fps = fps
    scene.frame_start = 1
    scene.frame_end = max(2, int(round(duration * fps)) + 1)
    reset_pose(armature)
    if not armature.animation_data:
        armature.animation_data_create()
    action = bpy.data.actions.new(name=clip["assetName"])
    armature.animation_data.action = action

    for bone_name, samples in clip["tracks"].items():
        bone = armature.pose.bones.get(bone_name)
        if bone is None:
            raise RuntimeError(f"{clip['id']}: missing bone {bone_name}")
        bone.rotation_mode = "XYZ"
        for sample in samples:
            frame = int(round(float(sample["t"]) * fps)) + 1
            if "r" in sample:
                bone.rotation_euler = tuple(math.radians(float(v)) for v in sample["r"])
                bone.keyframe_insert(data_path="rotation_euler", frame=frame, group=bone_name)
            if "p" in sample:
                bone.location = tuple(float(v) * 0.01 for v in sample["p"])
                bone.keyframe_insert(data_path="location", frame=frame, group=bone_name)
    return action


def export_action(armature, path: Path):
    bpy.ops.object.select_all(action="DESELECT")
    armature.select_set(True)
    bpy.context.view_layer.objects.active = armature
    bpy.ops.export_scene.fbx(
        filepath=str(path),
        use_selection=True,
        object_types={"ARMATURE"},
        add_leaf_bones=False,
        bake_anim=True,
        bake_anim_use_all_bones=True,
        bake_anim_use_nla_strips=False,
        bake_anim_use_all_actions=False,
        bake_anim_force_startend_keying=True,
        bake_anim_simplify_factor=0.0,
        axis_forward="-Y",
        axis_up="Z",
    )


def main():
    args = parse_args()
    rig_fbx = Path(args.rig_fbx).resolve()
    source_path = Path(args.source).resolve()
    output_dir = Path(args.output_dir).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    if not rig_fbx.is_file() or not source_path.is_file():
        raise RuntimeError("P4 requires the P3 LOD0 rig FBX and generated P4 source bundle")

    source = json.loads(source_path.read_text(encoding="utf-8"))
    clear_scene()
    armature = import_rig(rig_fbx)
    report = {"schemaVersion": 1, "exports": []}
    for clip in source["animation"]["clips"]:
        action = bake_clip(armature, clip)
        target = output_dir / f"{clip['assetName']}.fbx"
        export_action(armature, target)
        report["exports"].append({
            "id": clip["id"],
            "assetName": clip["assetName"],
            "durationSeconds": clip["durationSeconds"],
            "fps": clip["fps"],
            "loop": clip["loop"],
            "path": target.name,
            "curveCount": len(action.fcurves),
        })
        bpy.data.actions.remove(action)
        armature.animation_data.action = None

    (output_dir / "p4-animation-export-report.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )
    print(f"P4 animation export complete: {len(report['exports'])} clip(s).")


if __name__ == "__main__":
    main()
